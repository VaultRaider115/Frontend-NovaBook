#include "VendedorConsultaView.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Toast.h"
#include "Models.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include "Ui.h"

VendedorConsultaView::VendedorConsultaView(QWidget *parent, bool paraBodega)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(20);
    layout->addWidget(Ui::pageHeader(paraBodega ? "Bodega · Inventario" : "Vendedor · Inventario",
                                     paraBodega ? "Inventario General" : "Consulta de Inventario General",
                                     "Consulta el stock en piso de venta y bodega de libros y revistas por título, autor, EAN o SKU.", this));

    auto *filters = Ui::card(this);
    auto *toolbar = new QHBoxLayout(filters);
    toolbar->setContentsMargins(16, 16, 16, 16);
    toolbar->setSpacing(12);

    m_txtBuscar = Ui::searchBox("Buscar por título, autor, EAN o SKU...", filters);
    toolbar->addWidget(m_txtBuscar, 2);

    m_comboTipo = new QComboBox(filters);
    m_comboTipo->addItem("Todos los Tipos", "");
    m_comboTipo->addItem("Libros", "libro");
    m_comboTipo->addItem("Revistas", "revista");
    m_comboTipo->setMinimumWidth(190);
    toolbar->addWidget(m_comboTipo);

    m_comboUbicacion = new QComboBox(filters);
    m_comboUbicacion->addItem("Toda la Sucursal", 0);
    m_comboUbicacion->addItem("Solo Piso de Tienda", 1);
    m_comboUbicacion->addItem("Solo Almacén / Bodega", 2);
    m_comboUbicacion->setMinimumWidth(210);
    toolbar->addWidget(m_comboUbicacion);

    m_btnBuscar = Ui::button("Buscar", "primary", filters, "search-dark");
    toolbar->addWidget(m_btnBuscar);
    m_btnLimpiar = Ui::button("Restablecer filtros", "outline", filters, "refresh");
    toolbar->addWidget(m_btnLimpiar);
    layout->addWidget(filters);

    m_table = Ui::table({"EAN", "SKU", "Título", "Tipo", "Autor / Editorial", "Géneros / Categoría", "Stock Tienda", "Stock Bodega", "Precio"}, 2, this, 64);
    // Título y autor ajustan línea (como en el diseño) para que las 9 columnas quepan;
    // las numéricas llevan ancho fijo para no acaparar espacio.
    auto *hh = m_table->horizontalHeader();
    hh->setSectionResizeMode(4, QHeaderView::Stretch);
    for (int c : {6, 7, 8}) {
        hh->setSectionResizeMode(c, QHeaderView::Fixed);
        m_table->setColumnWidth(c, c == 8 ? 96 : 108);
    }
    m_table->setWordWrap(true);
    layout->addWidget(m_table, 1);

    connect(m_btnBuscar, &QPushButton::clicked, this, &VendedorConsultaView::buscarExistencias);
    connect(m_txtBuscar, &QLineEdit::returnPressed, this, &VendedorConsultaView::buscarExistencias);
    connect(m_comboTipo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VendedorConsultaView::buscarExistencias);
    connect(m_comboUbicacion, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VendedorConsultaView::buscarExistencias);
    connect(m_btnLimpiar, &QPushButton::clicked, this, [this]() {
        m_txtBuscar->clear();
        m_comboTipo->setCurrentIndex(0);
        m_comboUbicacion->setCurrentIndex(0);
        buscarExistencias();
    });
}

void VendedorConsultaView::buscarExistencias() {
    QString q = m_txtBuscar->text().trimmed();
    QString tipo = m_comboTipo->currentData().toString();
    int ubicacion = m_comboUbicacion->currentData().toInt();

    ApiClient::instance()->consultarExistencias(q, tipo, ubicacion, [this](bool ok, const QJsonValue &data, const QString &msg) {
        if (!ok || !data.isArray()) {
            Toast::showToast(this, "Error al buscar: " + msg, Toast::Error);
            return;
        }

        QJsonArray arr = data.toArray();
        m_table->clearContents();
        m_table->setRowCount(arr.size());

        for (int i = 0; i < arr.size(); ++i) {
            ExistenciaInventario ex = ExistenciaInventario::fromJson(arr.at(i).toObject());
            Ui::cellText(m_table, i, 0, QString::number(ex.codigo_ean), true);
            Ui::cellText(m_table, i, 1, ex.sku > 0 ? QString::number(ex.sku) : "—", true);
            Ui::cellText(m_table, i, 2, ex.titulo, false, true);
            Ui::cellWidget(m_table, i, 3, Ui::tipoChip(ex.tipo_producto));
            Ui::cellText(m_table, i, 4, ex.autor_o_editorial.isEmpty() ? "—" : ex.autor_o_editorial);

            // Géneros apilados (uno por línea), como en el diseño.
            auto *gen = new QWidget();
            auto *gl = new QVBoxLayout(gen);
            gl->setContentsMargins(0, 0, 0, 0);
            gl->setSpacing(4);
            QStringList lista = ex.generos.split(',', Qt::SkipEmptyParts);
            if (lista.isEmpty()) lista << "General";
            for (const QString &g : std::as_const(lista)) gl->addWidget(Ui::chip(g.trimmed(), "neutral"), 0, Qt::AlignLeft);
            m_table->setCellWidget(i, 5, gen);
            Ui::fitColumn(m_table, 5, gen);

            // Stock en cero se marca en rojo (el color no es el único indicador: el número se muestra siempre)
            if (ex.stock_tienda == 0) Ui::cellWidget(m_table, i, 6, Ui::chip("0", "danger"));
            else Ui::cellText(m_table, i, 6, QString::number(ex.stock_tienda), false, true);
            if (ex.stock_bodega == 0) Ui::cellWidget(m_table, i, 7, Ui::chip("0", "danger"));
            else Ui::cellText(m_table, i, 7, QString::number(ex.stock_bodega), false, true);

            Ui::cellText(m_table, i, 8, Ui::money(ex.precio), false, true, Theme::Accent);
        }
    });
}
