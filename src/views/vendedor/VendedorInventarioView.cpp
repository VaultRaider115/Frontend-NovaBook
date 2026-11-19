#include "VendedorInventarioView.h"
#include "ApiClient.h"
#include "EanClassifier.h"
#include "Theme.h"
#include "Toast.h"
#include "Ui.h"
#include "ProductoBuscador.h"
#include <QHBoxLayout>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QSpinBox>
#include <QVBoxLayout>

VendedorInventarioView::VendedorInventarioView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(18);
    layout->addWidget(Ui::pageHeader("Vendedor · Fin de mes", "Inventario Mensual del Piso de Venta",
        "Escanea cada libro y revista de la tienda. Al cerrar, el Jefe revisa las diferencias y lo que va a devolución.", this));

    auto *estadoCard = Ui::card(this);
    auto *el = new QHBoxLayout(estadoCard);
    el->setContentsMargins(20, 16, 20, 16);
    m_lblEstado = new QLabel(estadoCard);
    m_lblEstado->setWordWrap(true);
    el->addWidget(m_lblEstado, 1);
    m_btnIniciar = Ui::button("Iniciar inventario", "primary", estadoCard, "plus-dark");
    el->addWidget(m_btnIniciar);
    layout->addWidget(estadoCard);

    m_panelConteo = new QWidget(this);
    auto *pl = new QVBoxLayout(m_panelConteo);
    pl->setContentsMargins(0, 0, 0, 0);
    pl->setSpacing(18);

    auto *scanCard = Ui::card(m_panelConteo);
    auto *sl = new QVBoxLayout(scanCard);
    sl->setContentsMargins(20, 20, 20, 20);
    m_txtEan = new QLineEdit(scanCard);
    new ProductoBuscador(m_txtEan);
    m_txtEan->setPlaceholderText("Escanea el código de barras o escribe el título (cada uno suma 1)");
    m_txtEan->addAction(QIcon(":/icons/scan-gold.svg"), QLineEdit::LeadingPosition);
    sl->addWidget(Ui::field("EAN-13 o título", m_txtEan, scanCard));
    pl->addWidget(scanCard);

    m_tabla = Ui::table({"EAN", "Título", "Contadas", "Devolución", ""}, 1, m_panelConteo, 52);
    pl->addWidget(m_tabla, 1);

    auto *foot = Ui::card(m_panelConteo);
    auto *fl = new QHBoxLayout(foot);
    fl->setContentsMargins(20, 16, 20, 16);
    m_lblTotal = new QLabel(foot);
    m_lblTotal->setObjectName("Muted");
    fl->addWidget(m_lblTotal);
    fl->addStretch();
    m_btnCerrar = Ui::button("Cerrar conteo", "primary", foot, "check-dark");
    m_btnCerrar->setStyleSheet("QPushButton { padding: 14px 26px; }");
    fl->addWidget(m_btnCerrar);
    pl->addWidget(foot);
    layout->addWidget(m_panelConteo, 1);

    connect(m_btnIniciar, &QPushButton::clicked, this, [this]() {
        m_btnIniciar->setEnabled(false);
        ApiClient::instance()->abrirInventario([this](bool ok, const QJsonValue &, const QString &msg) {
            m_btnIniciar->setEnabled(true);
            if (!ok) Toast::showToast(this, "No se pudo iniciar: " + msg, Toast::Error);
            cargar();
        });
    });
    connect(m_txtEan, &QLineEdit::returnPressed, this, &VendedorInventarioView::escanear);
    connect(m_btnCerrar, &QPushButton::clicked, this, &VendedorInventarioView::cerrar);
    cargar();
}

