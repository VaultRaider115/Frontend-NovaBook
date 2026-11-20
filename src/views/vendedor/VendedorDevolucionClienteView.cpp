#include "VendedorDevolucionClienteView.h"
#include "ApiClient.h"
#include "EanClassifier.h"
#include "ProductoBuscador.h"
#include "Theme.h"
#include "Toast.h"
#include "SeccionesScroll.h"
#include "Ui.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

VendedorDevolucionClienteView::VendedorDevolucionClienteView(QWidget *parent)
    : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(40, 32, 40, 32);
  layout->setSpacing(18);
  layout->addWidget(
      Ui::pageHeader("Vendedor · Atención a clientes", "Devolución de Cliente",
                     "Hasta 15 días con ticket. Sin defecto regresa a piso; "
                     "con defecto queda apartado y no se vende.",
                     this));

  // Una sola vista con scroll: el ticket y, debajo, lo que elige el cliente
  // (reembolso o cambio). El resumen con "Registrar" queda fijo abajo.
  auto *secciones = new SeccionesScroll(this);
  layout->addWidget(secciones, 1);
  auto *secTicket = secciones->agregarSeccion("Ticket del cliente");
  auto *ticketLay = new QVBoxLayout(secTicket);
  ticketLay->setContentsMargins(0, 0, 0, 0);
  ticketLay->setSpacing(18);
  auto *secResolucion = secciones->agregarSeccion("Reembolso o cambio");
  auto *resLay = new QVBoxLayout(secResolucion);
  resLay->setContentsMargins(0, 0, 0, 0);

  // --- Ticket -------------------------------------------------------------
  auto *ticketCard = Ui::card(this);
  auto *tl = new QVBoxLayout(ticketCard);
  tl->setContentsMargins(20, 20, 20, 20);
  tl->setSpacing(10);
  auto *fila = new QHBoxLayout();
  fila->setSpacing(16);
  m_txtTicket = new QLineEdit(ticketCard);
  m_txtTicket->setPlaceholderText("Número de ticket, ej. 1084");
  m_txtTicket->setValidator(new QRegularExpressionValidator(
      QRegularExpression("[1-9][0-9]{0,17}"), m_txtTicket));
  m_txtTicket->addAction(QIcon(":/icons/receipt.svg"),
                         QLineEdit::LeadingPosition);
  fila->addWidget(Ui::field("Número de ticket", m_txtTicket, ticketCard), 1);
  m_btnBuscar =
      Ui::button("Buscar ticket", "primary", ticketCard, "search-dark");
  fila->addWidget(m_btnBuscar, 0, Qt::AlignBottom);
  tl->addLayout(fila);
  m_lblTicket =
      new QLabel("Escribe el número de ticket del cliente.", ticketCard);
  m_lblTicket->setObjectName("Muted");
  m_lblTicket->setWordWrap(true);
  tl->addWidget(m_lblTicket);
  ticketLay->addWidget(ticketCard);

  m_tablaTicket = Ui::table({"EAN", "Título", "Vendidos", "Ya devueltos",
                             "Precio pagado", "A devolver", "Con defecto"},
                            1, this, 56);
  m_tablaTicket->setMinimumHeight(260);
  ticketLay->addWidget(m_tablaTicket, 1);

  // --- Resolución y cambio ---------------------------------------------------
  auto *resCard = Ui::card(this);
  auto *rl = new QVBoxLayout(resCard);
  rl->setContentsMargins(20, 20, 20, 20);
  rl->setSpacing(12);
  auto *resLbl = Ui::label("EL CLIENTE ELIGE", "FieldLabel", resCard);
  Ui::spaced(resLbl, 0.8);
  rl->addWidget(resLbl);
  auto *resRow = new QHBoxLayout();
  resRow->setSpacing(8);
  m_btnReembolso = Ui::button("Reembolso", "pay", resCard, "cash");
  m_btnCambio =
      Ui::button("Cambio por otro producto", "pay", resCard, "refresh");
  resRow->addWidget(m_btnReembolso);
  resRow->addWidget(m_btnCambio);
  resRow->addStretch();
  rl->addLayout(resRow);

  m_panelCambio = new QWidget(resCard);
  auto *cl = new QVBoxLayout(m_panelCambio);
  cl->setContentsMargins(0, 8, 0, 0);
  cl->setSpacing(10);
  auto *addRow = new QHBoxLayout();
  addRow->setSpacing(16);
  m_txtEanCambio = new QLineEdit(m_panelCambio);
  new ProductoBuscador(m_txtEanCambio);
  m_txtEanCambio->setPlaceholderText("EAN o título del producto que se lleva");
  m_txtEanCambio->addAction(QIcon(":/icons/scan-gold.svg"),
                            QLineEdit::LeadingPosition);
  addRow->addWidget(Ui::field("Producto nuevo (EAN-13 o título)",
                              m_txtEanCambio, m_panelCambio),
                    1);
  m_spnCantCambio = new QSpinBox(m_panelCambio);
  m_spnCantCambio->setRange(1, 99);
  addRow->addWidget(Ui::field(
      "Cantidad", Ui::stepper(m_spnCantCambio, m_panelCambio), m_panelCambio));
  auto *btnAdd = Ui::button("Añadir", "tint", m_panelCambio, "plus-gold");
  addRow->addWidget(btnAdd, 0, Qt::AlignBottom);
  cl->addLayout(addRow);
  m_tablaCambio =
      Ui::table({"EAN", "Producto nuevo", "Cantidad", "Precio", "Subtotal", ""},
                1, m_panelCambio, 48);
  m_tablaCambio->setMinimumHeight(200);
  cl->addWidget(m_tablaCambio);
  auto *payRow = new QHBoxLayout();
  payRow->setSpacing(8);
  auto *payLbl =
      Ui::label("PAGA LA DIFERENCIA CON", "FieldLabel", m_panelCambio);
  Ui::spaced(payLbl, 0.8);
  payRow->addWidget(payLbl);
  const QList<QPair<QString, QString>> metodos = {{"Efectivo", "cash"},
                                                  {"Tarjeta", "card"}};
  for (const auto &[metodo, icono] : metodos) {
    auto *b = Ui::button(metodo, "pay", m_panelCambio, icono);
    Ui::setCurrent(b, metodo.toUpper() == m_metodoPago);
    connect(b, &QPushButton::clicked, this, [this, metodo]() {
      m_metodoPago = metodo.toUpper();
      for (auto *x : std::as_const(m_btnsPago))
        Ui::setCurrent(x, x->text().toUpper() == m_metodoPago);
    });
    m_btnsPago << b;
    payRow->addWidget(b);
  }
  payRow->addStretch();
  cl->addLayout(payRow);
  rl->addWidget(m_panelCambio);
  resLay->addWidget(resCard);

  // --- Resumen ------------------------------------------------------------
  auto *foot = Ui::card(this);
  auto *fl = new QHBoxLayout(foot);
  fl->setContentsMargins(20, 16, 20, 16);
  fl->setSpacing(24);
  auto caja = [&](const QString &titulo, QLabel *&valor) {
    auto *w = new QWidget(foot);
    auto *l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(2);
    auto *t = Ui::label(titulo, "FieldLabel", w);
    Ui::spaced(t, 0.8);
    l->addWidget(t);
    valor = new QLabel("$0.00", w);
    valor->setStyleSheet("font-size: 20px; font-weight: 700;");
    l->addWidget(valor);
    fl->addWidget(w);
  };
  caja("DEVUELVE", m_lblDevuelto);
  caja("SE LLEVA", m_lblNuevo);
  caja("RESULTADO", m_lblResultado);
  m_lblResultado->setStyleSheet(
      QString("font-size: 20px; font-weight: 700; color: %1;")
          .arg(Theme::Accent));
  fl->addStretch();
  m_btnRegistrar =
      Ui::button("Registrar devolución", "primary", foot, "check-dark");
  m_btnRegistrar->setStyleSheet("QPushButton { padding: 16px 28px; }");
  fl->addWidget(m_btnRegistrar);
  layout->addWidget(foot);

  connect(m_btnBuscar, &QPushButton::clicked, this,
          &VendedorDevolucionClienteView::buscarTicket);
  connect(m_txtTicket, &QLineEdit::returnPressed, this,
          &VendedorDevolucionClienteView::buscarTicket);
  connect(m_btnReembolso, &QPushButton::clicked, this,
          [this]() { setResolucion(false); });
  connect(m_btnCambio, &QPushButton::clicked, this,
          [this]() { setResolucion(true); });
  connect(btnAdd, &QPushButton::clicked, this,
          &VendedorDevolucionClienteView::agregarCambio);
  connect(m_txtEanCambio, &QLineEdit::returnPressed, this,
          &VendedorDevolucionClienteView::agregarCambio);
  connect(m_btnRegistrar, &QPushButton::clicked, this,
          &VendedorDevolucionClienteView::registrar);

  setResolucion(false);
  limpiar();
}

