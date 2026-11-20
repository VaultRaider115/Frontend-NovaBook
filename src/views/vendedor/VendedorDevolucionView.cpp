#include "VendedorDevolucionView.h"
#include "Theme.h"
#include "Ui.h"
#include "ProductoBuscador.h"
#include "ApiClient.h"
#include "Toast.h"
#include "Documentos.h"
#include "EanClassifier.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSharedPointer>

VendedorDevolucionView::VendedorDevolucionView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(18);

    // Encabezado + proveedor destino
    auto *top = new QHBoxLayout();
    top->setSpacing(24);
    top->addWidget(Ui::pageHeader("Vendedor · Logística","Gestión de Devoluciones a Proveedor",
                                  "Registra artículos para devolución con descarga de acta PDF.", this), 1);

    auto *provCard = Ui::card(this);
    provCard->setMinimumWidth(460);
    auto *pl = new QVBoxLayout(provCard);
    pl->setContentsMargins(16, 12, 16, 12);
    pl->setSpacing(6);
    m_comboProveedor = new QComboBox(provCard);
    m_lblRfc = Ui::label(QString(), "Mono", provCard);
    pl->addWidget(Ui::field("Proveedor destino", m_comboProveedor, provCard));
    pl->addWidget(m_lblRfc);
    top->addWidget(provCard, 0, Qt::AlignTop);
    layout->addLayout(top);

    // Agregar artículo
    auto *addCard = Ui::card(this);
    auto *cl = new QVBoxLayout(addCard);
    cl->setContentsMargins(20, 20, 20, 20);
    cl->setSpacing(10);
    auto *row = new QHBoxLayout();
    row->setSpacing(16);

    m_txtEan = new QLineEdit(addCard);
    new ProductoBuscador(m_txtEan, [this](const QJsonObject &p) {
        // Solo títulos del proveedor elegido (la devolución es de una editorial).
        return m_comboProveedor->currentIndex() < 0 || m_comboProveedor->currentData().toInt() <= 0
               || p.value("id_proveedor").toInt() == m_comboProveedor->currentData().toInt();
    });
    m_txtEan->setPlaceholderText("EAN-13 o título del libro / revista");
    m_txtEan->addAction(QIcon(":/icons/scan-gold.svg"), QLineEdit::LeadingPosition);
    row->addWidget(Ui::field("EAN-13 o título", m_txtEan, addCard), 2);

    m_spnCantidad = new QSpinBox(addCard);
    m_spnCantidad->setRange(1, 999);
    row->addWidget(Ui::field("Cantidad", Ui::stepper(m_spnCantidad, addCard), addCard));

    m_comboMotivo = new QComboBox(addCard);
    m_comboMotivo->setPlaceholderText("Selecciona un motivo");
    m_comboMotivo->setCurrentIndex(-1);
    row->addWidget(Ui::field("Motivo de devolución", m_comboMotivo, addCard), 2);

    m_btnAgregar = Ui::button("Añadir a Devolución", "tint", addCard, "plus-gold");
    row->addWidget(m_btnAgregar, 0, Qt::AlignBottom);
    cl->addLayout(row);

    m_lblEanStatus = new QLabel("Ingresa el código EAN-13 para clasificar automáticamente como Libro o Revista", addCard);
    m_lblEanStatus->setObjectName("Hint");
    m_lblEanStatus->setWordWrap(true);
    // Estado de la validación de editorial de toda la lista (se valida al añadir).
    auto *estado = new QHBoxLayout();
    estado->setSpacing(10);
    estado->addWidget(m_lblEanStatus, 1);
    estado->addWidget(Ui::label("Validación de editorial:", "Muted", addCard));
    m_lblValidacion = Ui::chip("Sin artículos", "neutral", addCard);
    estado->addWidget(m_lblValidacion);
    cl->addLayout(estado);
    layout->addWidget(addCard);

    // Artículos a devolver
    m_tableItems = Ui::table({"EAN", "Tipo", "Título", "Cantidad", "Motivo", "Editorial", "Acción"}, 2, this, 56);
    layout->addWidget(m_tableItems, 1);

    // Pie: totales + acciones
    auto *foot = Ui::card(this);
    auto *fl = new QHBoxLayout(foot);
    fl->setContentsMargins(20, 16, 20, 16);
    fl->setSpacing(24);

    auto *tot = new QWidget(foot);
    auto *tl = new QVBoxLayout(tot);
    tl->setContentsMargins(0, 0, 0, 0);
    tl->setSpacing(2);
    auto *totLbl = Ui::label("TOTAL PIEZAS A DEVOLVER", "FieldLabel", tot);
    Ui::spaced(totLbl, 0.8);
    tl->addWidget(totLbl);
    m_lblTotal = Ui::label("0", "BigNumber", tot);
    tl->addWidget(m_lblTotal);
    fl->addWidget(tot);
    fl->addWidget(Ui::divider(foot, true));

    auto *lote = new QWidget(foot);
    auto *ll = new QVBoxLayout(lote);
    ll->setContentsMargins(0, 0, 0, 0);
    ll->setSpacing(2);
    auto *loteLbl = Ui::label("LOTE DE DEVOLUCIÓN", "FieldLabel", lote);
    Ui::spaced(loteLbl, 0.8);
    ll->addWidget(loteLbl);
    m_lblLote = Ui::label("—", "Gold", lote);
    m_lblLote->setStyleSheet(QString("font-family: %1; font-size: 18px;").arg(Theme::FontMono));
    ll->addWidget(m_lblLote);
    fl->addWidget(lote);
    fl->addStretch();

    m_btnPdf = Ui::button("Generar y Descargar Acta PDF", "outline", foot, "pdf");
    m_btnPdf->setEnabled(false);
    m_btnPdf->setToolTip("Disponible después de registrar la solicitud");
    fl->addWidget(m_btnPdf);
    m_btnEnviar = Ui::button("Registrar Solicitud de Devolución", "primary", foot, "check-dark");
    fl->addWidget(m_btnEnviar);
    layout->addWidget(foot);

    connect(m_txtEan, &QLineEdit::textChanged, this, &VendedorDevolucionView::onEanChanged);
    connect(m_txtEan, &QLineEdit::returnPressed, this, &VendedorDevolucionView::onAgregarArticulo);
    connect(m_btnAgregar, &QPushButton::clicked, this, &VendedorDevolucionView::onAgregarArticulo);
    connect(m_btnEnviar, &QPushButton::clicked, this, &VendedorDevolucionView::onEnviarDevolucion);
    connect(m_btnPdf, &QPushButton::clicked, this, &VendedorDevolucionView::onDescargarPdf);
    connect(m_comboProveedor, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        actualizarProveedor();
        revalidarPartidas();
    });

    cargarCatalogos();
}

