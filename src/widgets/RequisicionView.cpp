#include "RequisicionView.h"
#include "Theme.h"
#include "Ui.h"
#include "Toast.h"
#include "EanClassifier.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSharedPointer>

RequisicionView::RequisicionView(const QString &eyebrow, const QString &title, const QString &sendText,
                                 const QString &addedMessage, QWidget *parent)
    : QWidget(parent), m_okMessage(addedMessage)
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
    m_txtEan->setPlaceholderText("9780000000000");
    m_txtEan->addAction(QIcon(":/icons/scan-gold.svg"), QLineEdit::LeadingPosition);
    row->addWidget(Ui::field("Código EAN-13", m_txtEan, addCard), 1);

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
        QString titulo = QString("Artículo %1").arg(ean);
        if (ok && data.isArray() && !data.toArray().isEmpty())
            titulo = data.toArray().first().toObject().value("titulo").toString(titulo);

        bool found = false;
        for (auto &p : m_items) {
            if (p.ean == ean) { p.cantidad += cant; found = true; break; }
        }
        if (!found) m_items.append({ean, tipo, titulo, cant});

        m_txtEan->clear();
        m_spnCantidad->setValue(1);
        m_txtEan->setFocus();
        render();
    });
}

void RequisicionView::render() {
    m_table->clearContents();
    m_table->setRowCount(m_items.size());
    int total = 0;
    for (int i = 0; i < m_items.size(); ++i) {
        const Partida &p = m_items.at(i);
        total += p.cantidad;
        Ui::cellText(m_table, i, 0, QString::number(p.ean), true);
        Ui::cellWidget(m_table, i, 1, Ui::tipoChip(p.tipo));
        Ui::cellText(m_table, i, 2, p.nombre);
        Ui::cellText(m_table, i, 3, QString::number(p.cantidad));
        auto *quitar = Ui::iconButton("trash", "Quitar partida");
        connect(quitar, &QPushButton::clicked, this, [this, i]() { m_items.removeAt(i); render(); });
        Ui::cellWidget(m_table, i, 4, quitar);
    }
    m_lblTotal->setText(QString("Piezas Totales: <b style=\"color:#FFFFFF;\">%1</b>").arg(total));
}

void RequisicionView::enviarTodo() {
    if (m_items.isEmpty()) {
        Toast::showToast(this, "Agrega al menos un artículo a la lista", Toast::Warning);
        return;
    }
    // La API traslada una partida por petición: se envían todas y se reporta cuáles fallaron.
    struct Estado { int pendientes; QList<Partida> fallidas; QStringList errores; };
    auto st = QSharedPointer<Estado>::create();
    st->pendientes = m_items.size();
    m_btnEnviar->setEnabled(false);

    const QList<Partida> items = m_items;
    for (const Partida &p : items) {
        trasladar(p.ean, p.tipo, p.cantidad, [this, st, p](bool ok, const QJsonValue &, const QString &msg) {
            if (!ok) { st->fallidas << p; st->errores << QString("%1: %2").arg(p.ean).arg(msg); }
            if (--st->pendientes > 0) return;

            m_btnEnviar->setEnabled(true);
            m_items = st->fallidas; // lo que falló se queda en la lista para reintentar
            render();
            if (st->fallidas.isEmpty())
                Toast::showToast(this, m_okMessage, Toast::Success);
            else
                Toast::showToast(this, QString("%1 partida(s) no se pudieron trasladar — %2")
                                           .arg(st->fallidas.size()).arg(st->errores.first()), Toast::Error, 6000);
        });
    }
}