void VendedorDevolucionClienteView::limpiar() {
  m_idVenta = 0;
  m_vigente = false;
  m_lineas.clear();
  m_cambios.clear();
  m_tablaTicket->clearContents();
  m_tablaTicket->setRowCount(0);
  pintarCambios();
  recalcular();
}

void VendedorDevolucionClienteView::buscarTicket() {
  const qint64 id = m_txtTicket->text().trimmed().toLongLong();
  if (id <= 0) {
    Toast::showToast(this, "Escribe un número de ticket", Toast::Warning);
    return;
  }
  m_btnBuscar->setEnabled(false);
  ApiClient::instance()->consultarTicket(
      id, [this](bool ok, const QJsonValue &data, const QString &msg) {
        m_btnBuscar->setEnabled(true);
        limpiar();
        if (!ok || !data.isObject()) {
          m_lblTicket->setText(msg);
          m_lblTicket->setStyleSheet(QString("color: %1;").arg(Theme::Error));
          return;
        }
        const QJsonObject t = data.toObject();
        m_idVenta = t.value("id_venta").toVariant().toLongLong();
        m_vigente = t.value("vigente").toBool();
        const int dias = t.value("dias_transcurridos").toInt();
        m_lblTicket->setText(
            QString("Ticket #%1 · %2 · %3 · Total %4 · %5")
                .arg(m_idVenta)
                .arg(t.value("nombre_cliente").toString(),
                     t.value("fecha_hora").toString())
                .arg(Ui::money(t.value("total").toDouble()))
                .arg(m_vigente ? QString("hace %1 día(s)").arg(dias)
                               : QString("hace %1 días: fuera del plazo de 15 "
                                         "días, no se acepta")
                                     .arg(dias)));
        m_lblTicket->setStyleSheet(
            m_vigente
                ? QString()
                : QString("color: %1; font-weight: 600;").arg(Theme::Error));
        for (const auto &v : t.value("items").toArray()) {
          const QJsonObject o = v.toObject();
          Linea l;
          l.ean = o.value("codigo_ean").toVariant().toLongLong();
          l.titulo = o.value("titulo").toString();
          l.tipo = o.value("tipo_producto").toString();
          l.vendida = o.value("cantidad_vendida").toInt();
          l.devuelta = o.value("cantidad_devuelta").toInt();
          l.precio = o.value("precio_unitario").toDouble();
          m_lineas.append(l);
        }
        pintarTicket();
      });
}

