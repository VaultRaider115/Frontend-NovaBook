#include "JefeBitacoraView.h"
#include "Theme.h"
#include "Ui.h"
#include "CatalogView.h"
#include "ApiClient.h"
#include "Toast.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDate>
#include <QSharedPointer>
#include <algorithm>

// Colores de categoría. VENTA, TRASLADO, COMPRA y ACCESO salen del diseño 7;
// el resto se eligió para no repetir esos tonos ni usar el rojo, que en la app
// indica errores y rechazos (salvo un acceso fallido, ver renderPagina).
struct CategoriaBitacora { const char *codigo; const char *etiqueta; const char *chip; };
static const CategoriaBitacora kCategorias[] = {
    {"ACCESO",     "ACCESO",     "neutral"},  // diseño: gris
    {"VENTA",      "VENTA",      "mint"},     // diseño: verde
    {"COMPRA",     "COMPRA",     "purple"},   // diseño: morado
    {"TRASLADO",   "TRASLADO",   "blue"},     // diseño: azul
    {"DEVOLUCION", "DEVOLUCIÓN", "amber"},
    {"INVENTARIO", "INVENTARIO", "rose"},
    {"USUARIOS",   "USUARIOS",   "gold"},
    {"CATALOGO",   "CATÁLOGO",   "indigo"},
    {"SISTEMA",    "SISTEMA",    "neutral"},
};

static const CategoriaBitacora &categoriaDe(const QString &codigo) {
    for (const auto &c : kCategorias) if (codigo == c.codigo) return c;
    return kCategorias[8];
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
        d->setMinimumWidth(235);
        d->setMaximumDate(QDate::currentDate());   // sin fechas futuras
        Ui::decorateDate(d);
    }
    m_desde->setToolTip("Desde");
    m_desde->setAccessibleName("Fecha inicial");
    m_hasta->setToolTip("Hasta");
    m_hasta->setAccessibleName("Fecha final");
    // Diseño 7: sin etiquetas; el combo muestra "Tipo de Evento:".
    m_comboTipo = new QComboBox(filters);
    m_comboTipo->addItem("Tipo de Evento: Todos", "");
    for (const auto &c : kCategorias) m_comboTipo->addItem(QString("Tipo de Evento: %1").arg(c.etiqueta), c.codigo);
    m_comboTipo->setMinimumWidth(260);
    m_comboTipo->setAccessibleName("Tipo de evento");
    m_btnBuscar = Ui::button("Consultar", "primary", filters, "search-dark");

    // "Hoy" es la única forma de volver al día actual: cambiar el tipo de evento,
    // la página o salir y volver a esta pantalla conserva el rango elegido.
    auto *btnHoy = Ui::button("Hoy", "outline", filters);
    btnHoy->setToolTip("Regresar el rango al día de hoy");

    fl->addWidget(m_desde);
    fl->addWidget(m_hasta);
    fl->addWidget(btnHoy);
    fl->addWidget(m_comboTipo);
    fl->addWidget(m_btnBuscar);
    fl->addStretch();
    layout->addWidget(filters);

    m_table = Ui::table({"Hora", "Día", "Empleado", "Rol en tienda", "Evento", "Descripción"}, 5, this);
    // Anchos para que no se corten nombres de empleados ni roles.
    auto *hh = m_table->horizontalHeader();
    hh->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(2, QHeaderView::Interactive);
    m_table->setColumnWidth(2, 220);
    hh->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(4, QHeaderView::Interactive);
    m_table->setColumnWidth(4, 185);
    hh->setSectionResizeMode(5, QHeaderView::Stretch);
    layout->addWidget(m_table, 1);

    m_pager = new PagerBar(5, this);
    layout->addWidget(m_pager);

    // Mantener el rango coherente: la fecha final nunca queda antes de la inicial.
    connect(m_desde, &QDateEdit::dateChanged, this, [this](const QDate &fecha) {
        if (m_hasta->date() < fecha) {
            QSignalBlocker b(m_hasta);
            m_hasta->setDate(fecha);
        }
    });
    connect(m_hasta, &QDateEdit::dateChanged, this, [this](const QDate &fecha) {
        if (fecha < m_desde->date()) {
            QSignalBlocker b(m_desde);
            m_desde->setDate(fecha);
        }
    });

    connect(m_btnBuscar, &QPushButton::clicked, this, [this]() { m_pager->reset(); cargarMovimientos(); });
    connect(btnHoy, &QPushButton::clicked, this, [this]() {
        const QDate hoy = QDate::currentDate();
        QSignalBlocker b1(m_desde), b2(m_hasta);
        m_desde->setDate(hoy);
        m_hasta->setDate(hoy);
        m_pager->reset();
        cargarMovimientos();
    });
    connect(m_comboTipo, QOverload<int>::of(&QComboBox::activated), this, [this]() { m_pager->reset(); cargarMovimientos(); });
    connect(m_pager, &PagerBar::pageChanged, this, &JefeBitacoraView::cargarMovimientos);
}

