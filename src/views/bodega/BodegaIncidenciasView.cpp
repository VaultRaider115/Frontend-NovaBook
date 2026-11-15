#include "BodegaIncidenciasView.h"
#include "ApiClient.h"
#include "Documentos.h"
#include "Theme.h"
#include "Toast.h"
#include "Ui.h"
#include <QHBoxLayout>
#include <QJsonArray>
#include <QVBoxLayout>

BodegaIncidenciasView::BodegaIncidenciasView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(18);

    auto *top = new QHBoxLayout();
    top->addWidget(Ui::pageHeader("Bodega · Compras", "Incidencias de Compra",
        "Paquetes que llegaron incompletos: avisa al proveedor por correo y registra los artículos restantes cuando lleguen.",
        this), 1);
    auto *stat = Ui::card(this);
    auto *sl = new QVBoxLayout(stat);
    sl->setContentsMargins(20, 12, 20, 12);
    sl->setSpacing(2);
    sl->addWidget(Ui::label("Incidencias abiertas", "Muted", stat));
    m_lblAbiertas = Ui::label("0", "BigNumber", stat);
    sl->addWidget(m_lblAbiertas);
    top->addWidget(stat, 0, Qt::AlignTop);
    layout->addLayout(top);

    auto *filters = Ui::card(this);
    auto *fl = new QHBoxLayout(filters);
    fl->setContentsMargins(16, 16, 16, 16);
    fl->setSpacing(12);
    m_comboEstado = new QComboBox(filters);
    m_comboEstado->addItem("Estado: Abiertas", "ABIERTA");
    m_comboEstado->addItem("Estado: Cerradas", "CERRADA");
    m_comboEstado->addItem("Estado: Todas", "");
    m_comboEstado->setMinimumWidth(220);
    m_comboEstado->setAccessibleName("Estado");
    m_btnActualizar = Ui::button("Actualizar", "primary", filters, "refresh");
    fl->addWidget(m_comboEstado);
    fl->addWidget(m_btnActualizar);
    fl->addStretch();
    layout->addWidget(filters);

    m_lista = Ui::table({"Incidencia", "Factura", "Proveedor", "Fecha", "Faltan", "Estado", "Acción"}, 2, this, 56);
    layout->addWidget(m_lista, 1);

    m_detalle = Ui::card(this);
    auto *dl = new QVBoxLayout(m_detalle);
    dl->setContentsMargins(20, 18, 20, 18);
    dl->setSpacing(12);
    m_lblTitulo = Ui::label("", "CardTitle", m_detalle);
    m_lblTitulo->setTextFormat(Qt::RichText);
    m_lblTitulo->setStyleSheet("font-size: 16px; font-weight: 600;");
    m_lblSub = Ui::label("", "Muted", m_detalle);
    m_lblSub->setWordWrap(true);
    dl->addWidget(m_lblTitulo);
    dl->addWidget(m_lblSub);
    m_tabla = Ui::table({"EAN", "Descripción", "Facturadas", "Recibidas", "Faltan", "Llegaron ahora"}, 1, m_detalle, 56);
    m_tabla->setMinimumHeight(160);
    dl->addWidget(m_tabla, 1);

    auto *pie = new QHBoxLayout();
    pie->setSpacing(12);
    auto *btnCerrar = Ui::button("Cerrar", "ghost", m_detalle);
    pie->addWidget(btnCerrar);
    pie->addStretch();
    m_lblResumen = Ui::label("", "Muted", m_detalle);
    m_lblResumen->setTextFormat(Qt::RichText);
    pie->addWidget(m_lblResumen);
    m_btnCorreo = Ui::button("Avisar al Proveedor", "outline", m_detalle, "receipt");
    m_btnCorreo->setToolTip("Abre tu programa de correo con la lista de faltantes");
    m_btnLlegada = Ui::button("Registrar Llegada", "primary", m_detalle, "check-dark");
    pie->addWidget(m_btnCorreo);
    pie->addWidget(m_btnLlegada);
    dl->addLayout(pie);
    m_detalle->hide();
    layout->addWidget(m_detalle, 2);

    connect(m_btnActualizar, &QPushButton::clicked, this, &BodegaIncidenciasView::cargar);
    connect(m_comboEstado, QOverload<int>::of(&QComboBox::activated), this, [this]() { cargar(); });
    connect(btnCerrar, &QPushButton::clicked, this, [this]() { m_detalle->hide(); });
    connect(m_btnCorreo, &QPushButton::clicked, this, [this]() {
        Documentos::abrirCorreo(this, m_actual.value("correo").toObject());
    });
    connect(m_btnLlegada, &QPushButton::clicked, this, &BodegaIncidenciasView::registrarLlegada);
}

