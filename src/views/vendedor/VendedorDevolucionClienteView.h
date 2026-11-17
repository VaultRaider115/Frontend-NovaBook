#pragma once

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QWidget>

// Devolución de cliente con ticket (diagrama de procesos, paso 5):
// - Ticket de 15 días o menos; el cliente puede regresar solo algunos libros.
// - Sin defecto regresa a piso; con defecto queda "Apartado" y no se vende.
// - Reembolso, o cambio por productos de igual o mayor precio pagando la diferencia.
class VendedorDevolucionClienteView : public QWidget {
    Q_OBJECT

public:
    explicit VendedorDevolucionClienteView(QWidget *parent = nullptr);

private:
    struct Linea {      // renglón del ticket
        qint64 ean = 0;
        QString titulo;
        QString tipo;
        int vendida = 0;
        int devuelta = 0;
        double precio = 0;
        QSpinBox *spnDevolver = nullptr;
        QCheckBox *chkDefecto = nullptr;
    };
    struct Cambio {     // producto que se lleva el cliente
        qint64 ean = 0;
        QString titulo;
        double precio = 0;
        int cantidad = 0;
    };

    void buscarTicket();
    void pintarTicket();
    void agregarCambio();
    void pintarCambios();
    void setResolucion(bool cambio);
    void recalcular();
    void registrar();
    void limpiar();

    QLineEdit *m_txtTicket;
    QPushButton *m_btnBuscar;
    QLabel *m_lblTicket;
    QTableWidget *m_tablaTicket;

    QPushButton *m_btnReembolso;
    QPushButton *m_btnCambio;
    QWidget *m_panelCambio;
    QLineEdit *m_txtEanCambio;
    QSpinBox *m_spnCantCambio;
    QTableWidget *m_tablaCambio;
    QList<QPushButton *> m_btnsPago;
    QString m_metodoPago = "EFECTIVO";

    QLabel *m_lblDevuelto;
    QLabel *m_lblNuevo;
    QLabel *m_lblResultado;
    QPushButton *m_btnRegistrar;

    qint64 m_idVenta = 0;
    bool m_vigente = false;
    bool m_esCambio = false;
    QList<Linea> m_lineas;
    QList<Cambio> m_cambios;
};
