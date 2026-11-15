#include "BodegaComprasView.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Toast.h"
#include "EanClassifier.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "Ui.h"
#include <QIcon>
#include <QHeaderView>
#include <QFrame>
#include <QMessageBox>
#include "Documentos.h"

BodegaComprasView::BodegaComprasView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(18);
    // Diseño 6: la segunda parte del título va atenuada.
    layout->addWidget(Ui::pageHeader("Bodega · Compras",
        QString("Recepción de Mercancía <span style=\"color:%1; font-weight:400;\">— Compras a Proveedor</span>")
            .arg(Theme::TextMuted), "", this));

    // Datos de factura: proveedor + número de factura / observaciones
    auto *headerCard = Ui::card(this);
    auto *hl = new QHBoxLayout(headerCard);
    hl->setContentsMargins(20, 20, 20, 20);
    hl->setSpacing(20);
    m_comboProveedor = new QComboBox(headerCard);
    m_txtObs = new QLineEdit(headerCard);
    m_txtObs->setPlaceholderText("Ej. F-1024");
    m_txtObs->setMaxLength(30);
    m_txtObs->addAction(QIcon(":/icons/receipt.svg"), QLineEdit::LeadingPosition);
    hl->addWidget(Ui::field("Proveedor Oficial", m_comboProveedor, headerCard), 1);
    hl->addWidget(Ui::field("Número de Factura", m_txtObs, headerCard), 1);
    layout->addWidget(headerCard);

    // Agregar producto
    auto *addCard = Ui::card(this);
    auto *cardLayout = new QVBoxLayout(addCard);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(12);

    auto *addLayout = new QHBoxLayout();
    addLayout->setSpacing(16);
    m_txtEan = new QLineEdit(addCard);
    m_txtEan->setPlaceholderText("9780000000000");
    m_txtEan->addAction(QIcon(":/icons/scan-gold.svg"), QLineEdit::LeadingPosition);
    addLayout->addWidget(Ui::field("Código EAN-13", m_txtEan, addCard), 1);

    // Diagrama, paso 1: si llegan menos piezas de las facturadas se abre una incidencia.
    m_spnCantidad = new QSpinBox(addCard);
    m_spnCantidad->setRange(1, 9999);
    m_spnCantidad->setValue(1);
    m_spnCantidad->setAccessibleName("Piezas facturadas");
    addLayout->addWidget(Ui::field("Facturadas", Ui::stepper(m_spnCantidad, addCard), addCard));

    m_spnRecibidas = new QSpinBox(addCard);
    m_spnRecibidas->setRange(0, 1);
    m_spnRecibidas->setValue(1);
    m_spnRecibidas->setAccessibleName("Piezas que llegaron");
    m_spnRecibidas->setToolTip("Piezas que llegaron en el paquete; si son menos que las facturadas se abre una incidencia");
    addLayout->addWidget(Ui::field("Llegaron", Ui::stepper(m_spnRecibidas, addCard), addCard));
    // "Llegaron" sigue a "Facturadas" mientras no se haya capturado algo distinto.
    connect(m_spnCantidad, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int fac) {
        const bool seguia = m_spnRecibidas->value() == m_spnRecibidas->maximum();
        m_spnRecibidas->setMaximum(fac);
        if (seguia) m_spnRecibidas->setValue(fac);
    });

    m_spnCosto = new QDoubleSpinBox(addCard);
    m_spnCosto->setRange(0.0, 99999.0);
    m_spnCosto->setValue(150.0);
    m_spnCosto->setPrefix("$ ");
    m_spnCosto->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_spnCosto->setFixedWidth(130);
    addLayout->addWidget(Ui::field("Costo unitario", m_spnCosto, addCard));

    m_btnAgregar = Ui::button("Añadir a Compra", "tint", addCard, "plus-gold");
    addLayout->addWidget(m_btnAgregar, 0, Qt::AlignBottom);
    cardLayout->addLayout(addLayout);

    m_lblEanStatus = new QLabel("Ingresa el código EAN-13 para clasificar automáticamente como Libro o Revista", addCard);
    m_lblEanStatus->setObjectName("Muted");
    cardLayout->addWidget(m_lblEanStatus);
    layout->addWidget(addCard);

    // Partidas de la compra
    m_tableCart = Ui::table({"EAN", "Tipo", "Descripción", "Llegaron / Fact.", "Costo unit.", "Subtotal", "Acción"}, 2, this, 56);
    layout->addWidget(m_tableCart, 1);

    // Liquidación
    auto *bottomCard = Ui::card(this);
    auto *bottomLayout = new QHBoxLayout(bottomCard);
    bottomLayout->setContentsMargins(20, 20, 20, 20);
    bottomLayout->setSpacing(16);

    m_btnVaciar = Ui::button("Vaciar Orden", "ghost", bottomCard, "trash");
    bottomLayout->addWidget(m_btnVaciar);
    bottomLayout->addStretch();

    m_lblTotalArticulos = new QLabel("Piezas Totales: 0", bottomCard);
    m_lblTotalArticulos->setObjectName("Muted");
    bottomLayout->addWidget(m_lblTotalArticulos);

    m_lblTotalPagar = new QLabel("Total Factura: $0.00", bottomCard);
    m_lblTotalPagar->setObjectName("Gold");
    m_lblTotalPagar->setStyleSheet("font-size: 18px; font-weight: 600;");
    bottomLayout->addWidget(m_lblTotalPagar);

    m_btnProcesar = Ui::button("Registrar Compra e Incrementar Bodega", "primary", bottomCard, "check-dark");
    m_btnProcesar->setStyleSheet("QPushButton { padding: 16px 28px; }");
    bottomLayout->addWidget(m_btnProcesar);
    layout->addWidget(bottomCard);

    connect(m_txtEan, &QLineEdit::textChanged, this, &BodegaComprasView::onEanChanged);
    connect(m_btnAgregar, &QPushButton::clicked, this, &BodegaComprasView::onAgregarArticulo);
    connect(m_txtEan, &QLineEdit::returnPressed, this, &BodegaComprasView::onAgregarArticulo);
    connect(m_btnVaciar, &QPushButton::clicked, this, &BodegaComprasView::limpiarCarrito);
    connect(m_btnProcesar, &QPushButton::clicked, this, &BodegaComprasView::onRegistrarCompra);

    cargarProveedores();
}

