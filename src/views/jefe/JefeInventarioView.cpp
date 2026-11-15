#include "JefeInventarioView.h"
#include "ApiClient.h"
#include "Models.h"
#include "Theme.h"
#include "Toast.h"
#include "Ui.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QVBoxLayout>

namespace {
const char *chipEstado(const QString &e) {
    if (e == "APROBADO") return "ok";
    if (e == "RECHAZADO") return "danger";
    if (e == "CERRADO") return "pending";
    return "blue";   // ABIERTO
}
QString etiquetaEstado(const QString &e) {
    if (e == "CERRADO") return "Por revisar";
    if (e == "ABIERTO") return "Contando";
    return e.left(1) + e.mid(1).toLower();
}
}

JefeInventarioView::JefeInventarioView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(18);
    layout->addWidget(Ui::pageHeader("Jefatura · Fin de mes", "Inventario Mensual",
        "Revisa el conteo del piso de venta, aprueba el ajuste de diferencias y registra lo que pidieron las editoriales.", this));

    auto *seg = new QHBoxLayout();
    seg->setSpacing(6);
    for (const QString &n : {QString("Inventarios"), QString("Solicitudes de editorial")}) {
        auto *b = Ui::button(n, "page", this);
        const int idx = m_segmentos.size();
        connect(b, &QPushButton::clicked, this, [this, idx]() { mostrarSeccion(idx); });
        m_segmentos << b;
        seg->addWidget(b);
    }
    seg->addStretch();
    layout->addLayout(seg);

    m_stack = new QStackedWidget(this);
    layout->addWidget(m_stack, 1);

    // --- Inventarios ---------------------------------------------------------
    auto *inv = new QWidget(m_stack);
    auto *il = new QVBoxLayout(inv);
    il->setContentsMargins(0, 0, 0, 0);
    il->setSpacing(14);
    m_tablaInv = Ui::table({"Folio", "Inicio", "Cierre", "Contó", "Estado", "Productos", "Con diferencia", "A devolución"}, 3, inv, 48);
    m_tablaInv->setMaximumHeight(230);
    il->addWidget(m_tablaInv);

    auto *detHead = new QHBoxLayout();
    m_lblDetalle = new QLabel("Elige un inventario para ver su detalle.", inv);
    m_lblDetalle->setObjectName("Muted");
    m_lblDetalle->setWordWrap(true);
    detHead->addWidget(m_lblDetalle, 1);
    m_btnRechazar = Ui::button("Rechazar", "reject", inv, "x");
    m_btnAprobar = Ui::button("Aprobar y ajustar piso", "approve", inv, "check");
    detHead->addWidget(m_btnRechazar);
    detHead->addWidget(m_btnAprobar);
    il->addLayout(detHead);

    m_tablaDet = Ui::table({"EAN", "Título", "Editorial", "Sistema", "Contadas", "Diferencia", "Apartadas", "A devolución"}, 1, inv, 48);
    il->addWidget(m_tablaDet, 1);
    m_stack->addWidget(inv);

    // --- Solicitudes de editorial ------------------------------------------------
    auto *sol = new QWidget(m_stack);
    auto *sol_l = new QVBoxLayout(sol);
    sol_l->setContentsMargins(0, 0, 0, 0);
    sol_l->setSpacing(14);
    auto *solHead = new QHBoxLayout();
    auto *solInfo = new QLabel("Mientras una solicitud esté abierta, sus títulos se marcan para devolución en el inventario.", sol);
    solInfo->setObjectName("Muted");
    solInfo->setWordWrap(true);
    solHead->addWidget(solInfo, 1);
    auto *btnNueva = Ui::button("Registrar solicitud", "primary", sol, "plus-dark");
    solHead->addWidget(btnNueva);
    sol_l->addLayout(solHead);
    m_tablaSol = Ui::table({"Folio", "Fecha", "Editorial", "Referencia del correo", "Títulos", "Estado", "Acción"}, 3, sol, 56);
    m_tablaSol->setWordWrap(true);
    sol_l->addWidget(m_tablaSol, 1);
    m_stack->addWidget(sol);

    connect(m_tablaInv, &QTableWidget::cellClicked, this, [this](int row, int) {
        if (row >= 0 && row < m_ids.size()) cargarDetalle(m_ids.at(row));
    });
    connect(m_btnAprobar, &QPushButton::clicked, this, [this]() { revisar(true); });
    connect(m_btnRechazar, &QPushButton::clicked, this, [this]() { revisar(false); });
    connect(btnNueva, &QPushButton::clicked, this, &JefeInventarioView::nuevaSolicitud);

    m_btnAprobar->hide();
    m_btnRechazar->hide();
    mostrarSeccion(0);
}