void VendedorDevolucionView::cargarCatalogos() {
    ApiClient::instance()->listarProveedores([this](bool ok, const QJsonValue &data, const QString &) {
        if (!ok || !data.isArray()) return;
        m_proveedores.clear();
        m_comboProveedor->clear();
        const QJsonArray arr = data.toArray();
        for (const auto &v : arr) {
            const ProveedorDto p = ProveedorDto::fromJson(v.toObject());
            m_proveedores.append(p);
            m_comboProveedor->addItem(p.nombre_proveedor, p.id_proveedor);
        }
        actualizarProveedor();
    });
    ApiClient::instance()->listarMotivos([this](bool ok, const QJsonValue &data, const QString &) {
        if (!ok || !data.isArray()) return;
        m_comboMotivo->clear();
        const QJsonArray arr = data.toArray();
        for (const auto &v : arr) {
            const MotivoDto m = MotivoDto::fromJson(v.toObject());
            m_comboMotivo->addItem(m.nombre, m.id_motivo);
        }
        m_comboMotivo->setCurrentIndex(-1);
    });
}

void VendedorDevolucionView::actualizarProveedor() {
    const int i = m_comboProveedor->currentIndex();
    m_lblRfc->setText(i >= 0 && i < m_proveedores.size() ? "RFC: " + m_proveedores.at(i).rfc : QString());
}

