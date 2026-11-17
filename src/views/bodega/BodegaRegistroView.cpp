#include "BodegaRegistroView.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Toast.h"
#include "EanClassifier.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include "Ui.h"
#include <QLabel>
#include <QFrame>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

BodegaRegistroView::BodegaRegistroView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(20);
    layout->addWidget(Ui::pageHeader("Bodega · Alta de mercancía", "Alta de Mercancía en Almacén (Ingreso por EAN)", "", this));

    auto *card = Ui::card(this);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(28, 28, 28, 28);
    cardLayout->setSpacing(14);

    // 1. EAN con detección automática de tipo
    auto *eanBox = new QWidget(card);
    auto *eanLayout = new QVBoxLayout(eanBox);
    eanLayout->setContentsMargins(0, 0, 0, 0);
    eanLayout->setSpacing(6);
    m_txtEan = new QLineEdit(eanBox);
    Ui::eanInput(m_txtEan);
    m_txtEan->setProperty("tone", "gold");
    m_txtEan->setPlaceholderText("Escanear o ingresar EAN-13 (13 dígitos)");
    eanLayout->addWidget(m_txtEan);
    m_lblEanStatus = new QLabel("Ingresa el código EAN-13 para clasificar automáticamente como Libro o Revista", eanBox);
    m_lblEanStatus->setObjectName("Hint");
    eanLayout->addWidget(m_lblEanStatus);
    cardLayout->addWidget(Ui::rowField("Código EAN-13:", eanBox, card));

    m_txtSku = new QLineEdit(card);
    m_txtSku->setPlaceholderText("SKU interno de 6 a 7 dígitos");
    m_txtSku->setMaxLength(7);
    // Solo dígitos y sin cero inicial (el SKU se guarda como número).
    m_txtSku->setValidator(new QRegularExpressionValidator(QRegularExpression("[1-9][0-9]{0,6}"), m_txtSku));
    cardLayout->addWidget(Ui::rowField("SKU Interno:", m_txtSku, card));

    m_txtNombre = new QLineEdit(card);
    m_txtNombre->setPlaceholderText("Título del libro o nombre de la revista");
    cardLayout->addWidget(Ui::rowField("Título / Nombre:", m_txtNombre, card));

    // 2. Campos de Libro (ISBN 978 / 979)
    m_panelLibro = new QWidget(card);
    auto *libroLayout = new QVBoxLayout(m_panelLibro);
    libroLayout->setContentsMargins(0, 0, 0, 0);
    libroLayout->setSpacing(14);
    m_comboAutor = new QComboBox(m_panelLibro);
    libroLayout->addWidget(Ui::rowField("Autor(es):", m_comboAutor, m_panelLibro));
    cardLayout->addWidget(m_panelLibro);
    m_panelLibro->hide();

    // 3. Campos de Revista (ISSN 977)
    m_panelRevista = new QWidget(card);
    auto *revistaLayout = new QVBoxLayout(m_panelRevista);
    revistaLayout->setContentsMargins(0, 0, 0, 0);
    revistaLayout->setSpacing(14);
    m_comboAutorEditorial = new QComboBox(m_panelRevista);
    m_comboAutorEditorial->setPlaceholderText("Selecciona una editorial");
    revistaLayout->addWidget(Ui::rowField("Editorial:", m_comboAutorEditorial, m_panelRevista));
    m_spnEdicion = new QSpinBox(m_panelRevista);
    m_spnEdicion->setRange(1, 9999);
    revistaLayout->addWidget(Ui::rowField("Número de Edición:", m_spnEdicion, m_panelRevista));
    m_comboPeriodicidad = new QComboBox(m_panelRevista);
    m_comboPeriodicidad->addItem("Selecciona una periodicidad", "");
    m_comboPeriodicidad->addItem("Mensual", "Mensual");
    m_comboPeriodicidad->addItem("Semanal", "Semanal");
    m_comboPeriodicidad->addItem("Quincenal", "Quincenal");
    revistaLayout->addWidget(Ui::rowField("Periodicidad:", m_comboPeriodicidad, m_panelRevista));
    cardLayout->addWidget(m_panelRevista);
    m_panelRevista->hide();

    // 4. Géneros / categorías (relación N:M)
    m_listGeneros = new QListWidget(card);
    m_listGeneros->setFixedHeight(120);
    cardLayout->addWidget(Ui::rowField("Géneros / Categoría:", m_listGeneros, card));

    m_spnPrecio = new QDoubleSpinBox(card);
    m_spnPrecio->setRange(0.0, 99999.0);
    m_spnPrecio->setValue(0.0);
    m_spnPrecio->setPrefix("$ ");
    cardLayout->addWidget(Ui::rowField("Precio de Venta:", m_spnPrecio, card));

    // La cantidad ya no se captura aquí: el stock entra por Compras a Proveedor.

    m_comboMueble = new QComboBox(card);
    cardLayout->addWidget(Ui::rowField("Mueble de Asignación:", m_comboMueble, card));

    m_comboProveedor = new QComboBox(card);
    cardLayout->addWidget(Ui::rowField("Proveedor Oficial:", m_comboProveedor, card));

    auto *btnLayout = new QHBoxLayout();
    m_btnGuardar = Ui::button("Registrar en Bodega", "primary", card, "check-dark");
    m_btnLimpiar = Ui::button("Limpiar", "", card);
    btnLayout->addSpacing(204);
    btnLayout->addWidget(m_btnGuardar);
    btnLayout->addWidget(m_btnLimpiar);
    btnLayout->addStretch();
    cardLayout->addLayout(btnLayout);

    // El formulario crece con el contenido: se desplaza dentro de un área con scroll.
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *inner = new QWidget();
    auto *innerLayout = new QVBoxLayout(inner);
    innerLayout->setContentsMargins(0, 0, 8, 0);
    innerLayout->addWidget(card);
    innerLayout->addStretch();
    scroll->setWidget(inner);
    layout->addWidget(scroll, 1);

    connect(m_txtEan, &QLineEdit::textChanged, this, &BodegaRegistroView::onEanChanged);
    m_timerVerificarEan = new QTimer(this);
    m_timerVerificarEan->setSingleShot(true);
    m_timerVerificarEan->setInterval(350);
    connect(m_timerVerificarEan, &QTimer::timeout, this, &BodegaRegistroView::verificarLibroExistente);
    connect(m_comboProveedor, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        if (m_tipoDetectado == DetectedType::Libro) m_timerVerificarEan->start();
    });
    connect(m_btnGuardar, &QPushButton::clicked, this, &BodegaRegistroView::onRegistrar);
    connect(m_btnLimpiar, &QPushButton::clicked, this, &BodegaRegistroView::limpiarCampos);

    cargarCatalogos();
}

