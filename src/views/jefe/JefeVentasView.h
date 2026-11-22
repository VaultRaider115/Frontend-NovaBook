#pragma once

#include <QDateEdit>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

// Hito 2: el Jefe consulta los tickets de venta (solo lectura) por rango de
// fechas y por número de ticket, cliente o vendedor; el ojo abre el detalle
// con lo vendido y lo que el cliente ya devolvió.
class JefeVentasView : public QWidget {
    Q_OBJECT

public:
    explicit JefeVentasView(QWidget *parent = nullptr);
    void cargar();

private:
    void pintar();
    void verTicket(qint64 idVenta);

    QDateEdit *m_desde;
    QDateEdit *m_hasta;
    QLineEdit *m_txtBuscar;
    QPushButton *m_btnBuscar;
    QTableWidget *m_table;
    QLabel *m_lblTickets;
    QLabel *m_lblVendido;
    QLabel *m_lblPie;

    QList<QJsonObject> m_tickets;
    int m_consulta = 0;   // descarta respuestas de una consulta anterior
};