void VendedorDevolucionView::onEanChanged(const QString &text) {
    EanClassifier::aplicarAControles(text, m_lblEanStatus);
}

void VendedorDevolucionView::onAgregarArticulo() {
    const QString eanStr = m_txtEan->text().trimmed();
    const auto eanRes = EanClassifier::clasificar(eanStr);
    if (!eanRes.esValido) {
        Toast::showToast(this, eanRes.mensaje, Toast::Warning);
        return;
    }
    const QString motivo = m_comboMotivo->currentText().trimmed();
    const int idMotivo = m_comboMotivo->currentIndex() >= 0 ? m_comboMotivo->currentData().toInt() : 0;
    if (motivo.isEmpty() || idMotivo <= 0) {
        Toast::showToast(this, "Selecciona el motivo de la devolución", Toast::Warning);
        return;
    }

    const qint64 ean = eanStr.toLongLong();
    const int cant = m_spnCantidad->value();
    const QString tipo = eanRes.tipo == EanClassifier::Tipo::Libro ? "libro" : "revista";

    // Límite de 60 piezas por solicitud (se valida también en la API)
    int yaAgregadas = 0;
    for (const auto &it : std::as_const(m_items)) if (it.tipo == tipo) yaAgregadas += it.cantidad;
    if (yaAgregadas + cant > 60) {
        Toast::showToast(this, QString("Una devolución no puede exceder 60 piezas (llevas %1)").arg(yaAgregadas), Toast::Warning);
        return;
    }

    if (m_comboProveedor->currentIndex() < 0) {
        Toast::showToast(this, "Selecciona el proveedor destino", Toast::Warning);
        return;
    }
    const int provId = m_comboProveedor->currentData().toInt();

    // La validación se hace al añadir (no hasta registrar): el producto existe,
    // es de la editorial del proveedor elegido y alcanza bodega + apartados.
    m_btnAgregar->setEnabled(false);
    ApiClient::instance()->validarPartidaDevolucion(provId, ean, piezasDeEan(ean) + cant,
        [this, ean, cant, tipo, motivo, idMotivo, provId](bool ok, const QJsonValue &data, const QString &msg) {
        m_btnAgregar->setEnabled(true);
        if (!ok || !data.isObject()) {
            m_lblEanStatus->setText("No se añadió: " + msg);
            m_lblEanStatus->setStyleSheet(QString("font-size: 12px; color: %1; font-weight: bold;").arg(Theme::Error));
            Toast::showToast(this, msg, Toast::Error, 6000);
            return;
        }
        if (m_comboProveedor->currentData().toInt() != provId) return;   // cambió mientras validaba
        const QJsonObject o = data.toObject();
        ReturnItemDraft it{ean, tipo, o.value("titulo").toString(QString("EAN %1").arg(ean)), cant, motivo, idMotivo};
        it.validacion = ReturnItemDraft::Validacion::Validada;
        it.nota = QString("Editorial %1 · disponibles %2").arg(o.value("proveedor").toString())
                      .arg(o.value("disponible").toVariant().toLongLong());
        m_items.append(it);
        renderItems();

        m_txtEan->clear();
        m_spnCantidad->setValue(1);
        m_txtEan->setFocus();
    });
}

int VendedorDevolucionView::piezasDeEan(qint64 ean) const {
    int n = 0;
    for (const auto &it : m_items) if (it.ean == ean) n += it.cantidad;
    return n;
}

bool VendedorDevolucionView::todasValidadas() const {
    for (const auto &it : m_items)
        if (it.validacion != ReturnItemDraft::Validacion::Validada) return false;
    return true;
}

