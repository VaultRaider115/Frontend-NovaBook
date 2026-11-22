#include "JefeVentasView.h"
#include "ApiClient.h"
#include "Limites.h"
#include "Theme.h"
#include "Toast.h"
#include "Ui.h"
#include <QDate>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QVBoxLayout>

namespace {
// Rango y búsqueda elegidos, mientras la app esté abierta (como en la bitácora).
struct FiltrosVentas { QDate desde, hasta; QString q; };
FiltrosVentas &filtrosGuardados() {
    static FiltrosVentas f;
    return f;
}

QLabel *chipPago(const QString &metodo) {
    const QString m = metodo.trimmed().toUpper();
    return Ui::chip(m.isEmpty() ? "—" : m, m == "TARJETA" ? "blue" : m == "EFECTIVO" ? "mint" : "neutral");
}

QWidget *statCard(const QString &titulo, QLabel *&valor, QWidget *parent) {
    auto *card = Ui::card(parent);
    auto *l = new QVBoxLayout(card);
    l->setContentsMargins(20, 12, 20, 12);
    l->setSpacing(2);
    l->addWidget(Ui::label(titulo, "Muted", card));
    valor = Ui::label("0", "BigNumber", card);
    l->addWidget(valor);
    return card;
}
} // namespace

JefeVentasView::JefeVentasView(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(24);

    auto *top = new QHBoxLayout();
    top->addWidget(Ui::pageHeader("Jefatura · Ventas", "Tickets de Venta",
                                  "Consulta los tickets del punto de venta: qué se vendió, a quién y quién lo cobró.",
                                  this), 1);
    top->addWidget(statCard("Tickets del rango", m_lblTickets, this), 0, Qt::AlignTop);
    top->addWidget(statCard("Total vendido", m_lblVendido, this), 0, Qt::AlignTop);
    layout->addLayout(top);

    // Filtros
    auto *filters = Ui::card(this);
    auto *fl = new QHBoxLayout(filters);
    fl->setContentsMargins(16, 16, 16, 16);
    fl->setSpacing(12);
    m_desde = new QDateEdit(QDate::currentDate(), filters);
    m_hasta = new QDateEdit(QDate::currentDate(), filters);
    for (auto *d : {m_desde, m_hasta}) {
        d->setCalendarPopup(true);
        d->setDisplayFormat("dd 'de' MMMM, yyyy");
        d->setMinimumWidth(235);
        d->setMaximumDate(QDate::currentDate());
        Ui::decorateDate(d);
    }
    m_desde->setToolTip("Desde");
    m_desde->setAccessibleName("Fecha inicial");
    m_hasta->setToolTip("Hasta");
    m_hasta->setAccessibleName("Fecha final");
    auto *btnHoy = Ui::button("Hoy", "outline", filters);
    btnHoy->setToolTip("Regresar el rango al día de hoy");
    m_txtBuscar = Ui::searchBox("Ticket #, cliente o vendedor…", filters);
    m_txtBuscar->setMaxLength(Limites::kNombreCliente);   // lo más largo que se busca (API: igual)
    m_txtBuscar->setMinimumWidth(260);
    m_btnBuscar = Ui::button("Consultar", "primary", filters, "search-dark");

    {
        const FiltrosVentas &g = filtrosGuardados();
        const QDate hoy = QDate::currentDate();
        if (g.desde.isValid() && g.hasta.isValid() && g.desde <= g.hasta && g.hasta <= hoy) {
            m_desde->setDate(g.desde);
            m_hasta->setDate(g.hasta);
        }
        m_txtBuscar->setText(g.q);
    }

    fl->addWidget(m_desde);
    fl->addWidget(m_hasta);
    fl->addWidget(btnHoy);
    fl->addWidget(m_txtBuscar, 1);
    fl->addWidget(m_btnBuscar);
    layout->addWidget(filters);

    m_table = Ui::table({"Ticket", "Fecha", "Vendedor", "Cliente", "Pago", "Títulos", "Piezas", "Total", "Acción"},
                        3, this, 56);
    for (int c : {0, 1, 4, 5, 6, 7, 8})
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
    m_table->setColumnWidth(2, 200);
    layout->addWidget(m_table, 1);

    m_lblPie = Ui::label(QString(), "Muted", this);
    layout->addWidget(m_lblPie);

    connect(m_btnBuscar, &QPushButton::clicked, this, &JefeVentasView::cargar);
    connect(m_txtBuscar, &QLineEdit::returnPressed, this, &JefeVentasView::cargar);
    connect(btnHoy, &QPushButton::clicked, this, [this]() {
        m_desde->setDate(QDate::currentDate());
        m_hasta->setDate(QDate::currentDate());
        cargar();
    });
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (row >= 0 && row < m_tickets.size())
            verTicket(m_tickets.at(row).value("id_venta").toVariant().toLongLong());
    });
}