void BodegaRegistroView::cargarCatalogos() {
    // 1. Géneros
    ApiClient::instance()->listarGeneros([this](bool ok, const QJsonValue &data, const QString &) {
        if (!ok || !data.isArray()) return;
        m_listGeneros->clear();
        m_generos.clear();
        QJsonArray arr = data.toArray();
        for (int i = 0; i < arr.size(); ++i) {
            GeneroDto g = GeneroDto::fromJson(arr.at(i).toObject());
            m_generos.append(g);

            auto *item = new QListWidgetItem(g.genero_literario, m_listGeneros);
            item->setData(Qt::UserRole, g.id_genero);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Unchecked);
        }
    });

    // 2. Autores
    ApiClient::instance()->listarAutores([this](bool ok, const QJsonValue &data, const QString &) {
        if (!ok || !data.isArray()) return;
        m_comboAutor->clear();
        m_autores.clear();
        QJsonArray arr = data.toArray();
        for (int i = 0; i < arr.size(); ++i) {
            AutorDto a = AutorDto::fromJson(arr.at(i).toObject());
            m_autores.append(a);
            QString label = QString("%1 %2").arg(a.nombre, a.apellidos);
            if (!a.nacionalidad.isEmpty()) label += QString(" (%1)").arg(a.nacionalidad);
            m_comboAutor->addItem(label, a.id_autor);
        }
    });

    // 2b. Editoriales (para revistas)
    ApiClient::instance()->listarEditoriales([this](bool ok, const QJsonValue &data, const QString &) {
        if (!ok || !data.isArray()) return;
        m_comboAutorEditorial->clear();
        const QJsonArray arr = data.toArray();
        for (const auto &v : arr) {
            const QJsonObject o = v.toObject();
            m_comboAutorEditorial->addItem(o.value("nombre_editorial").toString(), o.value("id_editorial").toInt());
        }
        m_comboAutorEditorial->setCurrentIndex(-1);
    });

    // 2c. Muebles de Bodega (ubicación 2)
    ApiClient::instance()->listarMuebles([this](bool ok, const QJsonValue &data, const QString &) {
        if (!ok || !data.isArray()) return;
        m_comboMueble->clear();
        const QJsonArray arr = data.toArray();
        for (const auto &v : arr) {
            const MuebleDto m = MuebleDto::fromJson(v.toObject());
            if (m.id_ubicacion != 2) continue;
            QString label = m.codigo.isEmpty() ? m.nombre : QString("%1 · %2").arg(m.codigo, m.nombre);
            if (!m.genero_asignado.isEmpty()) label += QString(" (%1)").arg(m.genero_asignado);
            m_comboMueble->addItem(label, m.id_mueble);
        }
    });

    // 3. Proveedores
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

