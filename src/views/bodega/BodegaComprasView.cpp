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
#include <QJsonArray>
#include <QRegularExpressionValidator>

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

    // Secciones: capturar una factura nueva o consultar las ya registradas.
    auto *seg = new QHBoxLayout();
    seg->setSpacing(6);
    for (const QString &n : {QString("Registrar factura"), QString("Facturas registradas")}) {
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
    auto *registro = new QWidget(m_stack);
    auto *rl = new QVBoxLayout(registro);
    rl->setContentsMargins(0, 0, 0, 0);
    rl->setSpacing(18);
    m_stack->addWidget(registro);

    // Datos de factura: proveedor + número de factura / observaciones
    auto *headerCard = Ui::card(this);
    auto *hl = new QHBoxLayout(headerCard);
    hl->setContentsMargins(20, 20, 20, 20);
    hl->setSpacing(20);
    m_comboProveedor = new QComboBox(headerCard);
    // La factura siempre lleva el prefijo "F-": se escribe solo el número.
    auto *facturaBox = new QWidget(headerCard);
    auto *fl = new QHBoxLayout(facturaBox);
    fl->setContentsMargins(0, 0, 0, 0);
    fl->setSpacing(8);
    auto *lblF = new QLabel("F-", facturaBox);
    lblF->setStyleSheet(QString("QLabel { color: %1; font-family: 'Consolas'; font-size: 15px; font-weight: 700;"
                                " background: %2; border: 1px solid %3; border-radius: 8px; padding: 9px 12px; }")
                            .arg(Theme::Accent, Theme::BgLabel, Theme::Border));
    lblF->setToolTip("Prefijo de factura: se agrega solo");
    m_txtObs = new QLineEdit(facturaBox);
    m_txtObs->setPlaceholderText("Ej. 1024");
    m_txtObs->setMaxLength(28);   // + "F-" = 30 (tamaño de la columna)
    m_txtObs->setValidator(new QRegularExpressionValidator(QRegularExpression("[A-Za-z0-9][A-Za-z0-9\\-]{0,27}"), m_txtObs));
    m_txtObs->setAccessibleName("Número de factura (sin el prefijo F-)");
    m_txtObs->addAction(QIcon(":/icons/receipt.svg"), QLineEdit::LeadingPosition);
    fl->addWidget(lblF);
    fl->addWidget(m_txtObs, 1);
    hl->addWidget(Ui::field("Proveedor Oficial", m_comboProveedor, headerCard), 1);
    hl->addWidget(Ui::field("Número de Factura", facturaBox, headerCard), 1);
    rl->addWidget(headerCard);

    // Agregar producto
    auto *addCard = Ui::card(this);
    auto *cardLayout = new QVBoxLayout(addCard);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(12);

    auto *addLayout = new QHBoxLayout();
    addLayout->setSpacing(16);
    m_txtEan = new QLineEdit(addCard);
    Ui::eanInput(m_txtEan);
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
    rl->addWidget(addCard);

    // Partidas de la compra
    m_tableCart = Ui::table({"EAN", "Tipo", "Descripción", "Llegaron / Fact.", "Costo unit.", "Subtotal", "Acción"}, 2, this, 56);
    rl->addWidget(m_tableCart, 1);

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
    rl->addWidget(bottomCard);

    // --- Facturas registradas: qué se compró en cada una ---------------------
    auto *hist = new QWidget(m_stack);
    auto *hl2 = new QVBoxLayout(hist);
    hl2->setContentsMargins(0, 0, 0, 0);
    hl2->setSpacing(14);
    auto *histHead = new QHBoxLayout();
    m_txtBuscarFactura = Ui::searchBox("Buscar por factura, proveedor, título o EAN…", hist);
    m_txtBuscarFactura->setMinimumWidth(380);
    histHead->addWidget(m_txtBuscarFactura);
    m_lblFacturas = new QLabel(hist);
    m_lblFacturas->setObjectName("Muted");
    histHead->addWidget(m_lblFacturas, 1);
    auto *btnRecargar = Ui::button("Actualizar", "ghost", hist);
    histHead->addWidget(btnRecargar);
    hl2->addLayout(histHead);
    m_tablaFacturas = Ui::table({"Compra", "Factura", "Proveedor", "Fecha", "Registró", "Títulos", "Piezas", "Total", "Acción"}, 2, hist, 56);
    for (int c : {0, 1, 3, 5, 6, 7, 8})
        m_tablaFacturas->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    hl2->addWidget(m_tablaFacturas, 1);
    m_stack->addWidget(hist);

    connect(m_txtBuscarFactura, &QLineEdit::textChanged, this, &BodegaComprasView::pintarFacturas);
    connect(btnRecargar, &QPushButton::clicked, this, &BodegaComprasView::cargarFacturas);
    connect(m_tablaFacturas, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (row >= 0 && row < m_facturasVisibles.size()) verFactura(m_facturasVisibles.at(row));
    });

    connect(m_txtEan, &QLineEdit::textChanged, this, &BodegaComprasView::onEanChanged);
    connect(m_btnAgregar, &QPushButton::clicked, this, &BodegaComprasView::onAgregarArticulo);
    connect(m_txtEan, &QLineEdit::returnPressed, this, &BodegaComprasView::onAgregarArticulo);
    connect(m_btnVaciar, &QPushButton::clicked, this, &BodegaComprasView::limpiarCarrito);
    connect(m_btnProcesar, &QPushButton::clicked, this, &BodegaComprasView::onRegistrarCompra);

    mostrarSeccion(0);
    cargar();
}

