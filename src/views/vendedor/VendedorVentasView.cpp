#include "VendedorVentasView.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Toast.h"
#include "EanClassifier.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDialog>
#include <QMessageBox>
#include <QFrame>
#include <QTimer>
#include <QDateTime>
#include <QLocale>
#include <QIcon>
#include "Ui.h"

VendedorVentasView::VendedorVentasView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(18);
    layout->addWidget(Ui::pageHeader("Vendedor · Caja registradora", "Punto de Venta — Registro de Venta en Mostrador", "", this));

    // Cliente + fecha/hora en vivo
    auto *clienteCard = Ui::card(this);
    auto *cl = new QHBoxLayout(clienteCard);
    cl->setContentsMargins(20, 20, 20, 20);
    cl->setSpacing(20);
    m_txtCliente = new QLineEdit(clienteCard);
    m_txtCliente->setPlaceholderText("Ej. Ignacio Zaragoza Seguín");
    auto *lblCliente = Ui::label("Nombre del Cliente", "Gold", clienteCard);
    lblCliente->setStyleSheet("font-weight: 600;");
    auto *clienteBox = new QWidget(clienteCard);
    auto *clienteLay = new QVBoxLayout(clienteBox);
    clienteLay->setContentsMargins(0, 0, 0, 0);
    clienteLay->setSpacing(8);
    clienteLay->addWidget(lblCliente);
    clienteLay->addWidget(m_txtCliente);
    cl->addWidget(clienteBox, 1);

    m_lblFecha = new QLabel(clienteCard);
    m_lblFecha->setObjectName("Hint");
    cl->addWidget(m_lblFecha, 0, Qt::AlignBottom);
    layout->addWidget(clienteCard);

    auto *tick = new QTimer(this);
    auto actualizarHora = [this]() {
        m_lblFecha->setText(QLocale(QLocale::Spanish).toString(QDateTime::currentDateTime(), "d 'de' MMMM, yyyy. HH:mm:ss"));
    };
    connect(tick, &QTimer::timeout, this, actualizarHora);
    actualizarHora();
    tick->start(1000);

    // Escaneo / alta al carrito
    auto *addCard = Ui::card(this);
    auto *cardLayout = new QVBoxLayout(addCard);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(10);
    auto *addLayout = new QHBoxLayout();
    addLayout->setSpacing(16);

    m_txtEan = new QLineEdit(addCard);
    m_txtEan->setPlaceholderText("Escriba el código de barras");
    m_txtEan->addAction(QIcon(":/icons/scan-gold.svg"), QLineEdit::LeadingPosition);
    addLayout->addWidget(Ui::field("Código EAN-13 / ISSN-13", m_txtEan, addCard), 1);

    m_spnCantidad = new QSpinBox(addCard);
    m_spnCantidad->setRange(1, 999);
    m_spnCantidad->setValue(1);
    addLayout->addWidget(Ui::field("Cantidad", Ui::stepper(m_spnCantidad, addCard), addCard));

    m_btnAgregar = Ui::button("Añadir al Carrito", "tint", addCard, "plus-gold");
    m_btnAgregar->setStyleSheet("QPushButton { padding: 13px 24px; }");
    addLayout->addWidget(m_btnAgregar, 0, Qt::AlignBottom);
    cardLayout->addLayout(addLayout);

    m_lblEanStatus = new QLabel("Ingresa el código EAN-13 para clasificar automáticamente como Libro o Revista", addCard);
    m_lblEanStatus->setObjectName("Muted");
    cardLayout->addWidget(m_lblEanStatus);
    layout->addWidget(addCard);

    // Carrito
    m_tableCart = Ui::table({"EAN", "Tipo", "Título / Descripción", "Cantidad", "Precio unit.", "Subtotal", "Acción"}, 2, this, 60);
    layout->addWidget(m_tableCart, 1);

    // Resumen: método de pago, vaciar, artículos totales
    auto *summary = Ui::card(this);
    auto *sl = new QVBoxLayout(summary);
    sl->setContentsMargins(20, 20, 20, 20);
    sl->setSpacing(16);

    auto *payRow = new QHBoxLayout();
    payRow->setSpacing(8);
    payRow->addWidget(Ui::label("Método de pago", "FieldLabel", summary));
    for (const QString &metodo : {"Efectivo", "Tarjeta", "Transferencia"}) {
        auto *b = Ui::button(metodo, "pay", summary);
        Ui::setCurrent(b, metodo == m_metodoPago);
        connect(b, &QPushButton::clicked, this, [this, metodo]() {
            m_metodoPago = metodo;
            for (auto *x : std::as_const(m_btnsPago)) Ui::setCurrent(x, x->text() == metodo);
        });
        m_btnsPago << b;
        payRow->addWidget(b);
    }
    payRow->addStretch();
    m_btnVaciar = Ui::button("Vaciar carrito", "ghost", summary, "trash");
    payRow->addWidget(m_btnVaciar);
    m_lblTotalArticulos = new QLabel("Artículos totales: 0", summary);
    m_lblTotalArticulos->setObjectName("Hint");
    payRow->addWidget(m_lblTotalArticulos);
    sl->addLayout(payRow);

    auto *totalRow = new QHBoxLayout();
    totalRow->setSpacing(16);
    auto *totalBox = new QFrame(summary);
    totalBox->setObjectName("Highlight");
    auto *tbl = new QHBoxLayout(totalBox);
    tbl->setContentsMargins(24, 16, 24, 16);
    tbl->addWidget(Ui::label("Total a cobrar", "Muted", totalBox));
    tbl->addStretch();
    m_lblTotalPagar = new QLabel("$0.00", totalBox);
    m_lblTotalPagar->setStyleSheet(QString("font-size: 44px; font-weight: 700; color: %1;").arg(Theme::Accent));
    tbl->addWidget(m_lblTotalPagar);
    totalRow->addWidget(totalBox, 1);

    m_btnProcesar = Ui::button("Procesar Venta e Imprimir Ticket", "primary", summary);
    m_btnProcesar->setStyleSheet("QPushButton { padding: 26px 32px; font-size: 18px; font-weight: 700; }");
    totalRow->addWidget(m_btnProcesar, 1);
    sl->addLayout(totalRow);
    layout->addWidget(summary);

    connect(m_txtEan, &QLineEdit::textChanged, this, &VendedorVentasView::onEanChanged);
    connect(m_btnAgregar, &QPushButton::clicked, this, &VendedorVentasView::onAgregarAlCarrito);
    connect(m_txtEan, &QLineEdit::returnPressed, this, &VendedorVentasView::onAgregarAlCarrito);
    connect(m_btnVaciar, &QPushButton::clicked, this, &VendedorVentasView::limpiarCarrito);
    connect(m_btnProcesar, &QPushButton::clicked, this, &VendedorVentasView::onProcesarVenta);
}

