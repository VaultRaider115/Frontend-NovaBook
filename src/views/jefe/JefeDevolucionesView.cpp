#include "JefeDevolucionesView.h"
#include <QInputDialog>
#include "Theme.h"
#include "Ui.h"
#include "ApiClient.h"
#include "Toast.h"
#include "Documentos.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCalendarWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>

JefeDevolucionesView::JefeDevolucionesView(bool puedeAprobar, QWidget *parent)
    : QWidget(parent), m_puedeAprobar(puedeAprobar)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(24);

    auto *top = new QHBoxLayout();
    if (puedeAprobar)
        top->addWidget(Ui::pageHeader("Jefatura · Control de merma y devoluciones",
                                      "Historial y Aprobación de Devoluciones a Proveedor",
                                      "Revisa, autoriza o rechaza los lotes de devolución generados.", this), 1);
    else
        top->addWidget(Ui::pageHeader("Gerencia · Control de devoluciones",
                                      "Historial de Devoluciones a Proveedor",
                                      "Consulta los lotes de devolución y descarga las actas aprobadas. "
                                      "La autorización corresponde al Jefe de Departamento.", this), 1);
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

    const QDate hoy = QDate::currentDate();
    m_fecha = new QDateEdit(hoy, filters);
    m_fecha->setCalendarPopup(true);
    m_fecha->setDisplayFormat("dd 'de' MMMM, yyyy");
    m_fecha->setMinimumWidth(235);
    m_fecha->setMaximumDate(hoy);
    if (auto *le = m_fecha->findChild<QLineEdit *>()) le->setReadOnly(true);
    if (auto *cal = m_fecha->calendarWidget()) {
        cal->setMinimumSize(330, 270);
        cal->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    }

    m_btnHoy = Ui::button("Hoy", "outline", filters);
    m_chkTodasFechas = new QCheckBox("Todas las fechas", filters);
    m_comboEstado = new QComboBox(filters);
    // Diseño 20: sin etiquetas; el combo muestra "Estado:".
    m_comboEstado->addItem("Estado: Todos", "Todos");
    m_comboEstado->addItem("Estado: Pendiente", "PENDIENTE");
    m_comboEstado->addItem("Estado: Aprobado", "APROBADO");
    m_comboEstado->addItem("Estado: Rechazado", "RECHAZADO");
    m_comboEstado->setMinimumWidth(200);
    m_comboEstado->setAccessibleName("Estado");
    m_btnBuscar = Ui::button("Consultar", "primary", filters, "search-dark");
    Ui::decorateDate(m_fecha);
    m_fecha->setAccessibleName("Fecha");

    fl->addWidget(m_fecha);
    fl->addWidget(m_btnHoy);
    fl->addWidget(m_chkTodasFechas);
    fl->addWidget(Ui::divider(filters, true));
    fl->addWidget(m_comboEstado);
    fl->addWidget(m_btnBuscar);
    fl->addStretch();
    layout->addWidget(filters);

    m_table = Ui::table({"Folio / Lote", "Fecha", "Proveedor", "Solicitante", "Piezas", "Motivo principal", "Estado", "Acciones"}, 2, this, 64);
    auto *hh = m_table->horizontalHeader();
    hh->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(2, QHeaderView::Stretch);
    hh->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    hh->setSectionResizeMode(5, QHeaderView::Interactive);
    m_table->setColumnWidth(5, 170);
    hh->setSectionResizeMode(6, QHeaderView::Interactive);
    m_table->setColumnWidth(6, 120);
    hh->setSectionResizeMode(7, QHeaderView::Interactive);
    m_table->setColumnWidth(7, 265);
    layout->addWidget(m_table, 1);

    connect(m_btnBuscar, &QPushButton::clicked, this, &JefeDevolucionesView::cargarDevoluciones);
    connect(m_comboEstado, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &JefeDevolucionesView::cargarDevoluciones);
    connect(m_fecha, &QDateEdit::dateChanged, this, [this]() {
        if (!m_chkTodasFechas->isChecked()) cargarDevoluciones();
    });
    connect(m_chkTodasFechas, &QCheckBox::toggled, this, [this](bool todas) {
        m_fecha->setDisabled(todas);
        cargarDevoluciones();
    });
    connect(m_btnHoy, &QPushButton::clicked, this, [this]() {
        m_chkTodasFechas->blockSignals(true);
        m_chkTodasFechas->setChecked(false);
        m_chkTodasFechas->blockSignals(false);
        m_fecha->setDisabled(false);
        m_fecha->blockSignals(true);
        m_fecha->setDate(QDate::currentDate());
        m_fecha->blockSignals(false);
        cargarDevoluciones();
    });
}

