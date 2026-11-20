#include "JefeInventarioView.h"
#include "ApiClient.h"
#include "Models.h"
#include "Theme.h"
#include "Toast.h"
#include "Ui.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include "Limites.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <functional>
#include <memory>
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

    // Una sola vista con scroll: inventarios y, debajo, solicitudes de editorial.
    // Arriba quedan fijas las anclas para saltar a cada sección.
    m_secciones = new SeccionesScroll(this);
    layout->addWidget(m_secciones, 1);

    // --- Inventarios ---------------------------------------------------------
    auto *inv = m_secciones->agregarSeccion("Inventarios");
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
    m_tablaDet->setMinimumHeight(300);   // dentro del scroll ya no se estira sola
    il->addWidget(m_tablaDet);

    // --- Solicitudes de editorial ------------------------------------------------
    auto *sol = m_secciones->agregarSeccion("Solicitudes de editorial");
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
    m_tablaSol->setMinimumHeight(260);
    sol_l->addWidget(m_tablaSol);

    connect(m_tablaInv, &QTableWidget::cellClicked, this, [this](int row, int) {
        if (row >= 0 && row < m_ids.size()) cargarDetalle(m_ids.at(row));
    });
    connect(m_btnAprobar, &QPushButton::clicked, this, [this]() { revisar(true); });
    connect(m_btnRechazar, &QPushButton::clicked, this, [this]() { revisar(false); });
    connect(btnNueva, &QPushButton::clicked, this, &JefeInventarioView::nuevaSolicitud);

    m_btnAprobar->hide();
    m_btnRechazar->hide();
}

void JefeInventarioView::mostrarSeccion(int i) {
    m_secciones->irA(i);
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
        if (!Ui::pedirTexto(this, "Rechazar inventario", "Motivo del rechazo (obligatorio)",
                            "Ej. El conteo de la sección infantil no coincide; repetirlo", Limites::kObservaciones,
                            /*obligatoria=*/true, obs))
            return;
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
            int piezas = 0;
            for (const auto &v : s.value("items").toArray()) piezas += v.toObject().value("piezas").toInt();
            auto *t = Ui::cellText(m_tablaSol, i, 4, QString("%1 título(s) · %2 pza(s)").arg(titulos.size()).arg(piezas),
                                   false, false, Theme::TextMuted);
            t->setToolTip(titulos.join("\n"));
            auto *ver = Ui::iconButton("eye", "Ver los títulos que pide la editorial");
            connect(ver, &QPushButton::clicked, this, [this, s]() { verTitulosSolicitud(s); });
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
                Ui::cellWidgets(m_tablaSol, i, 6, {ver, b});
            } else {
                Ui::cellWidget(m_tablaSol, i, 6, ver);
            }
        }
    });
}

