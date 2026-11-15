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

    m_spnCantidad = new QSpinBox(addCard);
    m_spnCantidad->setRange(1, 9999);
    m_spnCantidad->setValue(1);
    addLayout->addWidget(Ui::field("Cantidad", Ui::stepper(m_spnCantidad, addCard), addCard));

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
    m_tableCart = Ui::table({"EAN", "Tipo", "Descripción", "Cantidad", "Costo unit.", "Subtotal", "Acción"}, 2, this, 56);
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
    double costo = m_spnCosto->value();
    QString tipo = (eanRes.tipo == EanClassifier::Tipo::Libro ? "libro" : "revista");

    // Consult existing item title
    ApiClient::instance()->consultarExistencias(eanStr, tipo, 0, [this, ean, cant, costo, tipo](bool ok, const QJsonValue &data, const QString &) {
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
            ci.costo_unitario = costo;
            ci.subtotal = cant * costo;
            m_items.append(ci);
        }

        m_txtEan->clear();
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

    int totalPiezas = 0;
    double totalDinero = 0.0;

    for (int i = 0; i < m_items.size(); ++i) {
        const auto &item = m_items.at(i);
        totalPiezas += item.cantidad;
        totalDinero += item.subtotal;

        Ui::cellText(m_tableCart, i, 0, QString::number(item.ean), true);
        Ui::cellWidget(m_tableCart, i, 1, Ui::tipoChip(item.tipo));
        Ui::cellText(m_tableCart, i, 2, item.nombre);
        Ui::cellText(m_tableCart, i, 3, QString::number(item.cantidad));
        Ui::cellText(m_tableCart, i, 4, Ui::money(item.costo_unitario), false, false, Theme::TextMuted);
        Ui::cellText(m_tableCart, i, 5, Ui::money(item.subtotal), false, true);

        auto *btnQuitar = Ui::iconButton("trash", "Quitar partida");
        connect(btnQuitar, &QPushButton::clicked, this, [this, i]() { onQuitarFila(i); });
        Ui::cellWidget(m_tableCart, i, 6, btnQuitar);
    }

    m_lblTotalArticulos->setText(QString("Piezas Totales: %1").arg(totalPiezas));
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

    QJsonArray itemsArr;
    for (const auto &it : m_items) {
        QJsonObject obj;
        obj["codigo_ean"] = it.ean;
        obj["tipo_producto"] = it.tipo;
        obj["cantidad"] = it.cantidad;
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
        Toast::showToast(this, QString("Compra #%1 registrada con éxito. Inventario en bodega actualizado.").arg(cr.id_compra), Toast::Success);
    });
}