void JefeBitacoraView::cargarMovimientos() {
    if (m_cargando) return;

    // Si la app quedó abierta de un día para otro, el máximo avanza con la fecha.
    const QDate hoy = QDate::currentDate();
    m_desde->setMaximumDate(hoy);
    m_hasta->setMaximumDate(hoy);

    if (m_hasta->date() < m_desde->date()) {
        Toast::showToast(this, "La fecha final no puede ser anterior a la inicial", Toast::Warning);
        return;
    }
    m_cargando = true;
    m_btnBuscar->setEnabled(false);
    ApiClient::instance()->consultarBitacora(
        m_desde->date().toString("yyyy-MM-dd"), m_hasta->date().toString("yyyy-MM-dd"),
        m_comboTipo->currentData().toString(), m_pager->page(), m_pager->pageSize(),
        [this](bool ok, const QJsonValue &data, const QString &msg) {
            m_cargando = false;
            m_btnBuscar->setEnabled(true);
            if (!ok || !data.isObject()) {
                Toast::showToast(this, "Error al cargar la bitácora: " + msg, Toast::Error);
                return;
            }
            const QJsonObject o = data.toObject();
            m_total = o.value("total").toInt();
            m_pagina.clear();
            const QJsonArray arr = o.value("items").toArray();
            for (const auto &v : arr) m_pagina.append(MovimientoDiarioItem::fromJson(v.toObject()));
            renderPagina();
        });
}

void JefeBitacoraView::renderPagina() {
    m_lblTotal->setText(QString::number(m_total));
    m_pager->update(m_total, "eventos registrados", m_pagina.size());

    m_table->clearContents();
    m_table->setRowCount(m_pagina.size());
    for (int r = 0; r < m_pagina.size(); ++r) {
        const auto &m = m_pagina.at(r);
        const QString fh = QString(m.fecha_hora).replace('T', ' ');
        const QStringList partes = fh.split(' ', Qt::SkipEmptyParts);
        Ui::cellText(m_table, r, 0, partes.size() > 1 ? partes.at(1).left(8) : "—", true);
        // Día como en el diseño: 14/10/26
        const QDate dia = QDate::fromString(partes.value(0), "yyyy-MM-dd");
        Ui::cellText(m_table, r, 1, dia.isValid() ? dia.toString("dd/MM/yy") : partes.value(0, "—"), true);

        auto *emp = new QWidget();
        auto *el = new QHBoxLayout(emp);
        el->setContentsMargins(0, 0, 0, 0);
        el->setSpacing(10);
        el->addWidget(Ui::avatar(m.usuario, "gold", emp));
        el->addWidget(new QLabel(m.usuario, emp));
        el->addStretch();
        m_table->setCellWidget(r, 2, emp);

        Ui::cellText(m_table, r, 3, m.rol, false, false, Theme::TextMuted);
        const auto &cat = categoriaDe(m.categoria);
        // Un acceso fallido se resalta en rojo: es relevante para la seguridad.
        const bool fallido = m.accion.contains("FALLIDO", Qt::CaseInsensitive);
        auto *chip = Ui::chip(cat.etiqueta, fallido ? "danger" : cat.chip);
        chip->setToolTip(m.accion);
        Ui::cellWidget(m_table, r, 4, chip);
        Ui::cellText(m_table, r, 5, m.detalle.isEmpty() ? m.accion : m.detalle, false, false, Theme::TextMuted);
    }
}
