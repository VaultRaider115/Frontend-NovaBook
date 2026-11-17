#include "VendedorRecepcionView.h"
#include "ApiClient.h"
#include "Theme.h"
#include "Toast.h"
#include "Ui.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QVBoxLayout>

namespace {
const char *chipEstado(const QString &e) {
    if (e == "CONFIRMADA") return "ok";
    if (e == "CORREGIDA") return "amber";
    if (e == "PENDIENTE") return "pending";
    return "neutral";
}
QString etiquetaEstado(const QString &e) {
    if (e == "PENDIENTE") return "Por recibir";
    if (e == "CONFIRMADA") return "Recibida";
    if (e == "CORREGIDA") return "Corregida";
    return e.left(1) + e.mid(1).toLower();
}
} // namespace

VendedorRecepcionView::VendedorRecepcionView(bool puedeRecibir, QWidget *parent)
    : QWidget(parent), m_puedeRecibir(puedeRecibir)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(18);

    auto *top = new QHBoxLayout();
    top->addWidget(Ui::pageHeader(
        puedeRecibir ? "Vendedor · movimiento" : "Bodega · seguimiento",
        puedeRecibir ? "Recepción de Salida" : "Seguimiento de Salidas",
        puedeRecibir ? "Confirma lo que bajó Bodega, corrige si no coincide y elige el mueble de piso de cada libro."
                     : "Revisa si el piso ya recibió tus requisiciones de salida y reimprime las que se corrigieron.",
        this), 1);
    auto *stat = Ui::card(this);
    auto *sl = new QVBoxLayout(stat);
    sl->setContentsMargins(20, 12, 20, 12);
    sl->setSpacing(2);
    sl->addWidget(Ui::label("Por recibir en piso", "Muted", stat));
    m_lblPendientes = Ui::label("0", "BigNumber", stat);
    sl->addWidget(m_lblPendientes);
    top->addWidget(stat, 0, Qt::AlignTop);
    layout->addLayout(top);

    auto *filters = Ui::card(this);
    auto *fl = new QHBoxLayout(filters);
    fl->setContentsMargins(16, 16, 16, 16);
    fl->setSpacing(12);
    m_comboEstado = new QComboBox(filters);
    m_comboEstado->addItem("Estado: Por recibir", "PENDIENTE");
    m_comboEstado->addItem("Estado: Recibidas", "CONFIRMADA");
    m_comboEstado->addItem("Estado: Corregidas", "CORREGIDA");
    m_comboEstado->addItem("Estado: Todas", "");
    m_comboEstado->setMinimumWidth(220);
    m_comboEstado->setAccessibleName("Estado");
    m_btnActualizar = Ui::button("Actualizar", "primary", filters, "refresh");
    fl->addWidget(m_comboEstado);
    fl->addWidget(m_btnActualizar);
    fl->addStretch();
    layout->addWidget(filters);

    m_lista = Ui::table({"Folio", "Fecha", "Editorial", "Elaboró", "Piezas", "Estado", "Acción"}, 2, this, 56);
    layout->addWidget(m_lista, 1);

    // Detalle de la requisición abierta.
    m_detalle = Ui::card(this);
    auto *dl = new QVBoxLayout(m_detalle);
    dl->setContentsMargins(20, 18, 20, 18);
    dl->setSpacing(12);
    m_lblDetalleTitulo = Ui::label("", "CardTitle", m_detalle);
    m_lblDetalleTitulo->setTextFormat(Qt::RichText);
    m_lblDetalleTitulo->setStyleSheet("font-size: 16px; font-weight: 600;");
    m_lblDetalleSub = Ui::label("", "Muted", m_detalle);
    m_lblDetalleSub->setWordWrap(true);
    dl->addWidget(m_lblDetalleTitulo);
    dl->addWidget(m_lblDetalleSub);

    m_tabla = Ui::table({"EAN", "Tipo", "Descripción", "Enviado", "Recibido", "Mueble de piso", "Revisión"}, 2, m_detalle, 56);
    m_tabla->setMinimumHeight(200);
    dl->addWidget(m_tabla, 1);

    auto *pie = new QHBoxLayout();
    pie->setSpacing(12);
    m_txtObs = new QLineEdit(m_detalle);
    m_txtObs->setMaxLength(150);
    m_txtObs->setPlaceholderText("Observaciones (opcional): p. ej. llegaron 2 menos de Cien años de soledad");
    pie->addWidget(m_txtObs, 1);
    pie->addStretch(0);   // en solo lectura (sin observaciones) los botones quedan a la derecha
    m_lblResumen = Ui::label("", "Muted", m_detalle);
    m_lblResumen->setTextFormat(Qt::RichText);
    pie->addWidget(m_lblResumen);
    m_btnCerrar = Ui::button("Cerrar", "ghost", m_detalle);
    m_btnImprimir = Ui::button("Reimprimir", "outline", m_detalle, "pdf");
    m_btnEnviar = Ui::button("Todo coincide · Confirmar", "primary", m_detalle, "check-dark");
    pie->addWidget(m_btnCerrar);
    pie->addWidget(m_btnImprimir);
    pie->addWidget(m_btnEnviar);
    dl->addLayout(pie);
    m_detalle->hide();
    layout->addWidget(m_detalle, 2);

    connect(m_btnActualizar, &QPushButton::clicked, this, &VendedorRecepcionView::cargar);
    connect(m_comboEstado, QOverload<int>::of(&QComboBox::activated), this, [this]() { cargarLista(); });
    connect(m_btnCerrar, &QPushButton::clicked, this, [this]() { m_detalle->hide(); m_lista->clearSelection(); });
    connect(m_btnImprimir, &QPushButton::clicked, this, [this]() { imprimir(m_actual); });
    connect(m_btnEnviar, &QPushButton::clicked, this, &VendedorRecepcionView::enviar);
}