void VendedorInventarioView::cargar() {
    ApiClient::instance()->inventarioActual([this](bool ok, const QJsonValue &data, const QString &msg) {
        m_conteos.clear();
        m_idInventario = 0;
        if (!ok) {
            m_lblEstado->setText("No se pudo consultar el inventario: " + msg);
        } else if (data.isObject()) {
            const QJsonObject d = data.toObject();
            const QJsonObject r = d.value("resumen").toObject();
            m_idInventario = r.value("id_inventario").toVariant().toLongLong();
            m_lblEstado->setText(QString("<b>Inventario #%1 en curso</b> · iniciado %2 por %3")
                .arg(m_idInventario).arg(r.value("fecha_inicio").toString(), r.value("contado_por").toString().toHtmlEscaped()));
            for (const auto &v : d.value("items").toArray()) {
                const QJsonObject o = v.toObject();
                m_conteos.append({o.value("codigo_ean").toVariant().toLongLong(), o.value("titulo").toString(),
                                  o.value("cantidad_contada").toInt(), o.value("motivo_devolucion").toString()});
            }
        } else {
            m_lblEstado->setText("No hay un inventario en curso. Inícialo al terminar el día, a fin de mes.");
        }
        m_btnIniciar->setVisible(ok && m_idInventario == 0);
        m_panelConteo->setVisible(m_idInventario > 0);
        pintar();
        if (m_idInventario > 0) m_txtEan->setFocus();
    });
}

void VendedorInventarioView::escanear() {
    const QString eanStr = m_txtEan->text().trimmed();
    m_txtEan->clear();
    const auto res = EanClassifier::clasificar(eanStr);
    if (!res.esValido) {
        Toast::showToast(this, res.mensaje, Toast::Warning);
        return;
    }
    const qint64 ean = eanStr.toLongLong();
    for (const auto &c : std::as_const(m_conteos)) {
        if (c.ean == ean) {
            fijarCantidad(ean, c.cantidad + 1);
            return;
        }
    }
    // Primer escaneo: se busca el título para mostrarlo.
    const QString tipo = res.tipo == EanClassifier::Tipo::Libro ? "libro" : "revista";
    ApiClient::instance()->consultarExistencias(eanStr, tipo, 0, [this, ean](bool ok, const QJsonValue &data, const QString &) {
        QString titulo = QString("EAN %1").arg(ean);
        if (ok && data.isArray() && !data.toArray().isEmpty())
            titulo = data.toArray().first().toObject().value("titulo").toString(titulo);
        m_conteos.prepend({ean, titulo, 0, QString()});
        fijarCantidad(ean, 1);
    });
}

void VendedorInventarioView::fijarCantidad(qint64 ean, int cantidad) {
    ApiClient::instance()->registrarConteo(m_idInventario, ean, cantidad,
        [this, ean, cantidad](bool ok, const QJsonValue &, const QString &msg) {
            if (!ok) {
                Toast::showToast(this, "No se registró el conteo: " + msg, Toast::Error);
                cargar();   // vuelve a lo que tiene el servidor
                return;
            }
            int antes = 0;
            for (auto &c : m_conteos) if (c.ean == ean) { antes = c.cantidad; c.cantidad = cantidad; }
            pintar();
            // El motivo depende de lo contado (un título en 0 no se separa): se vuelve a preguntar.
            if ((antes == 0) != (cantidad == 0)) actualizarMotivos();
        });
}

// Pide al servidor qué títulos contados van a devolución (solicitud de la
// editorial o añejado), sin cambiar el orden de la lista.
void VendedorInventarioView::actualizarMotivos() {
    ApiClient::instance()->inventarioActual([this](bool ok, const QJsonValue &data, const QString &) {
        if (!ok || !data.isObject()) return;
        QHash<qint64, QString> motivos;
        for (const auto &v : data.toObject().value("items").toArray()) {
            const QJsonObject o = v.toObject();
            motivos.insert(o.value("codigo_ean").toVariant().toLongLong(), o.value("motivo_devolucion").toString());
        }
        bool nuevo = false;
        for (auto &c : m_conteos) {
            const QString m = motivos.value(c.ean);
            if (m != c.motivo && !m.isEmpty()) nuevo = true;
            c.motivo = m;
        }
        pintar();
        if (nuevo) Toast::showToast(this, "Este título va a devolución: sepáralo del piso de venta", Toast::Warning);
    });
}

