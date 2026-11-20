#pragma once

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
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
    // motivo: SOLICITUD_EDITORIAL / ANEJAMIENTO si va a devolución (se separa del piso).
    struct Conteo { qint64 ean; QString titulo; int cantidad; QString motivo; };

    void escanear();
    void fijarCantidad(qint64 ean, int cantidad);
    void pintar();
    void actualizarMotivos();
    void cargarDevolucion();   // encabezado y tabla de lo que va a devolución
    void cerrar();

    QLabel *m_lblEstado;
    QPushButton *m_btnIniciar;
    QWidget *m_panelConteo;
    QScrollArea *m_scrollConteo;
    QLineEdit *m_txtEan;
    QTableWidget *m_tabla;
    // Para devolución: contador general y "encontrados / faltantes" por título.
    QLabel *m_lblDevTitulos;
    QLabel *m_lblDevPiezas;
    QLabel *m_lblDevEncontradas;
    QLabel *m_lblDevFaltan;
    QTableWidget *m_tablaDev;
    int m_peticionDev = 0;
    QLabel *m_lblTotal;
    QPushButton *m_btnCerrar;

    qint64 m_idInventario = 0;
    QList<Conteo> m_conteos;
};