void VendedorVentasView::onEanChanged(const QString &text) {
    EanClassifier::aplicarAControles(text, m_lblEanStatus);
}

void VendedorVentasView::onAgregarAlCarrito() {
    QString eanStr = m_txtEan->text().trimmed();
    auto eanRes = EanClassifier::clasificar(eanStr);
    if (!eanRes.esValido) {
        Toast::showToast(this, eanRes.mensaje, Toast::Warning);
        return;
    }

    qint64 ean = eanStr.toLongLong();
    int cant = m_spnCantidad->value();
    QString tipo = (eanRes.tipo == EanClassifier::Tipo::Libro ? "libro" : "revista");

    // Consult existences to get title and price
    ApiClient::instance()->consultarExistencias(eanStr, tipo, 0, [this, ean, cant, tipo](bool ok, const QJsonValue &data, const QString &) {
        QString titulo = QString("Artículo %1").arg(ean);
        double precio = 0.0;

        if (ok && data.isArray() && !data.toArray().isEmpty()) {
            QJsonObject obj = data.toArray().first().toObject();
            titulo = obj.value("titulo").toString(titulo);
            precio = obj.value("precio").toDouble(0.0);
        }

        // Check if already in cart
        bool found = false;
        for (auto &item : m_cart) {
            if (item.ean == ean) {
                item.cantidad += cant;
                item.subtotal = item.cantidad * item.precio;
                found = true;
                break;
            }
        }

        if (!found) {
            CartItem ci;
            ci.ean = ean;
            ci.tipo = tipo;
            ci.nombre = titulo;
            ci.cantidad = cant;
            ci.precio = precio;
            ci.subtotal = cant * precio;
            m_cart.append(ci);
        }

        m_txtEan->clear();
        m_spnCantidad->setValue(1);
        m_txtEan->setFocus();
        recalcularTotales();
    });
}

void VendedorVentasView::onQuitarFila(int row) {
    if (row >= 0 && row < m_cart.size()) {
        m_cart.removeAt(row);
        recalcularTotales();
    }
}

void VendedorVentasView::limpiarCarrito() {
    m_cart.clear();
    m_txtCliente->clear();
    recalcularTotales();
}

