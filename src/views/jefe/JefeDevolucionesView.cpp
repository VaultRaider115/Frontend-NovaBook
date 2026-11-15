#include "JefeDevolucionesView.h"
#include <QInputDialog>
#include "Theme.h"
#include "Ui.h"
#include "ApiClient.h"
#include "Toast.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>

JefeDevolucionesView::JefeDevolucionesView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(24);

    auto *top = new QHBoxLayout();
    top->addWidget(Ui::pageHeader("Jefatura · Control de merma y devoluciones",
                                  "Historial y Aprobación de Devoluciones a Proveedor",
                                  "Revisa, autoriza o rechaza los lotes de devolución generados.", this), 1);
    auto *stat = Ui::card(this);
    auto *sl = new QVBoxLayout(stat);
    sl->setContentsMargins(20, 12, 20, 12);
    sl->setSpacing(2);
    sl->addWidget(Ui::label("Devoluciones listadas", "Muted", stat));
    m_lblTotal = Ui::label("0", "BigNumber", stat);
    sl->addWidget(m_lblTotal);
    top->addWidget(stat, 0, Qt::AlignTop);
    layout->addLayout(top);

    auto *filters = Ui::card(this);
    auto *fl = new QHBoxLayout(filters);
    fl->setContentsMargins(16, 16, 16, 16);
    fl->setSpacing(12);

    m_fecha = new QDateEdit(QDate::currentDate(), filters);
    m_fecha->setCalendarPopup(true);
    m_fecha->setDisplayFormat("dd 'de' MMMM, yyyy");
    m_fecha->setMinimumWidth(200);
    m_btnHoy = Ui::button("Hoy", "outline", filters);
    m_chkTodasFechas = new QCheckBox("Todas las fechas", filters);
    m_comboEstado = new QComboBox(filters);
    m_comboEstado->addItem("Todos", "Todos");
    m_comboEstado->addItem("Pendiente", "PENDIENTE");
    m_comboEstado->addItem("Aprobado", "APROBADO");
    m_comboEstado->addItem("Rechazado", "RECHAZADO");
    m_comboEstado->setMinimumWidth(160);
    m_btnBuscar = Ui::button("Consultar", "primary", filters);

    fl->addWidget(Ui::field("Fecha", m_fecha, filters));
    fl->addWidget(m_btnHoy, 0, Qt::AlignBottom);
    fl->addWidget(m_chkTodasFechas, 0, Qt::AlignBottom);
    fl->addWidget(Ui::field("Estado:", m_comboEstado, filters));
    fl->addStretch();
    fl->addWidget(m_btnBuscar, 0, Qt::AlignBottom);
    layout->addWidget(filters);

    m_table = Ui::table({"Folio / Lote", "Proveedor", "Solicitante", "Piezas", "Motivo principal", "Estado", "Acciones"}, 1, this, 64);
    layout->addWidget(m_table, 1);

    connect(m_btnBuscar, &QPushButton::clicked, this, &JefeDevolucionesView::cargarDevoluciones);
    connect(m_comboEstado, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &JefeDevolucionesView::cargarDevoluciones);
    connect(m_chkTodasFechas, &QCheckBox::toggled, m_fecha, &QWidget::setDisabled);
    connect(m_btnHoy, &QPushButton::clicked, this, [this]() {
        m_chkTodasFechas->setChecked(false);
        m_fecha->setDate(QDate::currentDate());
        cargarDevoluciones();
    });
}