void VendedorRecepcionView::cargar() {
    // Muebles del piso de venta para elegir dónde quedó cada libro (paso 3).
    if (m_puedeRecibir) {
        ApiClient::instance()->listarMuebles([this](bool ok, const QJsonValue &data, const QString &) {
            if (!ok || !data.isArray()) return;
            m_mueblesPiso.clear();
            for (const auto &v : data.toArray()) {
                const MuebleDto m = MuebleDto::fromJson(v.toObject());
                if (m.id_ubicacion == 1) m_mueblesPiso.append(m);
            }
        });
    }
    cargarLista();
}

void VendedorRecepcionView::cargarLista() {
    m_btnActualizar->setEnabled(false);
    const QString estado = m_comboEstado->currentData().toString();
    ApiClient::instance()->listarRequisiciones("SALIDA", estado, [this, estado](bool ok, const QJsonValue &data, const QString &msg) {
        m_btnActualizar->setEnabled(true);
        if (!ok || !data.isArray()) {
            Toast::showToast(this, "No se pudieron consultar las requisiciones: " + msg, Toast::Error);
            return;
        }
        const QJsonArray arr = data.toArray();
        m_lista->clearContents();
        m_lista->setRowCount(arr.size());
        int pendientes = 0;
        for (int i = 0; i < arr.size(); ++i) {
            const QJsonObject o = arr.at(i).toObject();
            const QString folio = o.value("folio").toString();
            const QString est = o.value("estado").toString();
            if (est == "PENDIENTE") ++pendientes;
            Ui::cellText(m_lista, i, 0, "#" + folio, true);
            Ui::cellText(m_lista, i, 1, o.value("fecha_hora").toString().left(16), true, false, Theme::TextMuted);
            Ui::cellText(m_lista, i, 2, o.value("editorial").toString(), false, true);
            Ui::cellText(m_lista, i, 3, o.value("registro").toString(), false, false, Theme::TextMuted);
            auto *pzs = new QLabel(QString("<b>%1</b> <span style=\"color:%2;\">en %3 partida(s)</span>")
                .arg(o.value("total_piezas").toInt()).arg(Theme::TextMuted).arg(o.value("partidas").toInt()));
            pzs->setTextFormat(Qt::RichText);
            Ui::cellWidget(m_lista, i, 4, pzs);
            Ui::cellWidget(m_lista, i, 5, Ui::chip(etiquetaEstado(est), chipEstado(est)));
            const bool recibir = m_puedeRecibir && est == "PENDIENTE";
            auto *btn = Ui::button(recibir ? "Recibir" : "Ver", recibir ? "tint" : "outline", nullptr,
                                   recibir ? "check-gold" : QString());
            btn->setProperty("variant", "small");
            btn->setAccessibleName(QString("%1 %2").arg(recibir ? "Recibir" : "Ver", folio));
            connect(btn, &QPushButton::clicked, this, [this, folio]() { abrir(folio); });
            Ui::cellWidget(m_lista, i, 6, btn);
        }
        // El contador siempre refleja las pendientes, aunque el filtro sea otro.
        if (estado == "PENDIENTE" || estado.isEmpty()) {
            m_lblPendientes->setText(QString::number(pendientes));
        } else {
            ApiClient::instance()->listarRequisiciones("SALIDA", "PENDIENTE", [this](bool ok2, const QJsonValue &d2, const QString &) {
                if (ok2 && d2.isArray()) m_lblPendientes->setText(QString::number(d2.toArray().size()));
            });
        }
        if (arr.isEmpty())
            Toast::showToast(this, estado == "PENDIENTE" ? "No hay salidas por recibir" : "No hay requisiciones con ese estado",
                             Toast::Info);
    });
}

