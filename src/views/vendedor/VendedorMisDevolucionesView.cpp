#include "VendedorMisDevolucionesView.h"
#include "ApiClient.h"
#include "Documentos.h"
#include "Theme.h"
#include "Toast.h"
#include "Ui.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QVBoxLayout>

VendedorMisDevolucionesView::VendedorMisDevolucionesView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(24);

    auto *top = new QHBoxLayout();
    top->addWidget(Ui::pageHeader("Vendedor · Seguimiento", "Mis Devoluciones a Proveedor",
                                  "Consulta si el Jefe aprobó tus solicitudes y descarga el acta para entregarla a Bodega.",
                                  this), 1);
    auto *stat = Ui::card(this);
    auto *sl = new QVBoxLayout(stat);
    sl->setContentsMargins(20, 12, 20, 12);
    sl->setSpacing(2);
    sl->addWidget(Ui::label("Pendientes de revisión", "Muted", stat));
    m_lblPendientes = Ui::label("0", "BigNumber", stat);
    sl->addWidget(m_lblPendientes);
    top->addWidget(stat, 0, Qt::AlignTop);
    layout->addLayout(top);

    auto *filters = Ui::card(this);
    auto *fl = new QHBoxLayout(filters);
    fl->setContentsMargins(16, 16, 16, 16);
    fl->setSpacing(12);
    m_comboEstado = new QComboBox(filters);
    m_comboEstado->addItem("Estado: Todos", "Todos");
    m_comboEstado->addItem("Estado: Pendiente", "PENDIENTE");
    m_comboEstado->addItem("Estado: Aprobado", "APROBADO");
    m_comboEstado->addItem("Estado: Rechazado", "RECHAZADO");
    m_comboEstado->setMinimumWidth(220);
    m_comboEstado->setAccessibleName("Estado");
    m_btnBuscar = Ui::button("Actualizar", "primary", filters, "refresh");
    fl->addWidget(m_comboEstado);
    fl->addWidget(m_btnBuscar);
    fl->addStretch();
    layout->addWidget(filters);

    m_table = Ui::table({"Folio / Lote", "Fecha", "Proveedor", "Piezas", "Motivo principal", "Estado", "Acciones"}, 2, this, 64);
    m_table->setWordWrap(true);
    auto *hh = m_table->horizontalHeader();
    hh->setSectionResizeMode(6, QHeaderView::Interactive);
    m_table->setColumnWidth(6, 300);
    layout->addWidget(m_table, 1);

    connect(m_btnBuscar, &QPushButton::clicked, this, &VendedorMisDevolucionesView::cargar);
    connect(m_comboEstado, QOverload<int>::of(&QComboBox::activated), this, &VendedorMisDevolucionesView::cargar);
}

void VendedorMisDevolucionesView::cargar() {
    m_btnBuscar->setEnabled(false);
    ApiClient::instance()->consultarMisDevoluciones(m_comboEstado->currentData().toString(),
        [this](bool ok, const QJsonValue &data, const QString &msg) {
            m_btnBuscar->setEnabled(true);
            if (!ok || !data.isArray()) {
                Toast::showToast(this, "Error al consultar tus devoluciones: " + msg, Toast::Error);
                return;
            }
            QList<Devolucion> lista;
            for (const auto &v : data.toArray()) lista.append(Devolucion::fromJson(v.toObject()));

            int pendientes = 0;
            m_table->clearContents();
            m_table->setRowCount(lista.size());
            for (int i = 0; i < lista.size(); ++i) {
                const Devolucion &d = lista.at(i);
                const QString est = d.estado.toLower();
                const bool aprobado = est == "aprobado" || est == "autorizado" || est == "aceptado";
                const bool rechazado = est == "rechazado";
                if (!aprobado && !rechazado) ++pendientes;

                Ui::cellText(m_table, i, 0, "#" + (d.folio.isEmpty() ? QString("DEV-%1").arg(d.id_devolucion) : d.folio), true);
                Ui::cellText(m_table, i, 1, d.fecha.isEmpty() ? "—" : d.fecha, true, false, Theme::TextMuted);
                Ui::cellText(m_table, i, 2, d.nombre_proveedor.isEmpty() ? QString("Proveedor #%1").arg(d.id_proveedor) : d.nombre_proveedor, false, true);
                auto *pzs = new QLabel(QString("<b>%1</b> <span style=\"color:%2;\">%3</span>")
                    .arg(d.total_piezas).arg(Theme::TextMuted, d.total_piezas == 1 ? "pieza" : "piezas"));
                pzs->setTextFormat(Qt::RichText);
                Ui::cellWidget(m_table, i, 3, pzs);
                Ui::cellText(m_table, i, 4, d.items.isEmpty() || d.items.first().motivo.isEmpty() ? "—" : d.items.first().motivo,
                             false, false, Theme::TextMuted);
                Ui::cellWidget(m_table, i, 5, Ui::chip(aprobado ? "Aprobado" : (rechazado ? "Rechazado" : "Pendiente"),
                                                       aprobado ? "ok" : (rechazado ? "danger" : "pending")));

                if (aprobado) {
                    auto *pdf = Ui::button("Ver Acta PDF", "outline", nullptr, "pdf");
                    pdf->setProperty("variant", "small");
                    pdf->setAccessibleName(QString("Ver acta de %1").arg(d.folio));
                    if (!d.observaciones.trimmed().isEmpty())
                        pdf->setToolTip("Nota del Jefe: " + d.observaciones.trimmed());
                    connect(pdf, &QPushButton::clicked, this, [this, d]() { verActa(d); });
                    Ui::cellWidget(m_table, i, 6, pdf);
                } else if (rechazado) {
                    // Diagrama, paso 9: el vendedor rehace la devolución con base en el motivo.
                    const QString motivo = d.observaciones.trimmed();
                    auto *lbl = new QLabel(motivo.isEmpty() ? "Rechazada (sin observaciones)"
                                                            : QString("Motivo: %1").arg(motivo));
                    lbl->setObjectName("Muted");
                    lbl->setWordWrap(true);
                    lbl->setToolTip(motivo);
                    m_table->setCellWidget(i, 6, lbl);
                } else {
                    Ui::cellWidget(m_table, i, 6, Ui::label("En revisión del Jefe", "Muted"));
                }
            }
            m_lblPendientes->setText(QString::number(pendientes));
            if (lista.isEmpty())
                Toast::showToast(this, "No tienes solicitudes con ese estado", Toast::Info);
        });
}

void VendedorMisDevolucionesView::verActa(const Devolucion &d) {
    const QString tipo = d.tipo_producto.isEmpty() ? "libro" : d.tipo_producto;
    const QString folio = d.folio.isEmpty() ? QString("DEV-%1").arg(d.id_devolucion) : d.folio;
    ApiClient::instance()->descargarPdfDevolucion(d.id_devolucion, tipo, [this, folio](bool ok, const QByteArray &bytes, const QString &msg) {
        if (!ok) {
            Toast::showToast(this, "No se pudo obtener el acta: " + msg, Toast::Error);
            return;
        }
        Documentos::guardarYAbrir(this, bytes, QString("acta_%1.pdf").arg(folio));
    });
}
