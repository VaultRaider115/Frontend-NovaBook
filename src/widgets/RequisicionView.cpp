#include "RequisicionView.h"
#include "Theme.h"
#include "Ui.h"
#include "Toast.h"
#include "EanClassifier.h"
#include "ProductoBuscador.h"
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QVBoxLayout>

RequisicionView::RequisicionView(const QString &eyebrow, const QString &title, const QString &sendText,
                                 const QString &addedMessage, bool salida, QWidget *parent)
    : QWidget(parent), m_okMessage(addedMessage), m_salida(salida)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(18);
    layout->addWidget(Ui::pageHeader(eyebrow, title, "", this));

    auto *addCard = Ui::card(this);
    auto *cl = new QVBoxLayout(addCard);
    cl->setContentsMargins(20, 20, 20, 20);
    cl->setSpacing(10);
    auto *row = new QHBoxLayout();
    row->setSpacing(16);

    m_txtEan = new QLineEdit(addCard);
    new ProductoBuscador(m_txtEan);
    m_txtEan->setPlaceholderText("EAN-13 o título del libro / revista");
    m_txtEan->addAction(QIcon(":/icons/scan-gold.svg"), QLineEdit::LeadingPosition);
    row->addWidget(Ui::field("EAN-13 o título", m_txtEan, addCard), 1);

    m_spnCantidad = new QSpinBox(addCard);
    m_spnCantidad->setRange(1, 9999);
    row->addWidget(Ui::field("Cantidad", Ui::stepper(m_spnCantidad, addCard), addCard));

    auto *btnAdd = Ui::button("Añadir a Lista", "tint", addCard, "plus-gold");
    row->addWidget(btnAdd, 0, Qt::AlignBottom);
    cl->addLayout(row);

    m_lblStatus = new QLabel("Ingresa el código EAN-13 para clasificar automáticamente como Libro o Revista", addCard);
    m_lblStatus->setObjectName("Muted");
    cl->addWidget(m_lblStatus);
    layout->addWidget(addCard);

    // Orden de columnas de los diseños 5 y 8.
    m_table = Ui::table({"EAN", "Tipo", "Descripción", "Cantidad", ""}, 2, this, 56);
    layout->addWidget(m_table, 1);

    auto *foot = Ui::card(this);
    auto *fl = new QHBoxLayout(foot);
    fl->setContentsMargins(20, 20, 20, 20);
    fl->setSpacing(16);
    auto *btnClear = Ui::button("Vaciar Lista", "ghost", foot, "trash");
    fl->addWidget(btnClear);
    // Folio de la última requisición registrada + reimpresión.
    m_lblFolio = new QLabel(foot);
    m_lblFolio->setObjectName("Muted");
    m_lblFolio->hide();
    fl->addWidget(m_lblFolio);
    m_btnImprimir = Ui::button("Imprimir requisición", "outline", foot, "pdf");
    m_btnImprimir->hide();
    fl->addWidget(m_btnImprimir);
    fl->addStretch();
    m_lblTotal = new QLabel("Piezas Totales: <b style=\"color:#FFFFFF;\">0</b>", foot);
    m_lblTotal->setObjectName("Muted");
    fl->addWidget(m_lblTotal);
    m_btnEnviar = Ui::button(sendText, "primary", foot, "check-dark");
    m_btnEnviar->setStyleSheet("QPushButton { padding: 16px 28px; }");
    fl->addWidget(m_btnEnviar);
    layout->addWidget(foot);

    connect(m_txtEan, &QLineEdit::textChanged, this, [this](const QString &t) { EanClassifier::aplicarAControles(t, m_lblStatus); });
    connect(m_txtEan, &QLineEdit::returnPressed, this, &RequisicionView::agregar);
    connect(btnAdd, &QPushButton::clicked, this, &RequisicionView::agregar);
    connect(btnClear, &QPushButton::clicked, this, [this]() { m_items.clear(); render(); });
    connect(m_btnEnviar, &QPushButton::clicked, this, &RequisicionView::enviarTodo);
    connect(m_btnImprimir, &QPushButton::clicked, this, &RequisicionView::imprimirUltima);
}