void VendedorDevolucionView::revalidarPartidas() {
    if (m_items.isEmpty() || m_comboProveedor->currentIndex() < 0) return;
    const int provId = m_comboProveedor->currentData().toInt();
    for (auto &it : m_items) {
        it.validacion = ReturnItemDraft::Validacion::Pendiente;
        it.nota.clear();
    }
    renderItems();

    // Una consulta por EAN con el total de piezas de ese EAN en la lista.
    QList<qint64> eans;
    for (const auto &it : std::as_const(m_items)) if (!eans.contains(it.ean)) eans << it.ean;
    const int ronda = ++m_revalidando;
    for (qint64 ean : std::as_const(eans)) {
        ApiClient::instance()->validarPartidaDevolucion(provId, ean, piezasDeEan(ean),
            [this, ean, ronda](bool ok, const QJsonValue &data, const QString &msg) {
            if (ronda != m_revalidando) return;   // ya se cambió otra vez de proveedor
            for (auto &it : m_items) {
                if (it.ean != ean) continue;
                it.validacion = ok ? ReturnItemDraft::Validacion::Validada : ReturnItemDraft::Validacion::Rechazada;
                it.nota = ok ? QString("Editorial %1").arg(data.toObject().value("proveedor").toString()) : msg;
            }
            renderItems();
        });
    }
}

void VendedorDevolucionView::renderItems() {
    m_tableItems->clearContents();
    m_tableItems->setRowCount(m_items.size());
    int total = 0;
    for (int i = 0; i < m_items.size(); ++i) {
        const auto &it = m_items.at(i);
        total += it.cantidad;
        Ui::cellText(m_tableItems, i, 0, QString::number(it.ean), true);
        Ui::cellWidget(m_tableItems, i, 1, Ui::tipoChip(it.tipo));
        Ui::cellText(m_tableItems, i, 2, it.nombre, false, true);
        Ui::cellText(m_tableItems, i, 3, QString::number(it.cantidad), false, true);
        Ui::cellText(m_tableItems, i, 4, it.motivo, false, false, Theme::TextMuted);
        QLabel *estado = nullptr;
        switch (it.validacion) {
        case ReturnItemDraft::Validacion::Validada: estado = Ui::chip("Validada", "ok"); break;
        case ReturnItemDraft::Validacion::Pendiente: estado = Ui::chip("Pendiente", "pending"); break;
        case ReturnItemDraft::Validacion::Rechazada: estado = Ui::chip("No es de esta editorial", "danger"); break;
        }
        estado->setToolTip(it.nota);
        Ui::cellWidget(m_tableItems, i, 5, estado);
        auto *quitar = Ui::iconButton("trash", "Quitar artículo");
        connect(quitar, &QPushButton::clicked, this, [this, i]() { m_items.removeAt(i); renderItems(); });
        Ui::cellWidget(m_tableItems, i, 6, quitar);
    }
    m_lblTotal->setText(QString::number(total));

    // Estado de la lista: validada solo si todas las partidas pasaron.
    int pendientes = 0, rechazadas = 0;
    for (const auto &it : std::as_const(m_items)) {
        if (it.validacion == ReturnItemDraft::Validacion::Pendiente) ++pendientes;
        if (it.validacion == ReturnItemDraft::Validacion::Rechazada) ++rechazadas;
    }
    const char *kind = "neutral";
    QString texto = "Sin artículos";
    if (!m_items.isEmpty()) {
        if (rechazadas) { kind = "danger"; texto = QString("%1 sin validar").arg(rechazadas); }
        else if (pendientes) { kind = "pending"; texto = "Pendiente"; }
        else { kind = "ok"; texto = "Validada"; }
    }
    m_lblValidacion->setText(texto);
    m_lblValidacion->setProperty("chip", kind);
    Ui::repolish(m_lblValidacion);
    // Mismo ancho que calcula Ui::chip (texto + padding).
    m_lblValidacion->setFixedWidth(m_lblValidacion->fontMetrics().horizontalAdvance(texto) + 28);
}