void BodegaComprasView::onEanChanged(const QString &text) {
    EanClassifier::aplicarAControles(text, m_lblEanStatus);
}

void BodegaComprasView::cargarProveedores() {
    ApiClient::instance()->listarProveedores([this](bool ok, const QJsonValue &data, const QString &) {
        if (!ok || !data.isArray()) return;

        m_comboProveedor->clear();
        m_proveedores.clear();
        QJsonArray arr = data.toArray();

        for (int i = 0; i < arr.size(); ++i) {
            ProveedorDto p = ProveedorDto::fromJson(arr.at(i).toObject());
            m_proveedores.append(p);
            m_comboProveedor->addItem(QString("%1 (RFC: %2)").arg(p.nombre_proveedor, p.rfc), p.id_proveedor);
        }
    });
}

void BodegaComprasView::onAgregarArticulo() {
    QString eanStr = m_txtEan->text().trimmed();
    auto eanRes = EanClassifier::clasificar(eanStr);
    if (!eanRes.esValido) {
        Toast::showToast(this, eanRes.mensaje, Toast::Warning);
        return;
    }

    qint64 ean = eanStr.toLongLong();
    int cant = m_spnCantidad->value();
    int recibidas = qMin(m_spnRecibidas->value(), cant);
    double costo = m_spnCosto->value();
    QString tipo = (eanRes.tipo == EanClassifier::Tipo::Libro ? "libro" : "revista");

    // Consult existing item title
    ApiClient::instance()->consultarExistencias(eanStr, tipo, 0, [this, ean, cant, recibidas, costo, tipo](bool ok, const QJsonValue &data, const QString &) {
        QString titulo = QString("Artículo %1").arg(ean);
        if (ok && data.isArray() && !data.toArray().isEmpty()) {
            QJsonObject obj = data.toArray().first().toObject();
            titulo = obj.value("titulo").toString(titulo);
        }

        // Check if item is already in list
        bool found = false;
        for (auto &item : m_items) {
            if (item.ean == ean) {
                item.cantidad += cant;
                item.recibidas += recibidas;
                item.costo_unitario = costo;
                item.subtotal = item.cantidad * item.costo_unitario;
                found = true;
                break;
            }
        }

        if (!found) {
            CompraCartItem ci;
            ci.ean = ean;
            ci.tipo = tipo;
            ci.nombre = titulo;
            ci.cantidad = cant;
            ci.recibidas = recibidas;
            ci.costo_unitario = costo;
            ci.subtotal = cant * costo;
            m_items.append(ci);
        }

        m_txtEan->clear();
        m_spnCantidad->setValue(1);
        m_spnRecibidas->setValue(1);
        m_txtEan->setFocus();
        recalcularTotales();
    });
}

void BodegaComprasView::onQuitarFila(int row) {
    if (row >= 0 && row < m_items.size()) {
        m_items.removeAt(row);
        recalcularTotales();
    }
}

void BodegaComprasView::limpiarCarrito() {
    m_items.clear();
    m_txtObs->clear();
    recalcularTotales();
}