void RequisicionView::agregar() {
    const QString eanStr = m_txtEan->text().trimmed();
    const auto res = EanClassifier::clasificar(eanStr);
    if (!res.esValido) {
        Toast::showToast(this, res.mensaje, Toast::Warning);
        return;
    }
    const qint64 ean = eanStr.toLongLong();
    const int cant = m_spnCantidad->value();
    const QString tipo = res.tipo == EanClassifier::Tipo::Libro ? "libro" : "revista";

    ApiClient::instance()->consultarExistencias(eanStr, tipo, 0, [this, ean, cant, tipo](bool ok, const QJsonValue &data, const QString &) {
        // La búsqueda puede traer otros títulos: se usa el renglón de este EAN.
        QJsonObject fila;
        if (ok && data.isArray()) {
            for (const auto &v : data.toArray()) {
                if (v.toObject().value("codigo_ean").toVariant().toLongLong() == ean) { fila = v.toObject(); break; }
            }
        }
        if (!ok) {
            Toast::showToast(this, "No se pudo consultar el producto; intenta de nuevo", Toast::Error);
            return;
        }
        // Se valida al agregar (no al registrar) para no armar una lista que luego no se pueda mandar.
        if (fila.isEmpty()) {
            Toast::showToast(this, QString("No hay ningún producto registrado con el EAN %1").arg(ean), Toast::Warning);
            return;
        }
        const QString titulo = fila.value("titulo").toString(QString("Artículo %1").arg(ean));
        const QString editorial = fila.value("proveedor").toString();
        const QString origen = m_salida ? "bodega" : "piso de venta";
        const int disponible = fila.value(m_salida ? "stock_bodega" : "stock_tienda").toInt();
        int yaEnLista = 0;
        for (const auto &p : std::as_const(m_items))
            if (p.ean == ean) yaEnLista = p.cantidad;
        if (disponible <= 0) {
            Toast::showToast(this, QString("\"%1\" no tiene piezas en %2").arg(titulo, origen), Toast::Warning, 6000);
            return;
        }
        if (yaEnLista + cant > disponible) {
            Toast::showToast(this, yaEnLista > 0
                ? QString("\"%1\": solo hay %2 en %3 y ya llevas %4 en la lista").arg(titulo).arg(disponible).arg(origen).arg(yaEnLista)
                : QString("\"%1\": solo hay %2 en %3").arg(titulo).arg(disponible).arg(origen),
                Toast::Warning, 6000);
            return;
        }

        // Diagrama, pasos 2 y 6: una sola editorial por documento.
        if (!m_items.isEmpty() && !editorial.isEmpty() && !m_items.first().editorial.isEmpty()
            && editorial != m_items.first().editorial) {
            Toast::showToast(this, QString("\"%1\" es de %2 y esta requisición es de %3. "
                                           "Registra esta primero y haz otra para %2.")
                                       .arg(titulo, editorial, m_items.first().editorial),
                             Toast::Warning, 6000);
            return;
        }

        bool found = false;
        for (auto &p : m_items) {
            if (p.ean == ean) { p.cantidad += cant; p.disponible = disponible; found = true; break; }
        }
        if (!found) m_items.append({ean, tipo, titulo, cant, editorial, disponible});

        m_txtEan->clear();
        m_spnCantidad->setValue(1);
        m_txtEan->setFocus();
        render();
    });
}