void VendedorDevolucionView::onEnviarDevolucion() {
    if (m_items.isEmpty()) {
        Toast::showToast(this, "Agrega al menos un artículo a devolver", Toast::Warning);
        return;
    }
    if (m_comboProveedor->currentIndex() < 0) {
        Toast::showToast(this, "Selecciona un proveedor destino", Toast::Warning);
        return;
    }
    if (!todasValidadas()) {
        Toast::showToast(this, "Hay artículos sin validar o que no son de la editorial del proveedor: quítalos o cambia el proveedor",
                         Toast::Warning, 6000);
        return;
    }
    const int provId = m_comboProveedor->currentData().toInt();

    // La API registra un tipo de producto por solicitud: se separa en libros y revistas.
    QStringList tipos;
    for (const auto &it : std::as_const(m_items)) if (!tipos.contains(it.tipo)) tipos << it.tipo;

    struct Estado { int pendientes; QList<QPair<qint64, QString>> ok; QStringList folios; QString error; };
    auto st = QSharedPointer<Estado>::create();
    st->pendientes = tipos.size();
    m_btnEnviar->setEnabled(false);

    for (const QString &tipo : std::as_const(tipos)) {
        QJsonArray itemsArr;
        for (const auto &it : std::as_const(m_items)) {
            if (it.tipo != tipo) continue;
            QJsonObject o;
            o["codigo_ean"] = it.ean;
            o["cantidad"] = it.cantidad;
            o["id_motivo"] = it.id_motivo;
            itemsArr.append(o);
        }
        ApiClient::instance()->crearDevolucion(provId, tipo, itemsArr, [this, st, tipo](bool ok, const QJsonValue &data, const QString &msg) {
            if (ok && data.isObject()) {
                const Devolucion d = Devolucion::fromJson(data.toObject());
                st->ok << qMakePair(d.id_devolucion, tipo);
                st->folios << "#" + (d.folio.isEmpty() ? QString("DEV-%1").arg(d.id_devolucion) : d.folio);
            } else {
                st->error = msg;
            }
            if (--st->pendientes > 0) return;

            m_btnEnviar->setEnabled(true);
            if (st->ok.isEmpty()) {
                Toast::showToast(this, "Error al registrar devolución: " + st->error, Toast::Error);
                return;
            }
            m_ultimas = st->ok;
            m_btnPdf->setToolTip("El acta se puede descargar cuando el Jefe apruebe la solicitud");
            m_lblLote->setText(st->folios.join(" · "));
            m_btnPdf->setEnabled(true);
            m_items.clear();
            renderItems();
            Toast::showToast(this, st->error.isEmpty()
                ? "Solicitud registrada. Requiere autorización del Jefe; revisa su estado en «Mis Devoluciones»."
                : "Solo se registró una parte de la solicitud: " + st->error,
                st->error.isEmpty() ? Toast::Success : Toast::Warning);
        });
    }
}

void VendedorDevolucionView::onDescargarPdf() {
    if (m_ultimas.isEmpty()) return;

    // Una acta por solicitud (libros y revistas van separadas). La API solo la
    // entrega si el Jefe ya aprobó; si no, su mensaje lo explica.
    for (const auto &r : std::as_const(m_ultimas)) {
        const qint64 id = r.first;
        ApiClient::instance()->descargarPdfDevolucion(id, r.second, [this, id](bool ok, const QByteArray &bytes, const QString &msg) {
            if (!ok) {
                Toast::showToast(this, QString("Acta #DEV-%1: %2").arg(id).arg(msg), Toast::Warning, 6000);
                return;
            }
            Documentos::guardarYAbrir(this, bytes, QString("acta_DEV-%1.pdf").arg(id));
        });
    }
}