void BodegaIncidenciasView::cargar() {
    m_btnActualizar->setEnabled(false);
    ApiClient::instance()->listarIncidencias(m_comboEstado->currentData().toString(),
        [this](bool ok, const QJsonValue &data, const QString &msg) {
            m_btnActualizar->setEnabled(true);
            if (!ok || !data.isArray()) {
                Toast::showToast(this, "No se pudieron consultar las incidencias: " + msg, Toast::Error);
                return;
            }
            const QJsonArray arr = data.toArray();
            m_lista->clearContents();
            m_lista->setRowCount(arr.size());
            int abiertas = 0;
            for (int i = 0; i < arr.size(); ++i) {
                const QJsonObject o = arr.at(i).toObject();
                const bool abierta = o.value("estado").toString() == "ABIERTA";
                if (abierta) ++abiertas;
                const QString factura = o.value("factura").toString();
                Ui::cellText(m_lista, i, 0, QString("#%1").arg(o.value("id_incidencia").toInt()), true);
                Ui::cellText(m_lista, i, 1, factura.isEmpty() ? QString("Compra #%1").arg(o.value("id_compra").toInt()) : factura, true);
                Ui::cellText(m_lista, i, 2, o.value("proveedor").toString(), false, true);
                Ui::cellText(m_lista, i, 3, o.value("fecha").toString(), true, false, Theme::TextMuted);
                const int faltan = o.value("piezas_pendientes").toInt();
                Ui::cellText(m_lista, i, 4, faltan ? QString("%1 pzs").arg(faltan) : "—", false, faltan > 0,
                             faltan ? Theme::Warning : Theme::TextMuted);
                Ui::cellWidget(m_lista, i, 5, Ui::chip(abierta ? "Abierta" : "Cerrada", abierta ? "amber" : "ok"));
                auto *btn = Ui::button(abierta ? "Atender" : "Ver", abierta ? "tint" : "outline");
                btn->setProperty("variant", "small");
                connect(btn, &QPushButton::clicked, this, [this, o]() { mostrar(o); });
                Ui::cellWidget(m_lista, i, 6, btn);
            }
            // El contador de abiertas se mantiene aunque el filtro sea otro.
            if (m_comboEstado->currentData().toString() == "CERRADA") {
                ApiClient::instance()->listarIncidencias("ABIERTA", [this](bool ok2, const QJsonValue &d2, const QString &) {
                    if (ok2 && d2.isArray()) m_lblAbiertas->setText(QString::number(d2.toArray().size()));
                });
            } else {
                m_lblAbiertas->setText(QString::number(abiertas));
            }
        });
}