void VendedorRecepcionView::abrir(const QString &folio) {
    ApiClient::instance()->obtenerRequisicion(folio, [this](bool ok, const QJsonValue &data, const QString &msg) {
        if (!ok || !data.isObject()) {
            Toast::showToast(this, "No se pudo abrir la requisición: " + msg, Toast::Error);
            return;
        }
        mostrarDetalle(Documentos::Requisicion::desdeJson(data.toObject()), data.toObject());
    });
}

void VendedorRecepcionView::mostrarDetalle(const Documentos::Requisicion &req, const QJsonObject &json) {
    m_actual = req;
    m_actualJson = json;
    const bool editable = m_puedeRecibir && req.estado == "PENDIENTE";

    m_lblDetalleTitulo->setText(QString("Folio <span style=\"color:%1;\">#%2</span> · %3")
                                    .arg(Theme::Accent, req.folio, req.editorial.toHtmlEscaped()));
    QString sub = QString("Elaboró %1 el %2").arg(req.elaboro, req.fechaHora.left(16));
    if (!req.recibio.isEmpty())
        sub += QString(" · Recibió %1 el %2").arg(req.recibio, req.fechaRecepcion.left(16));
    if (editable)
        sub += " · Cuenta lo que llegó: si todo coincide, confirma; si no, captura lo recibido y se reimprime corregida.";
    m_lblDetalleSub->setText(sub);
    if (!req.observaciones.isEmpty()) m_lblDetalleSub->setToolTip(req.observaciones);

    const QJsonArray items = json.value("items").toArray();
    m_tabla->clearContents();
    m_tabla->setRowCount(req.partidas.size());
    m_filas.clear();
    for (int i = 0; i < req.partidas.size(); ++i) {
        const auto &p = req.partidas.at(i);
        const QJsonObject it = items.at(i).toObject();
        Ui::cellText(m_tabla, i, 0, QString::number(p.ean), true);
        Ui::cellWidget(m_tabla, i, 1, Ui::tipoChip(p.tipo.toLower()));
        Ui::cellText(m_tabla, i, 2, p.titulo);
        Ui::cellText(m_tabla, i, 3, QString::number(p.cantidad));

        Fila f;
        if (editable) {
            f.recibido = new QSpinBox();
            f.recibido->setRange(0, 9999);
            f.recibido->setValue(p.cantidad);
            f.recibido->setFixedWidth(96);
            f.recibido->setAccessibleName(QString("Recibido de %1").arg(p.titulo));
            connect(f.recibido, QOverload<int>::of(&QSpinBox::valueChanged), this, &VendedorRecepcionView::actualizarDiferencias);
            Ui::cellWidget(m_tabla, i, 4, f.recibido);

            f.mueble = new QComboBox();
            // Ancho acotado: los nombres largos se recortan y la descripción conserva su espacio.
            f.mueble->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
            f.mueble->setMinimumContentsLength(24);
            f.mueble->setAccessibleName(QString("Mueble de %1").arg(p.titulo));
            const int actual = it.value("id_mueble").toInt();
            bool hay = false;
            for (const MuebleDto &m : std::as_const(m_mueblesPiso)) {
                QString texto = m.codigo + " · " + m.nombre;
                if (!m.genero_asignado.isEmpty()) texto += " (" + m.genero_asignado + ")";
                f.mueble->addItem(texto, m.id_mueble);
                if (m.id_mueble == actual) { f.mueble->setCurrentIndex(f.mueble->count() - 1); hay = true; }
            }
            // Si el catálogo aún no carga, al menos el mueble que asignó el sistema.
            if (!hay) {
                f.mueble->addItem(it.value("mueble").toString(), actual);
                f.mueble->setCurrentIndex(f.mueble->count() - 1);
            }
            f.mueble->setToolTip("El sistema lo asignó por género; cámbialo si lo colocaste en otro mueble");
            Ui::cellWidget(m_tabla, i, 5, f.mueble);
            Ui::fitColumn(m_tabla, 5, f.mueble);

            f.estado = new QLabel();
            Ui::cellWidget(m_tabla, i, 6, f.estado);
        } else {
            const int rec = p.recibida >= 0 ? p.recibida : p.cantidad;
            const bool difiere = p.recibida >= 0 && p.recibida != p.cantidad;
            Ui::cellText(m_tabla, i, 4, p.recibida >= 0 ? QString::number(rec) : "—", false, difiere,
                         difiere ? Theme::Warning : QString());
            Ui::cellText(m_tabla, i, 5, it.value("mueble").toString(), false, false, Theme::TextMuted);
            Ui::cellWidget(m_tabla, i, 6, p.recibida < 0 ? Ui::chip("Por recibir", "pending")
                                          : Ui::chip(difiere ? "Diferencia" : "Coincide", difiere ? "amber" : "ok"));
        }
        m_filas.append(f);
    }

    m_txtObs->setVisible(editable);
    m_txtObs->clear();
    m_btnEnviar->setVisible(editable);
    m_btnImprimir->setVisible(!editable);
    m_lblResumen->setVisible(editable);
    actualizarDiferencias();
    m_detalle->show();
}