void BodegaComprasView::cargar() {
    cargarProveedores();
    cargarFacturas();
}

void BodegaComprasView::mostrarSeccion(int i) {
    for (int k = 0; k < m_segmentos.size(); ++k) Ui::setCurrent(m_segmentos.at(k), k == i);
    m_stack->setCurrentIndex(i);
}

void BodegaComprasView::cargarFacturas() {
    ApiClient::instance()->listarCompras([this](bool ok, const QJsonValue &data, const QString &msg) {
        if (!ok || !data.isArray()) {
            Toast::showToast(this, "No se pudieron cargar las facturas: " + msg, Toast::Error);
            return;
        }
        m_facturas.clear();
        for (const auto &v : data.toArray()) m_facturas.append(CompraResponse::fromJson(v.toObject()));
        pintarFacturas();
    });
}

void BodegaComprasView::pintarFacturas() {
    const QString q = m_txtBuscarFactura->text().trimmed();
    m_facturasVisibles.clear();
    for (const CompraResponse &c : std::as_const(m_facturas)) {
        bool coincide = q.isEmpty() || c.factura.contains(q, Qt::CaseInsensitive)
            || c.nombre_proveedor.contains(q, Qt::CaseInsensitive)
            || QString::number(c.id_compra) == q;
        for (int k = 0; !coincide && k < c.items.size(); ++k)
            coincide = c.items.at(k).nombre_producto.contains(q, Qt::CaseInsensitive)
                       || QString::number(c.items.at(k).codigo_ean).contains(q);
        if (coincide) m_facturasVisibles.append(c);
    }

    m_tablaFacturas->clearContents();
    m_tablaFacturas->setRowCount(m_facturasVisibles.size());
    for (int i = 0; i < m_facturasVisibles.size(); ++i) {
        const CompraResponse &c = m_facturasVisibles.at(i);
        Ui::cellText(m_tablaFacturas, i, 0, QString("#%1").arg(c.id_compra), true);
        Ui::cellText(m_tablaFacturas, i, 1, c.factura.isEmpty() ? "Sin número" : c.factura, true, !c.factura.isEmpty(),
                     c.factura.isEmpty() ? QString(Theme::TextMuted) : QString());
        Ui::cellText(m_tablaFacturas, i, 2, c.nombre_proveedor, false, true);
        Ui::cellText(m_tablaFacturas, i, 3, c.fecha_hora.left(16), true, false, Theme::TextMuted);
        Ui::cellText(m_tablaFacturas, i, 4, c.comprador_nombre.trimmed().isEmpty() ? "—" : c.comprador_nombre.trimmed());
        Ui::cellText(m_tablaFacturas, i, 5, QString::number(c.items.size()));
        Ui::cellText(m_tablaFacturas, i, 6, QString::number(c.total_articulos));
        Ui::cellText(m_tablaFacturas, i, 7, Ui::money(c.total), false, true);
        auto *ver = Ui::iconButton("eye", "Ver qué se compró");
        connect(ver, &QPushButton::clicked, this, [this, c]() { verFactura(c); });
        Ui::cellWidget(m_tablaFacturas, i, 8, ver);
    }
    m_lblFacturas->setText(m_facturas.isEmpty() ? "Todavía no hay facturas registradas."
        : q.isEmpty() ? QString("%1 factura(s) · doble clic o el ojo para ver qué se compró").arg(m_facturas.size())
                      : QString("%1 de %2 factura(s)").arg(m_facturasVisibles.size()).arg(m_facturas.size()));
}

