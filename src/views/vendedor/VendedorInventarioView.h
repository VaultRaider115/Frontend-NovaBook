#pragma once

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

// Inventario mensual (diagrama de procesos, paso 7): el Vendedor escanea cada
// libro del piso de venta y cierra el conteo; el Jefe lo revisa después.
// Mientras cuenta no ve el stock del sistema (para no influir en el conteo).
class VendedorInventarioView : public QWidget {
    Q_OBJECT

public:
    explicit VendedorInventarioView(QWidget *parent = nullptr);
    void cargar();

private:
    struct Conteo { qint64 ean; QString titulo; int cantidad; };

    void escanear();
    void fijarCantidad(qint64 ean, int cantidad);
    void pintar();
    void cerrar();

    QLabel *m_lblEstado;
    QPushButton *m_btnIniciar;
    QWidget *m_panelConteo;
    QLineEdit *m_txtEan;
    QTableWidget *m_tabla;
    QLabel *m_lblTotal;
    QPushButton *m_btnCerrar;

    qint64 m_idInventario = 0;
    QList<Conteo> m_conteos;
};