void JefeDevolucionesView::cargarDevoluciones() {
    const QDate hoy = QDate::currentDate();
    m_fecha->setMaximumDate(hoy);
    if (!m_chkTodasFechas->isChecked() && m_fecha->date() > hoy) {
        Toast::showToast(this, "No se pueden consultar fechas futuras", Toast::Warning);
        return;
    }

    const QString fecha = m_chkTodasFechas->isChecked() ? QString() : m_fecha->date().toString("yyyy-MM-dd");
    const QString estado = m_comboEstado->currentData().toString();
    m_btnBuscar->setEnabled(false);

    ApiClient::instance()->consultarHistorialDevoluciones(fecha, estado, 0, [this](bool ok, const QJsonValue &data, const QString &msg) {
        m_btnBuscar->setEnabled(true);
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
            Ui::cellText(m_table, i, 1, d.fecha.isEmpty() ? "—" : d.fecha, true, false, Theme::TextMuted);
            Ui::cellText(m_table, i, 2, d.nombre_proveedor.isEmpty() ? QString("Proveedor #%1").arg(d.id_proveedor) : d.nombre_proveedor, false, true);
            // Diseño 20: nombre y, debajo, el rol en pequeño.
            auto *sol = new QLabel(d.vendedor_nombre.isEmpty() ? "—"
                : QString("%1<br><span style=\"color:%2; font-size:11px;\">%3</span>")
                      .arg(d.vendedor_nombre.toHtmlEscaped(), Theme::TextMuted, d.solicitante_rol.toHtmlEscaped()));
            sol->setTextFormat(Qt::RichText);
            Ui::cellWidget(m_table, i, 3, sol);
            auto *pzs = new QLabel(QString("<b>%1</b> <span style=\"color:%2;\">%3</span>")
                .arg(d.total_piezas).arg(Theme::TextMuted, d.total_piezas == 1 ? "pieza" : "piezas"));
            pzs->setTextFormat(Qt::RichText);
            Ui::cellWidget(m_table, i, 4, pzs);
            Ui::cellText(m_table, i, 5, d.items.isEmpty() || d.items.first().motivo.isEmpty() ? "—" : d.items.first().motivo, false, false, Theme::TextMuted);

            const QString est = d.estado.toLower();
            const bool autorizado = est == "autorizado" || est == "aprobado" || est == "aceptado";
            const bool rechazado = est == "rechazado";
            Ui::cellWidget(m_table, i, 6, Ui::chip(autorizado ? "Aprobado" : (rechazado ? "Rechazado" : "Pendiente"),
                                                   autorizado ? "ok" : (rechazado ? "danger" : "pending")));

            // Qué libros/revistas van en el lote, antes de aprobar o rechazar.
            auto *ver = Ui::iconButton("eye", "Ver qué se devuelve");
            ver->setAccessibleName(QString("Ver detalle de la devolución %1").arg(d.id_devolucion));
            connect(ver, &QPushButton::clicked, this, [this, d]() { mostrarDetalle(d, QString()); });

            if (autorizado) {
                auto *pdf = Ui::button("Ver Acta PDF", "outline", nullptr, "pdf");
                pdf->setProperty("variant", "small");
                connect(pdf, &QPushButton::clicked, this, [this, d]() { verActaPdf(d); });
                Ui::cellWidgets(m_table, i, 7, {ver, pdf});
            } else if (rechazado) {
                const QString motivo = d.observaciones.trimmed();
                if (m_puedeAprobar) {
                    // Diseño 20: "Sin acciones"; el motivo queda como tooltip.
                    auto *lbl = Ui::label("Sin acciones", "Muted");
                    lbl->setToolTip(motivo.isEmpty() ? QString() : "Motivo del rechazo: " + motivo);
                    Ui::cellWidgets(m_table, i, 7, {ver, lbl});
                } else {
                    auto *lbl = new QLabel(motivo.isEmpty() ? "Rechazada" : "Motivo: " + motivo);
                    lbl->setObjectName("Muted");
                    lbl->setWordWrap(true);
                    lbl->setToolTip(motivo);
                    Ui::cellWidgets(m_table, i, 7, {ver, lbl});
                }
            } else if (!m_puedeAprobar) {
                Ui::cellWidgets(m_table, i, 7, {ver, Ui::label("En revisión del Jefe", "Muted")});
            } else {
                auto *ok = Ui::button("Aprobar", "approve", nullptr, "check");
                auto *no = Ui::button("Rechazar", "reject", nullptr, "x");
                ok->setAccessibleName(QString("Aprobar devolución %1").arg(d.id_devolucion));
                no->setAccessibleName(QString("Rechazar devolución %1").arg(d.id_devolucion));
                connect(ok, &QPushButton::clicked, this, [this, d]() { evaluar(d, true); });
                connect(no, &QPushButton::clicked, this, [this, d]() { evaluar(d, false); });
                Ui::cellWidgets(m_table, i, 7, {ver, ok, no});
            }
        }
    });
}