void VendedorVentasView::recalcularTotales() {
    m_tableCart->clearContents();
    m_tableCart->setRowCount(m_cart.size());

    int totalPiezas = 0;
    double totalDinero = 0.0;

    for (int i = 0; i < m_cart.size(); ++i) {
        const auto &item = m_cart.at(i);
        totalPiezas += item.cantidad;
        totalDinero += item.subtotal;

        Ui::cellText(m_tableCart, i, 0, QString::number(item.ean), true);
        Ui::cellWidget(m_tableCart, i, 1, Ui::tipoChip(item.tipo));
        Ui::cellText(m_tableCart, i, 2, item.nombre, false, true);
        Ui::cellText(m_tableCart, i, 3, QString("%1 %2").arg(item.cantidad).arg(item.cantidad == 1 ? "pza" : "pzas"));
        Ui::cellText(m_tableCart, i, 4, item.precio > 0 ? Ui::money(item.precio) : "Al cobrar", false, false, Theme::TextMuted);
        Ui::cellText(m_tableCart, i, 5, item.precio > 0 ? Ui::money(item.subtotal) : "—", false, true);

        auto *btnQuitar = Ui::iconButton("trash", "Quitar del carrito");
        connect(btnQuitar, &QPushButton::clicked, this, [this, i]() { onQuitarFila(i); });
        Ui::cellWidget(m_tableCart, i, 6, btnQuitar);
    }

    m_lblTotalArticulos->setText(QString("Artículos totales: %1").arg(totalPiezas));
    m_lblTotalPagar->setText(Ui::money(totalDinero));
}

void VendedorVentasView::onProcesarVenta() {
    QString cliente = m_txtCliente->text().trimmed();
    if (cliente.isEmpty()) {
        Toast::showToast(this, "El nombre del cliente es obligatorio para registrar la venta", Toast::Warning);
        m_txtCliente->setFocus();
        return;
    }

    if (m_cart.isEmpty()) {
        Toast::showToast(this, "El carrito de compra está vacío", Toast::Warning);
        return;
    }

    QJsonArray itemsArr;
    for (const auto &item : m_cart) {
        QJsonObject it;
        it["codigo_ean"] = item.ean;
        it["tipo_producto"] = item.tipo;
        it["cantidad"] = item.cantidad;
        itemsArr.append(it);
    }

    ApiClient::instance()->registrarVenta(cliente, itemsArr, [this, cliente](bool ok, const QJsonValue &data, const QString &msg) {
        if (!ok || !data.isObject()) {
            Toast::showToast(this, "Error al procesar venta: " + msg, Toast::Error);
            return;
        }

        VentaResponse vr = VentaResponse::fromJson(data.toObject());

        // Display Receipt Dialog with Customer Name
        QDialog dlg(this);
        dlg.setWindowTitle("Comprobante de Venta Exitosa");
        dlg.setFixedWidth(440);

        auto *dlgLayout = new QVBoxLayout(&dlg);
        dlgLayout->setSpacing(14);

        auto *lblHeader = new QLabel(QString(
            "<div style='text-align: center;'>"
            "<h2 style='color: %1; margin:0;'>LIBRERÍA NOVABOOK</h2>"
            "<p style='color: %2; margin:2px;'>Ticket de Venta #%3</p>"
            "<p style='color: %2; margin:2px;'>Fecha: %4</p>"
            "<p style='color: %1; font-weight: bold; margin:4px;'>Cliente: %5</p>"
            "<p style='color: %2; margin:2px;'>Pago: %6</p>"
            "</div><hr style='border: 1px solid #27272A;'>"
        ).arg(Theme::Accent).arg(Theme::TextMuted).arg(vr.id_venta).arg(vr.fecha_hora).arg(vr.cliente).arg(m_metodoPago), &dlg);
        dlgLayout->addWidget(lblHeader);

        QString detailHtml = "<table width='100%' style='color: #FAFAFA; font-size: 12px;'>";
        detailHtml += "<tr><th align='left'>Cant</th><th align='left'>Producto</th><th align='right'>Subtotal</th></tr>";
        for (const auto &it : vr.items) {
            detailHtml += QString("<tr><td>%1x</td><td>%2</td><td align='right'>$ %3</td></tr>")
                .arg(it.cantidad)
                .arg(it.nombre_producto)
                .arg(QString::number(it.subtotal, 'f', 2));
        }
        detailHtml += "</table><hr style='border: 1px solid #27272A;'>";
        detailHtml += QString("<h3 align='right' style='color: %1; margin: 4px;'>TOTAL PAGADO: $ %2</h3>")
            .arg(Theme::Success)
            .arg(QString::number(vr.total, 'f', 2));

        auto *lblDetails = new QLabel(detailHtml, &dlg);
        dlgLayout->addWidget(lblDetails);

        auto *btnCerrar = Ui::button("Aceptar y Nueva Venta", "primary", &dlg);
        connect(btnCerrar, &QPushButton::clicked, &dlg, &QDialog::accept);
        dlgLayout->addWidget(btnCerrar);

        dlg.exec();

        limpiarCarrito();
        Toast::showToast(this, "Venta completada con éxito", Toast::Success);
    });
}