// Títulos de una solicitud ya registrada, con las piezas que hay que regresar.
void JefeInventarioView::verTitulosSolicitud(const QJsonObject &s) {
    const QJsonArray items = s.value("items").toArray();
    int piezas = 0;
    for (const auto &v : items) piezas += v.toObject().value("piezas").toInt();
    Ui::FormDialog dlg(QString("Solicitud #SOL-%1").arg(s.value("id_solicitud").toVariant().toLongLong()), "Cerrar", this, 720);
    for (QPushButton *b : dlg.findChildren<QPushButton *>())
        if (b != dlg.okButton() && b->text() == "Cancelar") b->hide();
    auto *res = new QLabel(QString("<b>%1</b> · %2<br><span style=\"color:%3;\">%4 título(s) · %5 pieza(s) por regresar</span>")
        .arg(s.value("proveedor").toString().toHtmlEscaped(), s.value("referencia").toString().toHtmlEscaped(), Theme::TextMuted)
        .arg(items.size()).arg(piezas), &dlg);
    res->setTextFormat(Qt::RichText);
    res->setWordWrap(true);
    auto *tabla = Ui::table({"EAN", "Título", "Por regresar"}, 1, &dlg, 44);
    tabla->setRowCount(items.size());
    for (int i = 0; i < items.size(); ++i) {
        const QJsonObject o = items.at(i).toObject();
        Ui::cellText(tabla, i, 0, QString::number(o.value("codigo_ean").toVariant().toLongLong()), true);
        Ui::cellText(tabla, i, 1, o.value("titulo").toString(), false, true);
        Ui::cellText(tabla, i, 2, QString::number(o.value("piezas").toInt()));
    }
    tabla->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tabla->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tabla->setMinimumHeight(qMin(340, 56 + 44 * qMax(1, static_cast<int>(items.size()))));
    dlg.addField("Editorial y referencia", res, 0, 0, 2);
    dlg.addField("Títulos que pide de vuelta", tabla, 1, 0, 2);
    connect(dlg.okButton(), &QPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
}

void JefeInventarioView::nuevaSolicitud() {
    Ui::FormDialog dlg("Registrar solicitud de editorial", "Registrar solicitud", this, 820);
    auto *combo = new QComboBox(&dlg);
    auto *txtRef = new QLineEdit(&dlg);
    txtRef->setPlaceholderText("Ej. Correo del 14/11: retiro de NatGeo marzo");
    auto *txtEans = new QPlainTextEdit(&dlg);
    txtEans->setPlaceholderText("Un EAN-13 por renglón (o escanéalos uno tras otro)");
    txtEans->setFixedHeight(96);
    txtEans->setStyleSheet("QPlainTextEdit { min-height: 66px; }");   // el estilo global la dejaba en un renglón

    // Vista previa: qué título es cada EAN, cuántas piezas hay y si es de esa editorial.
    auto *tabla = Ui::table({"EAN", "Título", "Piso", "Bodega", "Por regresar", "Estado"}, 1, &dlg, 42);
    auto *hh = tabla->horizontalHeader();
    for (int c : {0, 2, 3, 4, 5}) hh->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    tabla->setMinimumHeight(170);
    auto *lblResumen = new QLabel("Escribe los EAN para ver los títulos.", &dlg);
    lblResumen->setObjectName("Muted");
    lblResumen->setTextFormat(Qt::RichText);
    lblResumen->setWordWrap(true);

    dlg.addField("Editorial / proveedor", combo, 0, 0, 2);
    dlg.addField("Referencia del correo", Ui::conLimite(txtRef, Limites::kReferenciaSolicitud, &dlg), 1, 0, 2);
    dlg.addField("Títulos que pide de vuelta (EAN)", txtEans, 2, 0, 2);
    dlg.addField("Vista previa", tabla, 3, 0, 2);
    dlg.addField("Resumen", lblResumen, 4, 0, 2);

    // EAN → existencia (o {"_no": true} si no existe). Las búsquedas son asíncronas.
    auto cache = std::make_shared<QHash<qint64, QJsonObject>>();
    auto buscando = std::make_shared<QSet<qint64>>();
    QPointer<QDialog> guard(&dlg);

    // Lee los renglones: EAN válidos (sin repetir) y renglones mal escritos.
    auto leer = [txtEans](QStringList *malos) {
        QList<qint64> eans;
        for (const QString &l : txtEans->toPlainText().split('\n', Qt::SkipEmptyParts)) {
            const QString t = l.trimmed();
            if (t.isEmpty()) continue;
            bool okNum = false;
            const qint64 e = t.toLongLong(&okNum);
            if (!okNum || t.size() != 13) { if (malos) *malos << t; continue; }
            if (!eans.contains(e)) eans << e;
        }
        return eans;
    };

    // Cuenta de lo que impide registrar: EAN mal escrito, inexistente, de otra editorial o aún buscando.
    auto problemas = std::make_shared<int>(0);
    auto pintar = std::make_shared<std::function<void()>>();
    std::weak_ptr<std::function<void()>> pintarW = pintar;
    *pintar = [=]() {
        if (!guard) return;
        QStringList malos;
        const QList<qint64> eans = leer(&malos);
        const int idProv = combo->currentData().toInt();
        tabla->clearContents();
        tabla->setRowCount(eans.size() + malos.size());
        int titulos = 0, piezas = 0, otros = 0, noExisten = 0, pendientes = 0;
        int fila = 0;
        for (qint64 e : eans) {
            Ui::cellText(tabla, fila, 0, QString::number(e), true);
            if (!cache->contains(e)) {
                ++pendientes;
                Ui::cellText(tabla, fila, 1, "Buscando…", false, false, Theme::TextMuted);
                if (!buscando->contains(e)) {
                    buscando->insert(e);
                    ApiClient::instance()->consultarExistencias(QString::number(e), QString(), 0,
                        [=](bool ok, const QJsonValue &data, const QString &) {
                            buscando->remove(e);
                            QJsonObject hallado{{"_no", true}};
                            if (ok && data.isArray())
                                for (const auto &v : data.toArray())
                                    if (v.toObject().value("codigo_ean").toVariant().toLongLong() == e) hallado = v.toObject();
                            if (!ok) return;   // se reintenta al volver a pintar
                            cache->insert(e, hallado);
                            if (auto f = pintarW.lock()) (*f)();
                        });
                }
            } else {
                const QJsonObject o = cache->value(e);
                if (o.value("_no").toBool()) {
                    ++noExisten;
                    Ui::cellText(tabla, fila, 1, "—", false, false, Theme::TextMuted);
                    Ui::cellWidget(tabla, fila, 5, Ui::chip("No existe", "danger"));
                } else {
                    const int piso = o.value("stock_tienda").toInt();
                    const int bod = o.value("stock_bodega").toInt();
                    const bool deOtra = o.value("id_proveedor").toInt() != idProv;
                    Ui::cellText(tabla, fila, 1, o.value("titulo").toString(), false, true);
                    Ui::cellText(tabla, fila, 2, QString::number(piso));
                    Ui::cellText(tabla, fila, 3, QString::number(bod));
                    Ui::cellText(tabla, fila, 4, QString::number(piso + bod), false, true);
                    if (deOtra) {
                        ++otros;
                        auto *c = Ui::chip("Es de " + o.value("proveedor").toString(), "amber");
                        c->setToolTip("Registra una solicitud por editorial");
                        Ui::cellWidget(tabla, fila, 5, c);
                    } else {
                        ++titulos;
                        piezas += piso + bod;
                        Ui::cellWidget(tabla, fila, 5, Ui::chip("Listo", "ok"));
                    }
                }
            }
            ++fila;
        }
        for (const QString &m : std::as_const(malos)) {
            Ui::cellText(tabla, fila, 0, m, true, false, Theme::Error);
            Ui::cellText(tabla, fila, 1, "Debe tener 13 dígitos", false, false, Theme::Error);
            Ui::cellWidget(tabla, fila, 5, Ui::chip("EAN inválido", "danger"));
            ++fila;
        }
        *problemas = static_cast<int>(malos.size()) + noExisten + otros + pendientes;
        QString txt = eans.isEmpty() && malos.isEmpty() ? QString("Escribe los EAN para ver los títulos.")
            : QString("<b style=\"color:%1;\">%2 título(s)</b> · <b style=\"color:%1;\">%3 pieza(s)</b> por regresar (piso + bodega)")
                  .arg(Theme::TextPrimary).arg(titulos).arg(piezas);
        QStringList avisos;
        if (!malos.isEmpty()) avisos << QString("%1 EAN mal escrito(s)").arg(malos.size());
        if (noExisten) avisos << QString("%1 no existe(n)").arg(noExisten);
        if (otros) avisos << QString("%1 de otra editorial").arg(otros);
        if (!avisos.isEmpty()) txt += QString("<br><span style=\"color:%1;\">Corrige antes de registrar: %2</span>")
                                          .arg(Theme::Error, avisos.join(" · "));
        lblResumen->setText(txt);
    };

    auto *timer = new QTimer(&dlg);
    timer->setSingleShot(true);
    timer->setInterval(350);
    connect(timer, &QTimer::timeout, &dlg, [pintar]() { (*pintar)(); });
    connect(txtEans, &QPlainTextEdit::textChanged, timer, qOverload<>(&QTimer::start));
    connect(combo, &QComboBox::currentIndexChanged, &dlg, [pintar]() { (*pintar)(); });

    ApiClient::instance()->listarProveedores([combo, guard](bool ok, const QJsonValue &data, const QString &) {
        if (!guard || !ok || !data.isArray()) return;
        for (const auto &v : data.toArray()) {
            const ProveedorDto p = ProveedorDto::fromJson(v.toObject());
            combo->addItem(p.nombre_proveedor, p.id_proveedor);
        }
    });

    QPointer<JefeInventarioView> self(this);
    connect(dlg.okButton(), &QPushButton::clicked, &dlg, [&, guard, self]() {
        (*pintar)();
        const QList<qint64> eans = leer(nullptr);
        if (combo->currentIndex() < 0 || txtRef->text().trimmed().isEmpty() || eans.isEmpty()) {
            Toast::showToast(&dlg, "Elige la editorial, escribe la referencia y al menos un EAN", Toast::Warning);
            return;
        }
        if (*problemas > 0) {
            Toast::showToast(&dlg, "Revisa la vista previa: hay EAN inválidos, inexistentes, de otra editorial o aún buscándose",
                             Toast::Warning);
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