void JefeVentasView::cargar() {
    if (m_hasta->date() < m_desde->date()) {
        Toast::showToast(this, "La fecha final no puede ser anterior a la inicial", Toast::Warning);
        return;
    }
    FiltrosVentas &g = filtrosGuardados();
    g.desde = m_desde->date();
    g.hasta = m_hasta->date();
    g.q = m_txtBuscar->text().trimmed();

    const int consulta = ++m_consulta;
    m_btnBuscar->setEnabled(false);
    ApiClient::instance()->listarTicketsJefe(
        g.desde.toString("yyyy-MM-dd"), g.hasta.toString("yyyy-MM-dd"), g.q,
        [this, consulta](bool ok, const QJsonValue &data, const QString &msg) {
            if (consulta != m_consulta) return;
            m_btnBuscar->setEnabled(true);
            if (!ok || !data.isArray()) {
                Toast::showToast(this, "No se pudieron cargar los tickets: " + msg, Toast::Error);
                return;
            }
            m_tickets.clear();
            for (const auto &v : data.toArray()) m_tickets.append(v.toObject());
            pintar();
        });
}

void JefeVentasView::pintar() {
    m_table->clearContents();
    m_table->setRowCount(m_tickets.size());
    double vendido = 0.0;
    int piezas = 0;
    for (int i = 0; i < m_tickets.size(); ++i) {
        const QJsonObject &t = m_tickets.at(i);
        const qint64 id = t.value("id_venta").toVariant().toLongLong();
        const double total = t.value("total").toDouble();
        const int pzs = t.value("piezas").toInt();
        const int devueltas = t.value("piezas_devueltas").toInt();
        vendido += total;
        piezas += pzs;

        Ui::cellText(m_table, i, 0, QString("#%1").arg(id), true, true);
        Ui::cellText(m_table, i, 1, t.value("fecha_hora").toString().left(16), true, false, Theme::TextMuted);
        Ui::cellText(m_table, i, 2, t.value("vendedor").toString());
        Ui::cellText(m_table, i, 3, t.value("nombre_cliente").toString(), false, true);
        auto *chip = chipPago(t.value("metodo_pago").toString());
        Ui::cellWidget(m_table, i, 4, chip);
        Ui::fitColumn(m_table, 4, chip);
        Ui::cellText(m_table, i, 5, QString::number(t.value("titulos").toInt()));
        if (devueltas > 0)
            Ui::cellText(m_table, i, 6, QString("%1 (%2 dev.)").arg(pzs).arg(devueltas), false, false, Theme::Warning)
                ->setToolTip(QString("El cliente ya devolvió %1 pieza(s) de este ticket").arg(devueltas));
        else
            Ui::cellText(m_table, i, 6, QString::number(pzs));
        Ui::cellText(m_table, i, 7, Ui::money(total), false, true);
        auto *ver = Ui::iconButton("eye", "Ver ticket");
        connect(ver, &QPushButton::clicked, this, [this, id]() { verTicket(id); });
        Ui::cellWidget(m_table, i, 8, ver);
    }
    m_lblTickets->setText(QString::number(m_tickets.size()));
    m_lblVendido->setText(Ui::money(vendido));
    m_lblPie->setText(m_tickets.isEmpty()
                          ? "No hay tickets en este rango."
                          : QString("%1 ticket(s) · %2 pieza(s) · doble clic o el ojo para ver el ticket")
                                .arg(m_tickets.size())
                                .arg(piezas));
}

