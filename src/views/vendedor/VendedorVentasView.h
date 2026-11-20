#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QList>
#include <QLabel>
#include "Models.h"
#include <QFrame>
#include <QJsonObject>

struct CartItem {
    qint64 ean = 0;
    QString tipo = "libro";
    QString nombre;
    QString autor;       // autor del libro o editorial de la revista
    QString editorial;
    int cantidad = 1;
    double precio = 0.0;
    double subtotal = 0.0;
};

class VendedorVentasView : public QWidget {
    Q_OBJECT

public:
    explicit VendedorVentasView(QWidget *parent = nullptr);

private slots:
    void onEanChanged(const QString &text);
    void onAgregarAlCarrito();
    void onQuitarFila(int row);
    void onProcesarVenta();
    void limpiarCarrito();

private:
    void recalcularTotales();
    void consultarProducto(const QString &ean);   // llena el recuadro del producto
    void mostrarProducto(const QJsonObject &p);
    static QJsonObject filaDeEan(const QJsonValue &data, qint64 ean);

    QLineEdit *m_txtCliente;
    QList<QPushButton *> m_btnsPago;
    QString m_metodoPago = "Efectivo";
    QLineEdit *m_txtEan;
    QLabel *m_lblEanStatus;
    QSpinBox *m_spnCantidad;
    QPushButton *m_btnAgregar;

    // Recuadro "Producto": qué libro se está vendiendo.
    QFrame *m_cardProducto;
    QLabel *m_lblProdTitulo;
    QLabel *m_lblProdDetalle;
    QLabel *m_lblProdPrecio;
    QLabel *m_lblProdStock;
    QLabel *m_lblProdTipo;
    QString m_eanConsultado;

    QTableWidget *m_tableCart;
    QLabel *m_lblTotalArticulos;
    QLabel *m_lblTotalPagar;
    QPushButton *m_btnProcesar;
    QPushButton *m_btnVaciar;

    QList<CartItem> m_cart;
};