void BodegaComprasView::verFactura(const CompraResponse &c) {
    Ui::FormDialog dlg(c.factura.isEmpty() ? QString("Compra #%1").arg(c.id_compra)
                                           : QString("Factura %1 · Compra #%2").arg(c.factura).arg(c.id_compra),
                       "Cerrar", this, 820);
    for (QPushButton *b : dlg.findChildren<QPushButton *>())
        if (b != dlg.okButton() && b->text() == "Cancelar") b->hide();

    auto *resumen = new QLabel(QString("<b>%1</b><br><span style=\"color:%2;\">Registró %3 el %4 · %5 título(s), %6 pieza(s) · Total %7</span>")
        .arg(c.nombre_proveedor.toHtmlEscaped(), Theme::TextMuted,
             c.comprador_nombre.trimmed().isEmpty() ? "—" : c.comprador_nombre.trimmed().toHtmlEscaped(), c.fecha_hora.left(16))
        .arg(c.items.size()).arg(c.total_articulos).arg(Ui::money(c.total)), &dlg);
    resumen->setTextFormat(Qt::RichText);
    resumen->setWordWrap(true);

    auto *tabla = Ui::table({"EAN", "Tipo", "Título", "Piezas", "Costo unit.", "Subtotal"}, 2, &dlg, 44);
    tabla->setRowCount(c.items.size());
    for (int i = 0; i < c.items.size(); ++i) {
        const ItemCompraDto &it = c.items.at(i);
        Ui::cellText(tabla, i, 0, QString::number(it.codigo_ean), true);
        auto *chip = Ui::tipoChip(it.tipo_producto);
        Ui::cellWidget(tabla, i, 1, chip);
        Ui::fitColumn(tabla, 1, chip);
        Ui::cellText(tabla, i, 2, it.nombre_producto, false, true);
        Ui::cellText(tabla, i, 3, QString::number(it.cantidad));
        Ui::cellText(tabla, i, 4, Ui::money(it.costo_unitario), false, false, Theme::TextMuted);
        Ui::cellText(tabla, i, 5, Ui::money(it.subtotal), false, true);
    }
    for (int col : {0, 3, 4, 5})
        tabla->horizontalHeader()->setSectionResizeMode(col, QHeaderView::ResizeToContents);
    tabla->setMinimumHeight(qMin(380, 56 + 44 * qMax(1, static_cast<int>(c.items.size()))));

    auto *nota = new QLabel("Piezas que entraron a bodega. Si el paquete llegó incompleto, lo que faltaba "
                            "se suma aquí cuando llega (ver Incidencias de Compra).", &dlg);
    nota->setObjectName("Muted");
    nota->setWordWrap(true);

    dlg.addField("Proveedor", resumen, 0, 0, 2);
    dlg.addField("Libros / revistas comprados", tabla, 1, 0, 2);
    dlg.addField("", nota, 2, 0, 2);
    connect(dlg.okButton(), &QPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
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
    // Si escribieron el prefijo a mano ("F-12"), no se repite.
    QString numFactura = m_txtObs->text().trimmed().toUpper();
    if (numFactura.startsWith("F-")) numFactura = numFactura.mid(2);
    if (!numFactura.isEmpty()) {
        req["factura"] = "F-" + numFactura;
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
        cargarFacturas();
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