void VendedorRecepcionView::actualizarDiferencias() {
    int diferencias = 0, total = 0;
    for (int i = 0; i < m_filas.size(); ++i) {
        const Fila &f = m_filas.at(i);
        if (!f.recibido) continue;
        const int enviado = m_actual.partidas.at(i).cantidad;
        const int rec = f.recibido->value();
        total += rec;
        const bool difiere = rec != enviado;
        if (difiere) ++diferencias;
        f.estado->setText(difiere ? QString("<span style=\"color:%1;\">%2 %3</span>")
                                        .arg(Theme::Warning, rec > enviado ? "Sobran" : "Faltan").arg(qAbs(rec - enviado))
                                  : QString("<span style=\"color:%1;\">Coincide</span>").arg(Theme::Success));
        f.estado->setTextFormat(Qt::RichText);
    }
    m_lblResumen->setText(QString("Recibido: <b style=\"color:#FFFFFF;\">%1</b> pzs").arg(total));
    m_btnEnviar->setText(diferencias ? QString("Corregir y Reimprimir (%1)").arg(diferencias) : "Todo coincide · Confirmar");
}

void VendedorRecepcionView::enviar() {
    if (m_actual.folio.isEmpty()) return;
    QJsonArray items;
    QStringList cambios;
    for (int i = 0; i < m_filas.size(); ++i) {
        const Fila &f = m_filas.at(i);
        const auto &p = m_actual.partidas.at(i);
        items.append(QJsonObject{{"codigo_ean", p.ean},
                                 {"cantidad_recibida", f.recibido->value()},
                                 {"id_mueble", f.mueble->currentData().toInt()}});
        if (f.recibido->value() != p.cantidad)
            cambios << QString("• %1: %2 → %3").arg(p.titulo).arg(p.cantidad).arg(f.recibido->value());
    }
    if (!cambios.isEmpty()) {
        const auto r = QMessageBox::question(this, "Corregir requisición",
            QString("Lo recibido no coincide con el folio %1:\n\n%2\n\n"
                    "La diferencia se ajusta contra bodega y la requisición se reimprime corregida. ¿Continuar?")
                .arg(m_actual.folio, cambios.join("\n")),
            QMessageBox::Yes | QMessageBox::No);
        if (r != QMessageBox::Yes) return;
    }

    m_btnEnviar->setEnabled(false);
    ApiClient::instance()->recibirRequisicion(m_actual.folio, items, m_txtObs->text(),
        [this](bool ok, const QJsonValue &data, const QString &msg) {
            m_btnEnviar->setEnabled(true);
            if (!ok || !data.isObject()) {
                Toast::showToast(this, "No se registró la recepción: " + msg, Toast::Error, 7000);
                return;
            }
            const auto req = Documentos::Requisicion::desdeJson(data.toObject());
            const bool corregida = req.estado == "CORREGIDA";
            Toast::showToast(this, corregida ? QString("Folio %1 corregido con lo recibido").arg(req.folio)
                                             : QString("Folio %1 recibido: todo coincide").arg(req.folio),
                             corregida ? Toast::Warning : Toast::Success, 5000);
            mostrarDetalle(req, data.toObject());
            cargarLista();
            // Diagrama, paso 2: si no coincide, se corrige y se reimprime.
            if (corregida) imprimir(req);
        });
}

void VendedorRecepcionView::imprimir(const Documentos::Requisicion &req) {
    if (req.folio.isEmpty()) return;
    Documentos::guardarYAbrir(this, Documentos::pdfRequisicion(req),
                              QString("requisicion_%1%2.pdf").arg(req.folio, req.estado == "CORREGIDA" ? "_corregida" : ""));
}