void BodegaRegistroView::onEanChanged(const QString &text) {
    auto res = EanClassifier::clasificar(text);

    m_lblEanStatus->setText(res.mensaje);
    m_lblEanStatus->setStyleSheet(QString("color: %1; font-weight: %2;")
                                     .arg(res.colorEstilo, res.esValido || res.colorEstilo == Theme::Error ? "bold" : "500"));

    if (res.tipo == EanClassifier::Tipo::Libro) {
        m_tipoDetectado = DetectedType::Libro;
        m_panelLibro->show();
        m_panelRevista->hide();
    } else if (res.tipo == EanClassifier::Tipo::Revista) {
        m_tipoDetectado = DetectedType::Revista;
        m_panelLibro->hide();
        m_panelRevista->show();
    } else {
        m_tipoDetectado = DetectedType::Desconocido;
        m_panelLibro->hide();
        m_panelRevista->hide();
    }

    if (m_tipoDetectado == DetectedType::Libro && res.esValido)
        m_timerVerificarEan->start();
    else
        m_timerVerificarEan->stop();
}

void BodegaRegistroView::verificarLibroExistente() {
    const QString ean = m_txtEan->text().trimmed();
    if (m_tipoDetectado != DetectedType::Libro || !EanClassifier::clasificar(ean).esValido
        || m_comboProveedor->currentIndex() < 0) {
        return;
    }

    const int idProveedor = m_comboProveedor->currentData().toInt();
    ApiClient::instance()->consultarExistencias(ean, "libro", 0,
        [this, ean, idProveedor](bool ok, const QJsonValue &data, const QString &msg) {
            if (m_txtEan->text().trimmed() != ean
                || m_comboProveedor->currentData().toInt() != idProveedor) {
                return;
            }
            if (!ok || !data.isArray()) {
                Toast::showToast(this, "No se pudo verificar el libro existente: " + msg, Toast::Error);
                return;
            }

            for (const QJsonValue &value : data.toArray()) {
                const QJsonObject libro = value.toObject();
                if (libro.value("codigo_ean").toVariant().toLongLong() == ean.toLongLong()
                    && libro.value("id_proveedor").toInt() == idProveedor) {
                    Toast::showToast(this,
                        QString("El libro \"%1\" ya está registrado con este proveedor/editorial.")
                            .arg(libro.value("titulo").toString()),
                        Toast::Warning, 6000);
                    return;
                }
            }
        });
}

void BodegaRegistroView::limpiarCampos() {
    m_txtEan->clear();
    m_txtSku->clear();
    m_txtNombre->clear();
    m_comboAutorEditorial->setCurrentIndex(-1);
    m_comboPeriodicidad->setCurrentIndex(0);
    m_spnPrecio->setValue(0.0);
    m_spnEdicion->setValue(1);
    m_lblEanStatus->setText("Ingresa el código EAN-13 para clasificar automáticamente como Libro o Revista");
    m_lblEanStatus->setStyleSheet(QString());
    m_panelLibro->hide();
    m_panelRevista->hide();
    m_tipoDetectado = DetectedType::Desconocido;

    for (int i = 0; i < m_listGeneros->count(); ++i) {
        m_listGeneros->item(i)->setCheckState(Qt::Unchecked);
    }
}

