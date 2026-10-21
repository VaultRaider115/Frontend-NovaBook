#include "JefeBitacoraView.h"
#include "Theme.h"
#include "Ui.h"
#include "CatalogView.h"
#include "ApiClient.h"
#include "Toast.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDate>
#include <QSharedPointer>
#include <algorithm>

static const char *chipPorEvento(const QString &accion) {
    const QString a = accion.toLower();
    if (a.contains("venta")) return "mint";
    if (a.contains("traslado") || a.contains("requisici")) return "indigo";
    if (a.contains("compra")) return "amber";
    if (a.contains("acceso") || a.contains("sesi")) return "gold";
    if (a.contains("devol")) return "danger";
    return "neutral";
}

JefeBitacoraView::JefeBitacoraView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(24);

    // Encabezado + contador de eventos
    auto *top = new QHBoxLayout();
    top->addWidget(Ui::pageHeader("Jefatura · Trazabilidad", "Historial de Movimientos",
                                  "Auditoría de eventos del personal por rango de fechas.", this), 1);
    auto *stat = Ui::card(this);
    auto *sl = new QVBoxLayout(stat);
    sl->setContentsMargins(20, 12, 20, 12);
    sl->setSpacing(2);
    sl->addWidget(Ui::label("Eventos del rango", "Muted", stat));
    m_lblTotal = Ui::label("0", "BigNumber", stat);
    sl->addWidget(m_lblTotal);
    top->addWidget(stat, 0, Qt::AlignTop);
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
        d->setMinimumWidth(190);
    }
    m_comboTipo = new QComboBox(filters);
    m_comboTipo->addItem("Todos los eventos", "");
    m_comboTipo->setMinimumWidth(190);
    m_btnBuscar = Ui::button("Consultar", "primary", filters);

    fl->addWidget(Ui::field("Desde", m_desde, filters));
    fl->addWidget(Ui::field("Hasta", m_hasta, filters));
    fl->addWidget(Ui::field("Tipo de Evento:", m_comboTipo, filters));
    fl->addStretch();
    fl->addWidget(m_btnBuscar, 0, Qt::AlignBottom);
    layout->addWidget(filters);

    m_table = Ui::table({"Hora", "Día", "Empleado", "Rol en tienda", "Evento", "Descripción"}, 5, this);
    layout->addWidget(m_table, 1);

    m_pager = new PagerBar(10, this);
    layout->addWidget(m_pager);

    connect(m_btnBuscar, &QPushButton::clicked, this, &JefeBitacoraView::cargarMovimientos);
    connect(m_pager, &PagerBar::pageChanged, this, &JefeBitacoraView::renderPagina);
    connect(m_comboTipo, QOverload<int>::of(&QComboBox::activated), this, [this]() {
        const QString t = m_comboTipo->currentData().toString();
        m_filtrados.clear();
        for (const auto &m : std::as_const(m_todos))
            if (t.isEmpty() || m.accion == t) m_filtrados.append(m);
        m_pager->reset();
        renderPagina();
    });
}

void JefeBitacoraView::cargarMovimientos() {
    if (m_hasta->date() < m_desde->date()) {
        Toast::showToast(this, "La fecha final no puede ser anterior a la inicial", Toast::Warning);
        return;
    }
    const int dias = m_desde->date().daysTo(m_hasta->date()) + 1;
    if (dias > 31) {
        Toast::showToast(this, "El rango máximo es de 31 días", Toast::Warning);
        return;
    }

    // La API consulta un día por petición: se consulta cada día del rango y se unen los resultados.
    struct Acum { QList<MovimientoDiarioItem> items; int pendientes; bool fallo = false; };
    auto acum = QSharedPointer<Acum>::create();
    acum->pendientes = dias;
    m_btnBuscar->setEnabled(false);

    for (int i = 0; i < dias; ++i) {
        const QString fecha = m_desde->date().addDays(i).toString("yyyy-MM-dd");
        ApiClient::instance()->consultarMovimientoDiario(fecha, [this, acum](bool ok, const QJsonValue &data, const QString &msg) {
            if (ok && data.isObject()) acum->items += ResumenMovimientoDiario::fromJson(data.toObject()).movimientos;
            else if (!acum->fallo) { acum->fallo = true; Toast::showToast(this, "Error al cargar movimientos: " + msg, Toast::Error); }
            if (--acum->pendientes > 0) return;

            m_btnBuscar->setEnabled(true);
            m_todos = acum->items;
            std::sort(m_todos.begin(), m_todos.end(), [](const auto &a, const auto &b) { return a.fecha_hora > b.fecha_hora; });

            const QString actual = m_comboTipo->currentData().toString();
            m_comboTipo->clear();
            m_comboTipo->addItem("Todos los eventos", "");
            QStringList tipos;
            for (const auto &m : std::as_const(m_todos)) if (!tipos.contains(m.accion)) tipos << m.accion;
            tipos.sort();
            for (const QString &t : std::as_const(tipos)) m_comboTipo->addItem(t, t);
            const int idx = m_comboTipo->findData(actual);
            m_comboTipo->setCurrentIndex(idx >= 0 ? idx : 0);

            m_filtrados.clear();
            const QString t = m_comboTipo->currentData().toString();
            for (const auto &m : std::as_const(m_todos)) if (t.isEmpty() || m.accion == t) m_filtrados.append(m);
            m_pager->reset();
            renderPagina();
        });
    }
}

void JefeBitacoraView::renderPagina() {
    m_lblTotal->setText(QString::number(m_filtrados.size()));
    const int from = m_pager->page() * m_pager->pageSize();
    const int to = qMin(m_filtrados.size(), from + m_pager->pageSize());
    m_pager->update(m_filtrados.size(), "eventos registrados", to - from);

    m_table->clearContents();
    m_table->setRowCount(to - from);
    for (int i = from; i < to; ++i) {
        const auto &m = m_filtrados.at(i);
        const int r = i - from;
        const QString fh = QString(m.fecha_hora).replace('T', ' ');
        const QStringList partes = fh.split(' ', Qt::SkipEmptyParts);
        Ui::cellText(m_table, r, 0, partes.size() > 1 ? partes.at(1).left(8) : "—", true);
        Ui::cellText(m_table, r, 1, partes.value(0, "—"), true);

        auto *emp = new QWidget();
        auto *el = new QHBoxLayout(emp);
        el->setContentsMargins(16, 0, 16, 0);
        el->setSpacing(10);
        el->addWidget(Ui::avatar(m.usuario, "gray", emp));
        el->addWidget(new QLabel(m.usuario, emp));
        el->addStretch();
        m_table->setCellWidget(r, 2, emp);

        Ui::cellText(m_table, r, 3, m.rol, false, false, Theme::TextMuted);
        Ui::cellWidget(m_table, r, 4, Ui::chip(m.accion.toUpper(), chipPorEvento(m.accion)));
        Ui::cellText(m_table, r, 5, m.detalle, false, false, Theme::TextMuted);
    }
}