void VendedorDevolucionClienteView::pintarTicket() {
  m_tablaTicket->clearContents();
  m_tablaTicket->setRowCount(m_lineas.size());
  for (int i = 0; i < m_lineas.size(); ++i) {
    Linea &l = m_lineas[i];
    const int disponible = qMax(0, l.vendida - l.devuelta);
    Ui::cellText(m_tablaTicket, i, 0, QString::number(l.ean), true);
    Ui::cellText(m_tablaTicket, i, 1, l.titulo, false, true);
    Ui::cellText(m_tablaTicket, i, 2, QString::number(l.vendida));
    Ui::cellText(m_tablaTicket, i, 3, QString::number(l.devuelta), false, false,
                 Theme::TextMuted);
    Ui::cellText(m_tablaTicket, i, 4, Ui::money(l.precio));

    l.spnDevolver = new QSpinBox();
    l.spnDevolver->setRange(0, disponible);
    l.spnDevolver->setFixedSize(84,
                                34); // cabe en la fila (56 px menos el padding)
    l.spnDevolver->setStyleSheet(
        "QSpinBox { padding: 0px 10px; min-height: 0px; }");
    l.spnDevolver->setEnabled(m_vigente && disponible > 0);
    l.spnDevolver->setAccessibleName(
        QString("Piezas a devolver de %1").arg(l.titulo));
    connect(l.spnDevolver, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &VendedorDevolucionClienteView::recalcular);
    Ui::cellWidget(m_tablaTicket, i, 5, l.spnDevolver);

    l.chkDefecto = new QCheckBox("Defecto");
    l.chkDefecto->setEnabled(m_vigente && disponible > 0);
    l.chkDefecto->setToolTip("Con defecto queda apartado (no se vende) hasta "
                             "devolverlo a la editorial");
    Ui::cellWidget(m_tablaTicket, i, 6, l.chkDefecto);
  }
  recalcular();
}

