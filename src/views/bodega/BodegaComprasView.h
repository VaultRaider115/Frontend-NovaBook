#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QList>
#include "SeccionesScroll.h"
#include "Models.h"

struct CompraCartItem {
    qint64 ean = 0;
    QString tipo;
    QString nombre;
    int cantidad = 0;      // piezas que dice la factura
    int recibidas = 0;     // piezas que llegaron (si son menos, se abre una incidencia)
    double costo_unitario = 0.0;
    double subtotal = 0.0;
};

class BodegaComprasView : public QWidget {
    Q_OBJECT

public:
    explicit BodegaComprasView(QWidget *parent = nullptr);
    void cargar();               // proveedores + facturas registradas
    void cargarProveedores();
    void cargarFacturas();

private slots:
    void onEanChanged(const QString &text);
    void onAgregarArticulo();
    void onQuitarFila(int row);
    void onRegistrarCompra();
    void limpiarCarrito();

private:
    void recalcularTotales();
    void mostrarSeccion(int i);
    void pintarFacturas();
    void verFactura(const CompraResponse &c);

    QComboBox *m_comboProveedor;
    QLineEdit *m_txtEan;
    QLabel *m_lblEanStatus;
    QSpinBox *m_spnCantidad;      // facturadas
    QSpinBox *m_spnRecibidas;     // llegaron
    QDoubleSpinBox *m_spnCosto;
    QPushButton *m_btnAgregar;

    QTableWidget *m_tableCart;
    QLineEdit *m_txtObs;
    QLabel *m_lblTotalArticulos;
    QLabel *m_lblTotalPagar;
    QPushButton *m_btnVaciar;
    QPushButton *m_btnProcesar;

    QList<CompraCartItem> m_items;
    QList<ProveedorDto> m_proveedores;

    // Una sola vista con scroll: registrar factura y, abajo, facturas registradas.
    SeccionesScroll *m_secciones;
    QLineEdit *m_txtBuscarFactura;
    QLabel *m_lblFacturas;
    QTableWidget *m_tablaFacturas;
    QList<CompraResponse> m_facturas;
    QList<CompraResponse> m_facturasVisibles;
};