void VendedorInventarioView::pintar() {
    m_tabla->clearContents();
    m_tabla->setRowCount(m_conteos.size());
    int piezas = 0, separar = 0;
    for (int i = 0; i < m_conteos.size(); ++i) {
        const Conteo &c = m_conteos.at(i);
        piezas += c.cantidad;
        // Paso 7: lo que va a devolución se separa; el título sale en rojo.
        const bool devolver = !c.motivo.isEmpty();
        if (devolver) ++separar;
        Ui::cellText(m_tabla, i, 0, QString::number(c.ean), true, false, devolver ? Theme::Error : QString());
        auto *tit = Ui::cellText(m_tabla, i, 1, c.titulo, false, true, devolver ? Theme::Error : QString());
        if (devolver) tit->setToolTip("Va a devolución: sepáralo del piso de venta");
        if (c.motivo == "SOLICITUD_EDITORIAL") Ui::cellWidget(m_tabla, i, 3, Ui::chip("Separar · pidió la editorial", "danger"));
        else if (c.motivo == "ANEJAMIENTO") Ui::cellWidget(m_tabla, i, 3, Ui::chip("Separar · añejado", "danger"));
        else Ui::cellText(m_tabla, i, 3, "—", false, false, Theme::TextMuted);
        // Corregir a mano (p. ej. si se escaneó de más).
        auto *spn = new QSpinBox();
        spn->setRange(0, 99999);
        spn->setValue(c.cantidad);
        spn->setFixedSize(90, 34);
        spn->setStyleSheet("QSpinBox { padding: 0px 10px; min-height: 0px; }");   // cabe en la fila
        spn->setAccessibleName(QString("Piezas contadas de %1").arg(c.titulo));
        const qint64 ean = c.ean;
        connect(spn, &QSpinBox::editingFinished, this, [this, spn, ean]() {
            for (const auto &x : std::as_const(m_conteos))
                if (x.ean == ean && x.cantidad != spn->value()) fijarCantidad(ean, spn->value());
        });
        Ui::cellWidget(m_tabla, i, 2, spn);
        auto *quitar = Ui::iconButton("trash", "Poner en 0");
        connect(quitar, &QPushButton::clicked, this, [this, ean]() { fijarCantidad(ean, 0); });
        Ui::cellWidget(m_tabla, i, 4, quitar);
    }
    QString txt = QString("%1 título(s) · <b style=\"color:#FFFFFF;\">%2</b> pieza(s) contadas")
                      .arg(m_conteos.size()).arg(piezas);
    if (separar > 0)
        txt += QString(" · <b style=\"color:%1;\">%2 título(s) para separar (devolución)</b>").arg(Theme::Error).arg(separar);
    m_lblTotal->setText(txt);
}

void VendedorInventarioView::cerrar() {
    if (QMessageBox::question(this, "Cerrar conteo",
            "¿Ya escaneaste todo el piso de venta?\n\n"
            "Lo que no se escaneó cuenta como 0 piezas. Después de cerrar ya no se puede contar.",
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    m_btnCerrar->setEnabled(false);
    ApiClient::instance()->cerrarInventario(m_idInventario, [this](bool ok, const QJsonValue &data, const QString &msg) {
        m_btnCerrar->setEnabled(true);
        if (!ok || !data.isObject()) {
            Toast::showToast(this, "No se pudo cerrar: " + msg, Toast::Error);
            return;
        }
        const QJsonObject r = data.toObject().value("resumen").toObject();
        Toast::showToast(this, QString("Inventario #%1 cerrado: %2 con diferencia, %3 a devolución. Queda para revisión del Jefe.")
            .arg(r.value("id_inventario").toVariant().toLongLong())
            .arg(r.value("con_diferencia").toInt()).arg(r.value("a_devolucion").toInt()), Toast::Success, 7000);
        cargar();
    });
}
