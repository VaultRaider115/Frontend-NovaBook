#include "ProductoBuscador.h"
#include "ApiClient.h"
#include "Toast.h"
#include "Limites.h"
#include <QAbstractItemView>
#include <QCompleter>
#include <QJsonArray>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStandardItemModel>
#include <QTimer>

namespace {
constexpr int kMinLetras = 3;     // caracteres mínimos para buscar por título
constexpr int kMaxResultados = 15;
constexpr int kRolEan = Qt::UserRole + 1;
}

bool ProductoBuscador::esEanCompleto(const QString &texto) {
    static const QRegularExpression rx("^[0-9]{13}$");
    return rx.match(texto.trimmed()).hasMatch();
}

bool ProductoBuscador::esBusquedaPorTitulo(const QString &texto) {
    static const QRegularExpression rx("[^0-9]");
    return rx.match(texto.trimmed()).hasMatch();
}

ProductoBuscador::ProductoBuscador(QLineEdit *edit, Filtro filtro)
    : QObject(edit), m_edit(edit), m_filtro(std::move(filtro))
{
    // Solo dígitos: máximo 13 (EAN). Con letras: título de hasta Limites::kBusqueda caracteres.
    edit->setMaxLength(Limites::kBusqueda);
    edit->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QString("^(?:[0-9]{0,13}|(?=.*[^0-9]).{0,%1})$").arg(Limites::kBusqueda)), edit));
    edit->setClearButtonEnabled(true);

    m_modelo = new QStandardItemModel(this);
    m_completer = new QCompleter(m_modelo, this);
    // La lista muestra lo que trae la API (ya filtrado por título); al elegir
    // se escribe el EAN en el campo (rol kRolEan), no el texto del renglón.
    m_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    m_completer->setCompletionRole(kRolEan);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setMaxVisibleItems(8);
    m_completer->setWidget(edit);
    m_completer->popup()->setMinimumWidth(520);
    connect(m_completer, QOverload<const QModelIndex &>::of(&QCompleter::activated), this,
            [this](const QModelIndex &idx) {
                if (!m_edit) return;
                const QString ean = idx.data(kRolEan).toString();
                m_edit->setText(ean);
                m_edit->setFocus();
                m_edit->setCursorPosition(ean.size());
            });

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(300);
    connect(m_timer, &QTimer::timeout, this, [this]() { buscar(false); });
    connect(edit, &QLineEdit::textEdited, this, [this](const QString &t) {
        const QString q = t.trimmed();
        if (esBusquedaPorTitulo(q) && q.size() >= kMinLetras) m_timer->start();
        else { m_timer->stop(); m_completer->popup()->hide(); }
    });

    edit->installEventFilter(this);
}

bool ProductoBuscador::eventFilter(QObject *obj, QEvent *ev) {
    if (obj == m_edit && ev->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(ev);
        const bool enter = ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter;
        if (enter && !m_completer->popup()->isVisible() && esBusquedaPorTitulo(m_edit->text())) {
            m_timer->stop();
            buscar(true);
            return true;   // se decide al llegar la respuesta
        }
    }
    return QObject::eventFilter(obj, ev);
}

void ProductoBuscador::buscar(bool alPresionarEnter) {
    if (!m_edit) return;
    const QString q = m_edit->text().trimmed();
    if (q.size() < kMinLetras) {
        if (alPresionarEnter)
            Toast::showToast(m_edit->window(), QString("Escribe al menos %1 letras del título o el EAN-13 completo").arg(kMinLetras),
                             Toast::Warning);
        return;
    }

    auto decidir = [this, alPresionarEnter](const QList<QJsonObject> &productos) {
        if (!alPresionarEnter) { mostrar(productos); return; }
        if (productos.isEmpty()) {
            Toast::showToast(m_edit->window(), QString("No se encontró ningún producto con \"%1\"").arg(m_edit->text().trimmed()),
                             Toast::Warning);
        } else if (productos.size() == 1) {
            // Un solo resultado: se elige y la vista lo agrega como si hubieran escaneado el EAN.
            m_edit->setText(QString::number(productos.first().value("codigo_ean").toVariant().toLongLong()));
            emit m_edit->returnPressed();
        } else {
            mostrar(productos);
        }
    };

    if (q == m_ultimaBusqueda) { decidir(m_resultados); return; }

    const int peticion = ++m_peticion;
    QPointer<ProductoBuscador> self(this);
    ApiClient::instance()->consultarExistencias(q, QString(), 0,
        [self, peticion, q, decidir](bool ok, const QJsonValue &data, const QString &) {
            if (!self || !self->m_edit || peticion != self->m_peticion) return;
            if (self->m_edit->text().trimmed() != q) return;   // ya escribieron otra cosa
            QList<QJsonObject> productos;
            if (ok && data.isArray()) {
                for (const auto &v : data.toArray()) {
                    const QJsonObject p = v.toObject();
                    if (!self->m_filtro || self->m_filtro(p)) productos.append(p);
                    if (productos.size() >= kMaxResultados) break;
                }
            }
            self->m_ultimaBusqueda = q;
            self->m_resultados = productos;
            decidir(productos);
        });
}

void ProductoBuscador::mostrar(const QList<QJsonObject> &productos) {
    m_modelo->clear();
    for (const QJsonObject &p : productos) {
        const QString ean = QString::number(p.value("codigo_ean").toVariant().toLongLong());
        auto *item = new QStandardItem(QString("%1  —  %2 · %3 · Piso %4 / Bodega %5")
            .arg(p.value("titulo").toString(), ean, p.value("proveedor").toString())
            .arg(p.value("stock_tienda").toInt()).arg(p.value("stock_bodega").toInt()));
        item->setData(ean, kRolEan);
        item->setToolTip(p.value("autor_o_editorial").toString());
        m_modelo->appendRow(item);
    }
    if (productos.isEmpty()) {
        auto *item = new QStandardItem("Sin resultados para ese título");
        item->setEnabled(false);
        item->setSelectable(false);
        m_modelo->appendRow(item);
    }
    // La lista ocupa al menos el ancho del campo (si no, corta el stock).
    const int ancho = qMax(m_edit->width(), 560);
    m_completer->popup()->setMinimumWidth(ancho);
    QRect r = m_edit->rect();
    r.setWidth(ancho);
    m_completer->complete(r);
}