void BodegaIncidenciasView::mostrar(const QJsonObject &inc) {
    m_actual = inc;
    const bool abierta = inc.value("estado").toString() == "ABIERTA";
    const QString factura = inc.value("factura").toString();
    m_lblTitulo->setText(QString("Incidencia <span style=\"color:%1;\">#%2</span> · %3 · %4")
        .arg(Theme::Accent).arg(inc.value("id_incidencia").toInt())
        .arg(factura.isEmpty() ? QString("Compra #%1").arg(inc.value("id_compra").toInt()) : "Factura " + factura.toHtmlEscaped(),
             inc.value("proveedor").toString().toHtmlEscaped()));
    QString sub = QString("Recibió %1 el %2").arg(inc.value("recibio").toString(), inc.value("fecha").toString());
    const QString para = inc.value("correo").toObject().value("para").toString();
    sub += para.isEmpty() ? " · El proveedor no tiene correo registrado" : " · Correo del proveedor: " + para;
    if (!abierta) sub += " · Cerrada el " + inc.value("fecha_cierre").toString();
    m_lblSub->setText(sub);

    const QJsonArray items = inc.value("items").toArray();
    m_tabla->clearContents();
    m_tabla->setRowCount(items.size());
    m_llegaron.clear();
    for (int i = 0; i < items.size(); ++i) {
        const QJsonObject it = items.at(i).toObject();
        const int pendiente = it.value("pendiente").toInt();
        Ui::cellText(m_tabla, i, 0, QString::number(it.value("codigo_ean").toVariant().toLongLong()), true);
        Ui::cellText(m_tabla, i, 1, it.value("titulo").toString());
        Ui::cellText(m_tabla, i, 2, QString::number(it.value("cantidad_facturada").toInt()));
        Ui::cellText(m_tabla, i, 3, QString::number(it.value("cantidad_recibida").toInt() + it.value("cantidad_repuesta").toInt()));
        Ui::cellText(m_tabla, i, 4, QString::number(pendiente), false, pendiente > 0, pendiente ? Theme::Warning : Theme::TextMuted);
        if (abierta && pendiente > 0) {
            auto *spn = new QSpinBox();
            spn->setRange(0, pendiente);
            spn->setValue(0);
            spn->setFixedWidth(96);
            spn->setAccessibleName("Llegaron ahora de " + it.value("titulo").toString());
            m_llegaron.append(spn);
            Ui::cellWidget(m_tabla, i, 5, spn);
        } else {
            m_llegaron.append(nullptr);
            Ui::cellWidget(m_tabla, i, 5, Ui::chip(pendiente ? "Pendiente" : "Completo", pendiente ? "pending" : "ok"));
        }
    }
    m_lblResumen->setText(abierta ? QString("Faltan <b style=\"color:#FFFFFF;\">%1</b> pzs").arg(inc.value("piezas_pendientes").toInt())
                                  : QString());
    m_btnCorreo->setVisible(abierta);
    m_btnLlegada->setVisible(abierta);
    m_detalle->show();
}

void BodegaIncidenciasView::registrarLlegada() {
    const QJsonArray items = m_actual.value("items").toArray();
    QJsonArray llegaron;
    for (int i = 0; i < items.size() && i < m_llegaron.size(); ++i) {
        if (!m_llegaron.at(i) || m_llegaron.at(i)->value() == 0) continue;
        llegaron.append(QJsonObject{{"codigo_ean", items.at(i).toObject().value("codigo_ean")},
                                    {"cantidad", m_llegaron.at(i)->value()}});
    }
    if (llegaron.isEmpty()) {
        Toast::showToast(this, "Captura cuántas piezas llegaron de al menos un artículo", Toast::Warning);
        return;
    }
    m_btnLlegada->setEnabled(false);
    const qint64 id = m_actual.value("id_incidencia").toVariant().toLongLong();
    ApiClient::instance()->reponerIncidencia(id, llegaron, [this](bool ok, const QJsonValue &data, const QString &msg) {
        m_btnLlegada->setEnabled(true);
        if (!ok || !data.isObject()) {
            Toast::showToast(this, "No se registró la llegada: " + msg, Toast::Error, 6000);
            return;
        }
        const QJsonObject inc = data.toObject();
        const bool cerrada = inc.value("estado").toString() == "CERRADA";
        Toast::showToast(this, cerrada ? "Faltantes recibidos en bodega: la incidencia quedó cerrada"
                                       : QString("Llegada registrada; aún faltan %1 pzs").arg(inc.value("piezas_pendientes").toInt()),
                         Toast::Success, 5000);
        mostrar(inc);
        cargar();
    });
}
