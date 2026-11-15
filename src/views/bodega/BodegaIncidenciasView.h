#pragma once

#include <QComboBox>
#include <QFrame>
#include <QJsonObject>
#include <QLabel>
#include <QList>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QWidget>

// Incidencias por paquete incompleto (diagrama de procesos, paso 1).
// Se abren solas al registrar una compra donde llegó menos de lo facturado.
// Aquí Bodega avisa al proveedor por correo (se abre el programa de correo del
// equipo con el texto listo) y registra los artículos restantes cuando llegan;
// al completarse, la incidencia se cierra.
class BodegaIncidenciasView : public QWidget {
    Q_OBJECT

public:
    explicit BodegaIncidenciasView(QWidget *parent = nullptr);
    void cargar();

private:
    void mostrar(const QJsonObject &inc);
    void registrarLlegada();

    QComboBox *m_comboEstado;
    QPushButton *m_btnActualizar;
    QTableWidget *m_lista;
    QLabel *m_lblAbiertas;

    QFrame *m_detalle;
    QLabel *m_lblTitulo;
    QLabel *m_lblSub;
    QTableWidget *m_tabla;
    QLabel *m_lblResumen;
    QPushButton *m_btnCorreo;
    QPushButton *m_btnLlegada;
    QList<QSpinBox *> m_llegaron;
    QJsonObject m_actual;
};