void JefeInventarioView::mostrarSeccion(int i) {
    for (int k = 0; k < m_segmentos.size(); ++k) Ui::setCurrent(m_segmentos.at(k), k == i);
    m_stack->setCurrentIndex(i);
}

void JefeInventarioView::cargar() {
    cargarInventarios();
    cargarSolicitudes();
}

void JefeInventarioView::cargarInventarios() {
    ApiClient::instance()->listarInventarios([this](bool ok, const QJsonValue &data, const QString &msg) {
        if (!ok || !data.isArray()) {
            Toast::showToast(this, "Error al consultar inventarios: " + msg, Toast::Error);
            return;
        }
        const QJsonArray arr = data.toArray();
        m_ids.clear();
        m_tablaInv->clearContents();
        m_tablaInv->setRowCount(arr.size());
        qint64 porRevisar = 0;
        for (int i = 0; i < arr.size(); ++i) {
            const QJsonObject r = arr.at(i).toObject();
            const qint64 id = r.value("id_inventario").toVariant().toLongLong();
            const QString estado = r.value("estado").toString();
            if (estado == "CERRADO" && porRevisar == 0) porRevisar = id;
            m_ids << id;
            Ui::cellText(m_tablaInv, i, 0, QString("#INV-%1").arg(id), true);
            Ui::cellText(m_tablaInv, i, 1, r.value("fecha_inicio").toString(), true, false, Theme::TextMuted);
            Ui::cellText(m_tablaInv, i, 2, r.value("fecha_cierre").toString("—"), true, false, Theme::TextMuted);
            Ui::cellText(m_tablaInv, i, 3, r.value("contado_por").toString());
            Ui::cellWidget(m_tablaInv, i, 4, Ui::chip(etiquetaEstado(estado), chipEstado(estado)));
            Ui::cellText(m_tablaInv, i, 5, QString::number(r.value("total_productos").toInt()));
            const int dif = r.value("con_diferencia").toInt();
            Ui::cellText(m_tablaInv, i, 6, QString::number(dif), false, dif > 0, dif > 0 ? Theme::Warning : Theme::TextMuted);
            Ui::cellText(m_tablaInv, i, 7, QString::number(r.value("a_devolucion").toInt()));
        }
        // Abre el primero por revisar (o el más reciente).
        const qint64 sel = m_idSel && m_ids.contains(m_idSel) ? m_idSel : (porRevisar ? porRevisar : (m_ids.isEmpty() ? 0 : m_ids.first()));
        if (sel) cargarDetalle(sel);
    });
}

