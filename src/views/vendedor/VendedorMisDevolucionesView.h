#pragma once

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>
#include "Models.h"

// Seguimiento de las solicitudes de devolución del propio vendedor
// (diagrama de procesos, pasos 9–10): ver si el Jefe aprobó o rechazó,
// descargar el acta aprobada para imprimirla y leer el motivo del rechazo.
class VendedorMisDevolucionesView : public QWidget {
    Q_OBJECT

public:
    explicit VendedorMisDevolucionesView(QWidget *parent = nullptr);
    void cargar();

private:
    void verActa(const Devolucion &d);

    QComboBox *m_comboEstado;
    QPushButton *m_btnBuscar;
    QTableWidget *m_table;
    QLabel *m_lblPendientes;
};