void BodegaRegistroView::onRegistrar() {
    if (m_tipoDetectado == DetectedType::Desconocido) {
        Toast::showToast(this, "Ingresa un código EAN-13 válido (978/979 para libros o 977 para revistas)", Toast::Warning);
        return;
    }

    qint64 ean = m_txtEan->text().trimmed().toLongLong();
    QString nombre = m_txtNombre->text().trimmed();
    if (nombre.isEmpty()) {
        Toast::showToast(this, "El título o nombre del producto es obligatorio", Toast::Warning);
        return;
    }

    // El SKU es obligatorio para un producto nuevo; la API lo exige y avisa si falta.
    const QString sku = m_txtSku->text().trimmed();
    if (!sku.isEmpty() && (sku.size() < 6 || sku.size() > 7)) {
        Toast::showToast(this, "El SKU debe tener de 6 a 7 dígitos", Toast::Warning);
        m_txtSku->setFocus();
        return;
    }

    // El precio empieza en 0: hay que capturarlo (la API tampoco acepta $0).
    if (m_spnPrecio->value() <= 0.0) {
        Toast::showToast(this, "El precio de venta debe ser mayor a $0", Toast::Warning);
        m_spnPrecio->setFocus();
        m_spnPrecio->selectAll();
        return;
    }

    if (m_comboMueble->currentIndex() < 0) {
        Toast::showToast(this, "Selecciona el mueble de bodega donde se ubicará el producto", Toast::Warning);
        return;
    }
    if (m_comboProveedor->currentIndex() < 0) {
        Toast::showToast(this, "Selecciona un proveedor válido", Toast::Warning);
        return;
    }

    // Obtener géneros seleccionados (Relación N:M)
    QJsonArray generosArr;
    for (int i = 0; i < m_listGeneros->count(); ++i) {
        auto *item = m_listGeneros->item(i);
        if (item->checkState() == Qt::Checked) {
            generosArr.append(item->data(Qt::UserRole).toInt());
        }
    }

    QJsonObject obj;
    obj["codigo_ean"] = ean;
    if (!sku.isEmpty()) {
        obj["sku"] = sku.toLongLong();
    }
    obj["id_mueble"] = m_comboMueble->currentData().toInt();
    obj["id_proveedor"] = m_comboProveedor->currentData().toInt();
    obj["precio"] = m_spnPrecio->value();
    obj["id_ubicacion"] = 2; // Bodega
    obj["generos"] = generosArr;

    if (m_tipoDetectado == DetectedType::Libro) {
        obj["nombre_libro"] = nombre;

        QJsonArray autoresArr;
        if (m_comboAutor->currentIndex() >= 0) {
            autoresArr.append(m_comboAutor->currentData().toInt());
        }
        obj["autores"] = autoresArr;

        ApiClient::instance()->registrarLibroBodega(obj, [this](bool ok, const QJsonValue &, const QString &msg) {
            if (ok) {
                Toast::showToast(this, "Libro registrado exitosamente en Bodega con géneros y autor asociados", Toast::Success);
                limpiarCampos();
            } else {
                Toast::showToast(this, "Error al registrar libro: " + msg, Toast::Error);
            }
        });
    } else {
        obj["nombre_revista"] = nombre;
        if (m_comboAutorEditorial->currentIndex() < 0) {
            Toast::showToast(this, "Selecciona la editorial de la revista", Toast::Warning);
            return;
        }
        obj["id_editorial"] = m_comboAutorEditorial->currentData().toInt();
        obj["numero_edicion"] = m_spnEdicion->value();
        QString periodicidad = m_comboPeriodicidad->currentData().toString();
        if (!periodicidad.isEmpty()) {
            obj["periodicidad"] = periodicidad;
        }

        ApiClient::instance()->registrarRevistaBodega(obj, [this](bool ok, const QJsonValue &, const QString &msg) {
            if (ok) {
                Toast::showToast(this, "Revista registrada exitosamente en Bodega con géneros asociados", Toast::Success);
                limpiarCampos();
            } else {
                Toast::showToast(this, "Error al registrar revista: " + msg, Toast::Error);
            }
        });
    }
}