void RequisicionView::render() {
    m_table->clearContents();
    m_table->setRowCount(m_items.size());
    for (int i = 0; i < m_items.size(); ++i) {
        const Partida &p = m_items.at(i);
        Ui::cellText(m_table, i, 0, QString::number(p.ean), true);
        Ui::cellWidget(m_table, i, 1, Ui::tipoChip(p.tipo));
        Ui::cellText(m_table, i, 2, p.nombre);
        {
            // La cantidad se ajusta en la lista, sin pasar de lo que hay en el origen.
            auto *cantidad = new QSpinBox(m_table);
            cantidad->setRange(1, qMax(1, p.disponible));
            cantidad->setFixedWidth(96);   // igual que en Recepción de Salida
            cantidad->setToolTip(QString("Máximo %1 (hay %1 en %2)").arg(p.disponible).arg(m_salida ? "bodega" : "piso de venta"));
            cantidad->setValue(p.cantidad);
            cantidad->setAccessibleName(QString("Cantidad de %1").arg(p.nombre));
            connect(cantidad, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, i](int valor) {
                if (i < 0 || i >= m_items.size()) return;
                m_items[i].cantidad = valor;
                actualizarResumen();
            });
            Ui::cellWidget(m_table, i, 3, cantidad);
        }
        auto *quitar = Ui::iconButton("trash", "Quitar partida");
        connect(quitar, &QPushButton::clicked, this, [this, i]() { m_items.removeAt(i); render(); });
        Ui::cellWidget(m_table, i, 4, quitar);
    }
    actualizarResumen();
}

void RequisicionView::actualizarResumen() {
    int total = 0;
    for (const Partida &p : std::as_const(m_items)) total += p.cantidad;
    const QString editorial = m_items.isEmpty() ? QString() : m_items.first().editorial;
    m_lblTotal->setText((editorial.isEmpty() ? QString() : "Editorial: " + editorial.toHtmlEscaped() + " · ")
                        + QString("Piezas Totales: <b style=\"color:#FFFFFF;\">%1</b>").arg(total));
}

void RequisicionView::enviarTodo() {
    if (m_items.isEmpty()) {
        Toast::showToast(this, "Agrega al menos un artículo a la lista", Toast::Warning);
        return;
    }
    // Un solo documento: la API registra todas las partidas o ninguna.
    QJsonArray items;
    for (const Partida &p : std::as_const(m_items))
        items.append(QJsonObject{{"codigo_ean", p.ean}, {"cantidad", p.cantidad}});

    m_btnEnviar->setEnabled(false);
    ApiClient::instance()->registrarRequisicion(m_salida, items, [this](bool ok, const QJsonValue &data, const QString &msg) {
        m_btnEnviar->setEnabled(true);
        if (!ok || !data.isObject()) {
            // Nada se movió: la lista se conserva para corregir y reintentar.
            QString error = msg;
            if (m_salida) {
                for (const Partida &p : std::as_const(m_items)) {
                    error.replace(QString("EAN %1").arg(p.ean), QString("\"%1\"").arg(p.nombre));
                }
            }
            Toast::showToast(this, "No se registró la requisición: " + error, Toast::Error, 6000);
            return;
        }
        const QJsonObject o = data.toObject();
        m_ultima = {};
        m_ultima.folio = o.value("folio").toString();
        m_ultima.fechaHora = o.value("fecha_hora").toString();
        m_ultima.salida = m_salida;
        m_ultima.elaboro = ApiClient::instance()->currentUser().nombreCompleto();
        m_ultima.editorial = o.value("editorial").toString();
        for (const auto &v : o.value("items").toArray()) {
            const QJsonObject it = v.toObject();
            m_ultima.partidas.append({it.value("codigo_ean").toVariant().toLongLong(),
                                      it.value("tipo_producto").toString(),
                                      it.value("titulo").toString(),
                                      it.value("cantidad").toInt()});
        }

        m_items.clear();
        render();
        m_lblFolio->setText(QString("Folio <b style=\"color:%1;\">#%2</b>").arg(Theme::Accent, m_ultima.folio));
        m_lblFolio->show();
        m_btnImprimir->show();
        Toast::showToast(this, QString("%1 · Folio %2").arg(m_okMessage, m_ultima.folio), Toast::Success);

        // Diagrama de procesos: la requisición se imprime al registrarla.
        imprimirUltima();
    });
}

void RequisicionView::imprimirUltima() {
    if (m_ultima.folio.isEmpty()) return;
    Documentos::guardarYAbrir(this, Documentos::pdfRequisicion(m_ultima),
                              QString("requisicion_%1.pdf").arg(m_ultima.folio));
}