void JefeDevolucionesView::evaluar(const Devolucion &dev, bool aprobar) {
    // Se confirma viendo los títulos del lote, no solo el total de piezas.
    if (!mostrarDetalle(dev, aprobar ? "Aprobar devolución" : "Rechazar devolución")) return;

    // Regla de negocio: el rechazo exige observaciones (devolucion.observaciones).
    QString notas;
    if (!aprobar) {
        bool aceptado = false;
        notas = QInputDialog::getMultiLineText(this, "Rechazar devolución",
            "Observaciones del rechazo (obligatorias):", QString(), &aceptado).trimmed();
        if (!aceptado) return;
        if (notas.isEmpty()) {
            Toast::showToast(this, "Escribe las observaciones para poder rechazar", Toast::Warning);
            return;
        }
        if (notas.size() > 200) {   // mismo límite que la API (se guardan en la bitácora)
            Toast::showToast(this, QString("Las observaciones no pueden exceder 200 caracteres (tienen %1)").arg(notas.size()),
                             Toast::Warning);
            return;
        }
    }

    m_table->setEnabled(false);
    ApiClient::instance()->aprobarDevolucion(dev.id_devolucion, aprobar, notas,
        [this, aprobar](bool ok, const QJsonValue &, const QString &msg) {
            m_table->setEnabled(true);
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
    const QString folio = dev.folio.isEmpty() ? QString("DEV-%1").arg(dev.id_devolucion) : dev.folio;
    ApiClient::instance()->descargarPdfDevolucion(dev.id_devolucion, tipo, [this, folio](bool ok, const QByteArray &bytes, const QString &msg) {
        if (!ok) {
            Toast::showToast(this, "No se pudo obtener el acta: " + msg, Toast::Error);
            return;
        }
        // Se guarda y se abre en el visor del sistema para imprimirla.
        Documentos::guardarYAbrir(this, bytes, QString("acta_%1.pdf").arg(folio));
    });
}
// Lista de lo que se devuelve. Con okText vacío es solo consulta (botón Cerrar);
// si no, es la confirmación de aprobar/rechazar y devuelve si se aceptó.
bool JefeDevolucionesView::mostrarDetalle(const Devolucion &dev, const QString &okText) {
    const bool consulta = okText.isEmpty();
    const QString folio = dev.folio.isEmpty() ? QString("DEV-%1").arg(dev.id_devolucion) : dev.folio;
    Ui::FormDialog dlg(consulta ? QString("Devolución #%1").arg(folio) : QString("Confirmar: %1").arg(okText.toLower()),
                       consulta ? "Cerrar" : okText, this, 780);
    if (consulta) {
        for (QPushButton *b : dlg.findChildren<QPushButton *>())
            if (b != dlg.okButton() && b->text() == "Cancelar") b->hide();
    } else if (okText.startsWith("Rechazar")) {
        dlg.okButton()->setProperty("variant", "reject");
        dlg.okButton()->setIcon(QIcon(":/icons/x.svg"));
        Ui::repolish(dlg.okButton());
    }

    int piezas = 0;
    for (const ItemDevolucion &it : dev.items) piezas += it.cantidad;
    auto *resumen = new QLabel(QString("<b>%1</b> · %2<br><span style=\"color:%3;\">Solicitó %4 el %5 · %6 título(s), %7 pieza(s)</span>")
        .arg(dev.nombre_proveedor.toHtmlEscaped(), dev.tipo_producto.compare("revista", Qt::CaseInsensitive) == 0 ? "Revistas" : "Libros", Theme::TextMuted,
             dev.vendedor_nombre.isEmpty() ? "—" : dev.vendedor_nombre.toHtmlEscaped(), dev.fecha.isEmpty() ? "—" : dev.fecha)
        .arg(dev.items.size()).arg(piezas), &dlg);
    resumen->setTextFormat(Qt::RichText);
    resumen->setWordWrap(true);

    auto *tabla = Ui::table({"EAN", "Título", "Piezas", "Motivo"}, 1, &dlg, 44);
    tabla->setRowCount(dev.items.size());
    for (int i = 0; i < dev.items.size(); ++i) {
        const ItemDevolucion &it = dev.items.at(i);
        Ui::cellText(tabla, i, 0, QString::number(it.codigo_ean), true);
        Ui::cellText(tabla, i, 1, it.titulo.isEmpty() ? "—" : it.titulo, false, true);
        Ui::cellText(tabla, i, 2, QString::number(it.cantidad));
        Ui::cellText(tabla, i, 3, it.motivo.isEmpty() ? "—" : it.motivo, false, false, Theme::TextMuted);
    }
    tabla->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tabla->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tabla->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    tabla->setMinimumHeight(qMin(360, 56 + 44 * qMax(1, static_cast<int>(dev.items.size()))));

    dlg.addField("Lote", resumen, 0, 0, 2);
    dlg.addField("Libros / revistas que se devuelven", tabla, 1, 0, 2);
    if (!dev.observaciones.trimmed().isEmpty()) {
        auto *obs = new QLabel(dev.observaciones.toHtmlEscaped(), &dlg);
        obs->setWordWrap(true);
        dlg.addField("Observaciones del Jefe", obs, 2, 0, 2);
    }
    connect(dlg.okButton(), &QPushButton::clicked, &dlg, &QDialog::accept);
    return dlg.exec() == QDialog::Accepted && !consulta;
}