void BodegaComprasView::recalcularTotales() {
    m_tableCart->clearContents();
    m_tableCart->setRowCount(m_items.size());

    int totalPiezas = 0, totalRecibidas = 0;
    double totalDinero = 0.0;

    for (int i = 0; i < m_items.size(); ++i) {
        const auto &item = m_items.at(i);
        totalPiezas += item.cantidad;
        totalRecibidas += item.recibidas;
        totalDinero += item.subtotal;

        Ui::cellText(m_tableCart, i, 0, QString::number(item.ean), true);
        Ui::cellWidget(m_tableCart, i, 1, Ui::tipoChip(item.tipo));
        Ui::cellText(m_tableCart, i, 2, item.nombre);
        if (item.recibidas < item.cantidad)
            Ui::cellText(m_tableCart, i, 3, QString("%1 / %2").arg(item.recibidas).arg(item.cantidad), false, true, Theme::Warning)
                ->setToolTip(QString("Faltan %1: se abrirá una incidencia").arg(item.cantidad - item.recibidas));
        else
            Ui::cellText(m_tableCart, i, 3, QString::number(item.cantidad));
        Ui::cellText(m_tableCart, i, 4, Ui::money(item.costo_unitario), false, false, Theme::TextMuted);
        Ui::cellText(m_tableCart, i, 5, Ui::money(item.subtotal), false, true);

        auto *btnQuitar = Ui::iconButton("trash", "Quitar partida");
        connect(btnQuitar, &QPushButton::clicked, this, [this, i]() { onQuitarFila(i); });
        Ui::cellWidget(m_tableCart, i, 6, btnQuitar);
    }

    m_lblTotalArticulos->setText(totalRecibidas < totalPiezas
        ? QString("Piezas: %1 de %2 <span style=\"color:%3;\">(faltan %4)</span>")
              .arg(totalRecibidas).arg(totalPiezas).arg(Theme::Warning).arg(totalPiezas - totalRecibidas)
        : QString("Piezas Totales: %1").arg(totalPiezas));
    m_lblTotalPagar->setText(QString("Total Factura: %1").arg(Ui::money(totalDinero)));
}

void BodegaComprasView::onRegistrarCompra() {
    if (m_comboProveedor->currentIndex() < 0) {
        Toast::showToast(this, "Selecciona un proveedor válido", Toast::Warning);
        return;
    }
    if (m_items.isEmpty()) {
        Toast::showToast(this, "La orden de compra no tiene artículos", Toast::Warning);
        return;
    }

    int idProveedor = m_comboProveedor->currentData().toInt();
    QJsonObject req;
    req["id_proveedor"] = idProveedor;
    if (!m_txtObs->text().trimmed().isEmpty()) {
        req["factura"] = m_txtObs->text().trimmed();
    }

    int recibidasTotal = 0;
    for (const auto &it : std::as_const(m_items)) recibidasTotal += it.recibidas;
    if (recibidasTotal == 0) {
        Toast::showToast(this, "No llegó ninguna pieza: no hay compra que registrar", Toast::Warning);
        return;
    }

    QJsonArray itemsArr;
    for (const auto &it : m_items) {
        QJsonObject obj;
        obj["codigo_ean"] = it.ean;
        obj["tipo_producto"] = it.tipo;
        obj["cantidad"] = it.recibidas;
        obj["cantidad_facturada"] = it.cantidad;
        obj["costo_unitario"] = it.costo_unitario;
        itemsArr.append(obj);
    }
    req["items"] = itemsArr;

    ApiClient::instance()->registrarCompra(req, [this](bool ok, const QJsonValue &data, const QString &msg) {
        if (!ok || !data.isObject()) {
            Toast::showToast(this, "Error al registrar compra: " + msg, Toast::Error);
            return;
        }

        CompraResponse cr = CompraResponse::fromJson(data.toObject());
        limpiarCarrito();
        const QJsonObject inc = data.toObject().value("incidencia").toObject();
        if (inc.isEmpty()) {
            Toast::showToast(this, QString("Compra #%1 registrada con éxito. Inventario en bodega actualizado.").arg(cr.id_compra), Toast::Success);
            return;
        }
        // Diagrama, paso 1: paquete incompleto → incidencia y correo al proveedor.
        const QString para = inc.value("correo").toObject().value("para").toString();
        const auto r = QMessageBox::question(this, "Paquete incompleto",
            QString("La compra #%1 se registró con lo que llegó y se abrió la incidencia #%2: faltan %3 pieza(s) de %4.\n\n"
                    "¿Abrir el correo para pedir al proveedor los artículos restantes%5?\n"
                    "Cuando lleguen, regístralos en Incidencias de Compra.")
                .arg(cr.id_compra).arg(inc.value("id_incidencia").toInt()).arg(inc.value("piezas_pendientes").toInt())
                .arg(inc.value("proveedor").toString(), para.isEmpty() ? QString() : " (" + para + ")"),
            QMessageBox::Yes | QMessageBox::No);
        if (r == QMessageBox::Yes) Documentos::abrirCorreo(this, inc.value("correo").toObject());
        else Toast::showToast(this, QString("Incidencia #%1 abierta; puedes avisar después desde Incidencias de Compra")
                                        .arg(inc.value("id_incidencia").toInt()), Toast::Warning, 6000);
    });
}