void JefeVentasView::verTicket(qint64 idVenta) {
    ApiClient::instance()->consultarTicketJefe(idVenta, [this, idVenta](bool ok, const QJsonValue &data,
                                                                       const QString &msg) {
        if (!ok || !data.isObject()) {
            Toast::showToast(this, QString("No se pudo abrir el ticket #%1: %2").arg(idVenta).arg(msg), Toast::Error);
            return;
        }
        const QJsonObject t = data.toObject();
        QString vendedor;
        for (const QJsonObject &r : std::as_const(m_tickets))
            if (r.value("id_venta").toVariant().toLongLong() == idVenta) vendedor = r.value("vendedor").toString();

        Ui::FormDialog dlg(QString("Ticket #%1").arg(idVenta), "Cerrar", this, 980);
        for (QPushButton *b : dlg.findChildren<QPushButton *>())
            if (b != dlg.okButton() && b->text() == "Cancelar") b->hide();

        const QJsonArray items = t.value("items").toArray();
        int devueltas = 0;
        for (const auto &v : items) devueltas += v.toObject().value("cantidad_devuelta").toInt();
        const bool vigente = t.value("vigente").toBool();
        auto *resumen = new QLabel(
            QString("<b>%1</b><br><span style=\"color:%2;\">%3 · Vendió %4 · Pago %5 · Total %6</span><br>"
                    "<span style=\"color:%7;\">%8</span>")
                .arg(t.value("nombre_cliente").toString().toHtmlEscaped(), Theme::TextMuted,
                     t.value("fecha_hora").toString().left(16),
                     vendedor.isEmpty() ? "—" : vendedor.toHtmlEscaped(),
                     t.value("metodo_pago").toString(), Ui::money(t.value("total").toDouble()),
                     vigente ? QString(Theme::TextMuted) : QString(Theme::Warning),
                     vigente ? QString("Dentro de los 15 días para devolución (%1 día(s))")
                                   .arg(t.value("dias_transcurridos").toInt())
                             : QString("Fuera del plazo de devolución (%1 días)")
                                   .arg(t.value("dias_transcurridos").toInt())),
            &dlg);
        resumen->setTextFormat(Qt::RichText);
        resumen->setWordWrap(true);

        auto *tabla = Ui::table({"EAN", "Tipo", "Título", "Vendidas", "Devueltas", "Precio", "Subtotal"}, 2, &dlg, 44);
        tabla->setRowCount(items.size());
        for (int i = 0; i < items.size(); ++i) {
            const QJsonObject it = items.at(i).toObject();
            const int vend = it.value("cantidad_vendida").toInt();
            const int dev = it.value("cantidad_devuelta").toInt();
            const double precio = it.value("precio_unitario").toDouble();
            Ui::cellText(tabla, i, 0, QString::number(it.value("codigo_ean").toVariant().toLongLong()), true);
            auto *chip = Ui::tipoChip(it.value("tipo_producto").toString());
            Ui::cellWidget(tabla, i, 1, chip);
            Ui::fitColumn(tabla, 1, chip);
            Ui::cellText(tabla, i, 2, it.value("titulo").toString(), false, true);
            Ui::cellText(tabla, i, 3, QString::number(vend));
            Ui::cellText(tabla, i, 4, dev > 0 ? QString::number(dev) : "—", false, dev > 0,
                         dev > 0 ? QString(Theme::Warning) : QString(Theme::TextMuted));
            Ui::cellText(tabla, i, 5, Ui::money(precio), false, false, Theme::TextMuted);
            Ui::cellText(tabla, i, 6, Ui::money(precio * vend), false, true);
        }
        for (int col : {0, 3, 4, 5, 6})
            tabla->horizontalHeader()->setSectionResizeMode(col, QHeaderView::ResizeToContents);
        tabla->setMinimumHeight(qMin(380, 56 + 44 * qMax(1, static_cast<int>(items.size()))));

        dlg.addField("Cliente", resumen, 0, 0, 2);
        dlg.addField("Libros / revistas vendidos", tabla, 1, 0, 2);
        if (devueltas > 0) {
            auto *nota = new QLabel(QString("El cliente ya devolvió %1 pieza(s) de este ticket "
                                            "(Devolución de Cliente).").arg(devueltas), &dlg);
            nota->setObjectName("Muted");
            nota->setWordWrap(true);
            dlg.addField("", nota, 2, 0, 2);
        }
        connect(dlg.okButton(), &QPushButton::clicked, &dlg, &QDialog::accept);
        dlg.exec();
    });
}