void VendedorDevolucionClienteView::setResolucion(bool cambio) {
  m_esCambio = cambio;
  Ui::setCurrent(m_btnReembolso, !cambio);
  Ui::setCurrent(m_btnCambio, cambio);
  m_panelCambio->setVisible(cambio);
  recalcular();
}

void VendedorDevolucionClienteView::agregarCambio() {
  const QString eanStr = m_txtEanCambio->text().trimmed();
  const auto res = EanClassifier::clasificar(eanStr);
  if (!res.esValido) {
    Toast::showToast(this, res.mensaje, Toast::Warning);
    return;
  }
  const QString tipo =
      res.tipo == EanClassifier::Tipo::Libro ? "libro" : "revista";
  const int cant = m_spnCantCambio->value();
  // Precio y título vigentes (la API vuelve a calcularlos al registrar).
  ApiClient::instance()->consultarExistencias(
      eanStr, tipo, 1,
      [this, eanStr, cant](bool ok, const QJsonValue &data,
                           const QString &msg) {
        if (!ok || !data.isArray() || data.toArray().isEmpty()) {
          Toast::showToast(this,
                           ok ? "No hay ese producto en piso de venta"
                              : "Error al buscar el producto: " + msg,
                           Toast::Warning);
          return;
        }
        const QJsonObject o = data.toArray().first().toObject();
        const qint64 ean = eanStr.toLongLong();
        bool existe = false;
        for (auto &c : m_cambios)
          if (c.ean == ean) {
            c.cantidad += cant;
            existe = true;
          }
        if (!existe)
          m_cambios.append({ean, o.value("titulo").toString(),
                            o.value("precio").toDouble(), cant});
        m_txtEanCambio->clear();
        m_spnCantCambio->setValue(1);
        pintarCambios();
      });
}