void JefeDevolucionesView::cargarDevoluciones() {
    const QString fecha = m_chkTodasFechas->isChecked() ? QString() : m_fecha->date().toString("yyyy-MM-dd");
    const QString estado = m_comboEstado->currentData().toString();

    ApiClient::instance()->consultarHistorialDevoluciones(fecha, estado, 0, [this](bool ok, const QJsonValue &data, const QString &msg) {
        if (!ok || !data.isArray()) {
            Toast::showToast(this, "Error al consultar historial: " + msg, Toast::Error);
            return;
        }
        m_devoluciones.clear();
        const QJsonArray arr = data.toArray();
        for (const auto &v : arr) m_devoluciones.append(Devolucion::fromJson(v.toObject()));

        m_lblTotal->setText(QString::number(m_devoluciones.size()));
        m_table->clearContents();
        m_table->setRowCount(m_devoluciones.size());

        for (int i = 0; i < m_devoluciones.size(); ++i) {
            const Devolucion &d = m_devoluciones.at(i);
            Ui::cellText(m_table, i, 0, "#" + (d.folio.isEmpty() ? QString("DEV-%1").arg(d.id_devolucion) : d.folio), true);
            Ui::cellText(m_table, i, 1, d.nombre_proveedor.isEmpty() ? QString("Proveedor #%1").arg(d.id_proveedor) : d.nombre_proveedor, false, true);
            Ui::cellText(m_table, i, 2, d.vendedor_nombre.isEmpty() ? "—"
                         : (d.solicitante_rol.isEmpty() ? d.vendedor_nombre : QString("%1 · %2").arg(d.vendedor_nombre, d.solicitante_rol)));
            Ui::cellText(m_table, i, 3, QString("%1 %2").arg(d.total_piezas).arg(d.total_piezas == 1 ? "pieza" : "piezas"), true, true, Theme::TextPrimary);
            Ui::cellText(m_table, i, 4, d.items.isEmpty() || d.items.first().motivo.isEmpty() ? "—" : d.items.first().motivo, false, false, Theme::TextMuted);

            const QString est = d.estado.toLower();
            const bool autorizado = est == "autorizado" || est == "aprobado";
            const bool rechazado = est == "rechazado";
            Ui::cellWidget(m_table, i, 5, Ui::chip(autorizado ? "Aprobado" : (rechazado ? "Rechazado" : "Pendiente"),
                                                   autorizado ? "ok" : (rechazado ? "danger" : "pending")));

            if (autorizado) {
                auto *pdf = Ui::button("Ver Acta PDF", "outline", nullptr, "pdf");
                pdf->setProperty("variant", "small");
                connect(pdf, &QPushButton::clicked, this, [this, d]() { verActaPdf(d); });
                Ui::cellWidget(m_table, i, 6, pdf);
            } else if (rechazado) {
                Ui::cellWidget(m_table, i, 6, Ui::label("Sin acciones", "Muted"));
            } else {
                auto *ok = Ui::button("Aprobar", "approve");
                auto *no = Ui::button("Rechazar", "reject");
                ok->setAccessibleName(QString("Aprobar devolución %1").arg(d.id_devolucion));
                no->setAccessibleName(QString("Rechazar devolución %1").arg(d.id_devolucion));
                connect(ok, &QPushButton::clicked, this, [this, d]() { evaluar(d, true); });
                connect(no, &QPushButton::clicked, this, [this, d]() { evaluar(d, false); });
                Ui::cellWidgets(m_table, i, 6, {ok, no});
            }
        }
    });
}

void JefeDevolucionesView::evaluar(const Devolucion &dev, bool aprobar) {
    const QString verbo = aprobar ? "aprobar" : "rechazar";
    if (QMessageBox::question(this, "Confirmar",
            QString("¿Deseas %1 el lote #DEV-%2 de %3 (%4 piezas)?").arg(verbo).arg(dev.id_devolucion)
                .arg(dev.nombre_proveedor).arg(dev.total_piezas),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

    ApiClient::instance()->aprobarDevolucion(dev.id_devolucion, aprobar, QString(),
        [this, aprobar](bool ok, const QJsonValue &, const QString &msg) {
            if (!ok) {
                Toast::showToast(this, QString("No se pudo %1: %2").arg(aprobar ? "aprobar" : "rechazar", msg), Toast::Error);
                return;
            }
            Toast::showToast(this, aprobar ? "Devolución aprobada exitosamente" : "Devolución rechazada",
                             aprobar ? Toast::Success : Toast::Warning);
            cargarDevoluciones();
        });
}

void JefeDevolucionesView::verActaPdf(const Devolucion &dev) {
    const QString tipo = dev.tipo_producto.isEmpty() ? "libro" : dev.tipo_producto;
    const QString savePath = QFileDialog::getSaveFileName(this, "Guardar Acta de Devolución en PDF",
        QString("acta_devolucion_%1.pdf").arg(dev.id_devolucion), "Archivos PDF (*.pdf)");
    if (savePath.isEmpty()) return;

    ApiClient::instance()->descargarPdfDevolucion(dev.id_devolucion, tipo, [this, savePath](bool ok, const QByteArray &bytes, const QString &msg) {
        QFile f(savePath);
        if (!ok || bytes.isEmpty() || !f.open(QIODevice::WriteOnly)) {
            Toast::showToast(this, "No se pudo guardar el acta: " + msg, Toast::Error);
            return;
        }
        f.write(bytes);
        Toast::showToast(this, "Acta PDF guardada exitosamente", Toast::Success);
    });
}
