#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QDateEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "Models.h"

class JefeBitacoraView : public QWidget {
    Q_OBJECT

public:
    explicit JefeBitacoraView(QWidget *parent = nullptr);
    // Vuelve a consultar desde el primer evento (filtros nuevos o al entrar).
    void cargarMovimientos();

private:
    // Sin páginas: la tabla se recorre con scroll y, al llegar al final, se
    // piden los siguientes eventos al servidor y se agregan abajo.
    void cargarLote(bool desdeInicio);
    void cargarMas();
    void agregarFilas(int desde);
    void actualizarPie();

    QDateEdit *m_desde;
    QDateEdit *m_hasta;
    QComboBox *m_comboTipo;
    QPushButton *m_btnBuscar;
    QTableWidget *m_table;
    QLabel *m_lblTotal;
    QLabel *m_lblMostrando;

    QList<MovimientoDiarioItem> m_filas;    // eventos ya cargados (en orden)
    int m_total = 0;                        // eventos que cumplen el filtro
    int m_siguiente = 0;                    // siguiente lote que se pide al servidor
    int m_consulta = 0;                     // descarta respuestas de una consulta anterior
    bool m_cargando = false;
};