void VendedorDevolucionClienteView::pintarCambios() {
  m_tablaCambio->clearContents();
  m_tablaCambio->setRowCount(m_cambios.size());
  for (int i = 0; i < m_cambios.size(); ++i) {
    const Cambio &c = m_cambios.at(i);
    Ui::cellText(m_tablaCambio, i, 0, QString::number(c.ean), true);
    Ui::cellText(m_tablaCambio, i, 1, c.titulo, false, true);
    Ui::cellText(m_tablaCambio, i, 2, QString::number(c.cantidad));
    Ui::cellText(m_tablaCambio, i, 3, Ui::money(c.precio));
    Ui::cellText(m_tablaCambio, i, 4, Ui::money(c.precio * c.cantidad), false,
                 true);
    auto *quitar = Ui::iconButton("trash", "Quitar producto");
    connect(quitar, &QPushButton::clicked, this, [this, i]() {
      m_cambios.removeAt(i);
      pintarCambios();
    });
    Ui::cellWidget(m_tablaCambio, i, 5, quitar);
  }
  recalcular();
}

void VendedorDevolucionClienteView::recalcular() {
  double devuelto = 0;
  int piezas = 0;
  for (const auto &l : std::as_const(m_lineas)) {
    if (!l.spnDevolver)
      continue;
    devuelto += l.precio * l.spnDevolver->value();
    piezas += l.spnDevolver->value();
  }
  double nuevo = 0;
  for (const auto &c : std::as_const(m_cambios))
    nuevo += c.precio * c.cantidad;
  if (!m_esCambio)
    nuevo = 0;

  m_lblDevuelto->setText(Ui::money(devuelto));
  m_lblNuevo->setText(m_esCambio ? Ui::money(nuevo) : QString("—"));
  QString resultado;
  bool valido = m_idVenta > 0 && m_vigente && piezas > 0;
  if (!m_esCambio) {
    resultado = QString("Reembolsar %1").arg(Ui::money(devuelto));
  } else if (nuevo + 0.005 < devuelto) {
    resultado = "Falta: el cambio debe ser de igual o mayor precio";
    valido = false;
  } else {
    resultado = QString("Cobrar %1").arg(Ui::money(nuevo - devuelto));
    valido = valido && !m_cambios.isEmpty();
  }
  m_lblResultado->setText(resultado);
  m_btnRegistrar->setEnabled(valido);
}

void VendedorDevolucionClienteView::registrar() {
  QJsonArray items;
  for (const auto &l : std::as_const(m_lineas)) {
    if (!l.spnDevolver || l.spnDevolver->value() <= 0)
      continue;
    items.append(QJsonObject{{"codigo_ean", l.ean},
                             {"cantidad", l.spnDevolver->value()},
                             {"con_defecto", l.chkDefecto->isChecked()}});
  }
  QJsonArray cambio;
  if (m_esCambio)
    for (const auto &c : std::as_const(m_cambios))
      cambio.append(
          QJsonObject{{"codigo_ean", c.ean}, {"cantidad", c.cantidad}});

  QJsonObject req{{"id_venta", m_idVenta},
                  {"resolucion", m_esCambio ? "CAMBIO" : "REEMBOLSO"},
                  {"items", items},
                  {"items_cambio", cambio},
                  {"metodo_pago", m_metodoPago}};

  m_btnRegistrar->setEnabled(false);
  ApiClient::instance()->registrarDevolucionCliente(
      req, [this](bool ok, const QJsonValue &data, const QString &msg) {
        if (!ok || !data.isObject()) {
          Toast::showToast(this, "No se registró la devolución: " + msg,
                           Toast::Error, 6000);
          recalcular(); // vuelve a habilitar si procede
          return;
        }
        const QJsonObject r = data.toObject();
        const QString texto =
            r.value("id_venta_cambio").isNull()
                ? QString("Devolución registrada. Reembolsar %1 al cliente")
                      .arg(Ui::money(r.value("reembolso").toDouble()))
                : QString("Cambio registrado (ticket nuevo #%1). Cobrar %2")
                      .arg(r.value("id_venta_cambio").toVariant().toLongLong())
                      .arg(Ui::money(r.value("diferencia_cobrada").toDouble()));
        Toast::showToast(this, texto, Toast::Success, 6000);
        m_cambios.clear();
        buscarTicket(); // refresca lo que queda por devolver
      });
}