void JefeInventarioView::cargarDetalle(qint64 id) {
    m_idSel = id;
    const int fila = m_ids.indexOf(id);
    if (fila >= 0) m_tablaInv->selectRow(fila);
    ApiClient::instance()->obtenerInventario(id, [this, id](bool ok, const QJsonValue &data, const QString &msg) {
        if (!ok || !data.isObject()) {
            Toast::showToast(this, "Error al consultar el inventario: " + msg, Toast::Error);
            return;
        }
        const QJsonObject d = data.toObject();
        const QJsonObject r = d.value("resumen").toObject();
        const QString estado = r.value("estado").toString();
        QString txt = QString("<b>#INV-%1</b> · %2 · %3 producto(s), %4 con diferencia, %5 a devolución (añejado = sin ventas en %6 días)")
            .arg(id).arg(etiquetaEstado(estado))
            .arg(r.value("total_productos").toInt()).arg(r.value("con_diferencia").toInt())
            .arg(r.value("a_devolucion").toInt()).arg(r.value("dias_anejamiento").toInt());
        if (!r.value("revisado_por").isNull())
            txt += QString("<br>Revisó %1 el %2%3").arg(r.value("revisado_por").toString().toHtmlEscaped(),
                r.value("fecha_revision").toString(),
                r.value("observaciones").toString().isEmpty() ? QString()
                    : " · " + r.value("observaciones").toString().toHtmlEscaped());
        m_lblDetalle->setText(txt);
        m_btnAprobar->setVisible(estado == "CERRADO");
        m_btnRechazar->setVisible(estado == "CERRADO");

        const QJsonArray items = d.value("items").toArray();
        m_tablaDet->clearContents();
        m_tablaDet->setRowCount(items.size());
        for (int i = 0; i < items.size(); ++i) {
            const QJsonObject o = items.at(i).toObject();
            Ui::cellText(m_tablaDet, i, 0, QString::number(o.value("codigo_ean").toVariant().toLongLong()), true);
            Ui::cellText(m_tablaDet, i, 1, o.value("titulo").toString(), false, true);
            Ui::cellText(m_tablaDet, i, 2, o.value("proveedor").toString(), false, false, Theme::TextMuted);
            Ui::cellText(m_tablaDet, i, 3, o.value("stock_sistema").isNull() ? "—" : QString::number(o.value("stock_sistema").toInt()));
            Ui::cellText(m_tablaDet, i, 4, QString::number(o.value("cantidad_contada").toInt()), false, true);
            if (o.value("diferencia").isNull()) {
                Ui::cellText(m_tablaDet, i, 5, "—");
            } else {
                const int dif = o.value("diferencia").toInt();
                Ui::cellWidget(m_tablaDet, i, 5, dif == 0 ? Ui::chip("0", "neutral")
                    : Ui::chip(QString("%1%2").arg(dif > 0 ? "+" : "").arg(dif), dif > 0 ? "blue" : "danger"));
            }
            Ui::cellText(m_tablaDet, i, 6, QString::number(o.value("stock_apartado").toInt()), false, false, Theme::TextMuted);
            const QString motivo = o.value("motivo_devolucion").toString();
            if (motivo == "SOLICITUD_EDITORIAL") Ui::cellWidget(m_tablaDet, i, 7, Ui::chip("Pidió la editorial", "purple"));
            else if (motivo == "ANEJAMIENTO") Ui::cellWidget(m_tablaDet, i, 7, Ui::chip("Añejado", "amber"));
            else Ui::cellText(m_tablaDet, i, 7, "—", false, false, Theme::TextMuted);
        }
    });
}

void JefeInventarioView::revisar(bool aprobar) {
    if (!m_idSel) return;
    QString obs;
    if (aprobar) {
        if (QMessageBox::question(this, "Aprobar inventario",
                QString("¿Aprobar #INV-%1?\n\nEl stock de piso se ajusta con las diferencias contadas.").arg(m_idSel),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    } else {
        bool aceptado = false;
        obs = QInputDialog::getMultiLineText(this, "Rechazar inventario",
            "Motivo del rechazo (obligatorio, máx. 200 caracteres):", QString(), &aceptado).trimmed();
        if (!aceptado) return;
        if (obs.isEmpty() || obs.size() > 200) {
            Toast::showToast(this, "Escribe un motivo de 1 a 200 caracteres", Toast::Warning);
            return;
        }
    }
    m_btnAprobar->setEnabled(false);
    m_btnRechazar->setEnabled(false);
    ApiClient::instance()->revisarInventario(m_idSel, aprobar, obs, [this](bool ok, const QJsonValue &, const QString &msg) {
        m_btnAprobar->setEnabled(true);
        m_btnRechazar->setEnabled(true);
        Toast::showToast(this, ok ? msg : "No se pudo revisar: " + msg, ok ? Toast::Success : Toast::Error);
        cargarInventarios();
    });
}

void JefeInventarioView::cargarSolicitudes() {
    ApiClient::instance()->listarSolicitudesEditorial([this](bool ok, const QJsonValue &data, const QString &msg) {
        if (!ok || !data.isArray()) {
            Toast::showToast(this, "Error al consultar solicitudes: " + msg, Toast::Error);
            return;
        }
        const QJsonArray arr = data.toArray();
        m_tablaSol->clearContents();
        m_tablaSol->setRowCount(arr.size());
        for (int i = 0; i < arr.size(); ++i) {
            const QJsonObject s = arr.at(i).toObject();
            const qint64 id = s.value("id_solicitud").toVariant().toLongLong();
            const bool abierta = s.value("estado").toString() == "ABIERTA";
            QStringList titulos;
            for (const auto &v : s.value("items").toArray()) titulos << v.toObject().value("titulo").toString();
            Ui::cellText(m_tablaSol, i, 0, QString("#SOL-%1").arg(id), true);
            Ui::cellText(m_tablaSol, i, 1, s.value("fecha").toString(), true, false, Theme::TextMuted);
            Ui::cellText(m_tablaSol, i, 2, s.value("proveedor").toString(), false, true);
            Ui::cellText(m_tablaSol, i, 3, s.value("referencia").toString());
            auto *t = Ui::cellText(m_tablaSol, i, 4, QString("%1 título(s)").arg(titulos.size()), false, false, Theme::TextMuted);
            t->setToolTip(titulos.join("\n"));
            Ui::cellWidget(m_tablaSol, i, 5, Ui::chip(abierta ? "Abierta" : "Atendida", abierta ? "pending" : "ok"));
            if (abierta) {
                auto *b = Ui::button("Marcar atendida", "outline");
                b->setProperty("variant", "small");
                connect(b, &QPushButton::clicked, this, [this, id]() {
                    ApiClient::instance()->atenderSolicitudEditorial(id, [this](bool ok, const QJsonValue &, const QString &msg) {
                        Toast::showToast(this, ok ? msg : "No se pudo: " + msg, ok ? Toast::Success : Toast::Error);
                        cargarSolicitudes();
                    });
                });
                Ui::cellWidget(m_tablaSol, i, 6, b);
            } else {
                Ui::cellText(m_tablaSol, i, 6, "—", false, false, Theme::TextMuted);
            }
        }
    });
}

void JefeInventarioView::nuevaSolicitud() {
    Ui::FormDialog dlg("Registrar solicitud de editorial", "Registrar solicitud", this);
    auto *combo = new QComboBox(&dlg);
    auto *txtRef = new QLineEdit(&dlg);
    txtRef->setMaxLength(150);
    txtRef->setPlaceholderText("Ej. Correo del 14/11: retiro de NatGeo marzo");
    auto *txtEans = new QPlainTextEdit(&dlg);
    txtEans->setPlaceholderText("Un EAN-13 por renglón");
    txtEans->setFixedHeight(120);
    dlg.addField("Editorial / proveedor", combo, 0, 0, 2);
    dlg.addField("Referencia del correo", txtRef, 1, 0, 2);
    dlg.addField("Títulos que pide de vuelta (EAN)", txtEans, 2, 0, 2);

    ApiClient::instance()->listarProveedores([combo](bool ok, const QJsonValue &data, const QString &) {
        if (!ok || !data.isArray()) return;
        for (const auto &v : data.toArray()) {
            const ProveedorDto p = ProveedorDto::fromJson(v.toObject());
            combo->addItem(p.nombre_proveedor, p.id_proveedor);
        }
    });

    QPointer<QDialog> guard(&dlg);
    QPointer<JefeInventarioView> self(this);
    connect(dlg.okButton(), &QPushButton::clicked, &dlg, [&, guard, self]() {
        QList<qint64> eans;
        for (const QString &l : txtEans->toPlainText().split('\n', Qt::SkipEmptyParts)) {
            bool okNum = false;
            const qint64 e = l.trimmed().toLongLong(&okNum);
            if (!okNum || l.trimmed().size() != 13) {
                Toast::showToast(&dlg, QString("EAN inválido: «%1» (deben ser 13 dígitos)").arg(l.trimmed()), Toast::Warning);
                return;
            }
            if (!eans.contains(e)) eans << e;
        }
        if (combo->currentIndex() < 0 || txtRef->text().trimmed().isEmpty() || eans.isEmpty()) {
            Toast::showToast(&dlg, "Elige la editorial, escribe la referencia y al menos un EAN", Toast::Warning);
            return;
        }
        dlg.okButton()->setEnabled(false);
        ApiClient::instance()->crearSolicitudEditorial(combo->currentData().toInt(), txtRef->text().trimmed(), eans,
            [guard, self](bool ok, const QJsonValue &, const QString &msg) {
                if (!ok) {
                    if (guard) static_cast<Ui::FormDialog *>(guard.data())->okButton()->setEnabled(true);
                    Toast::showToast(guard ? guard.data() : static_cast<QWidget *>(self.data()), "No se registró: " + msg, Toast::Error);
                    return;
                }
                if (self) {
                    Toast::showToast(self, "Solicitud registrada", Toast::Success);
                    self->cargarSolicitudes();
                }
                if (guard) guard->accept();
            });
    });
    dlg.exec();
}
