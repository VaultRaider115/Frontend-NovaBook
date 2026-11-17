#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QDateEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "Models.h"

class PagerBar;

class JefeBitacoraView : public QWidget {
    Q_OBJECT

public:
    explicit JefeBitacoraView(QWidget *parent = nullptr);
    void cargarMovimientos();

private:
    void renderPagina();

    QDateEdit *m_desde;
    QDateEdit *m_hasta;
    QComboBox *m_comboTipo;
    QPushButton *m_btnBuscar;
    QTableWidget *m_table;
    QLabel *m_lblTotal;
    PagerBar *m_pager;

    QList<MovimientoDiarioItem> m_pagina;   // filas de la página actual (paginación en el servidor)
    int m_total = 0;                        // eventos que cumplen el filtro
    bool m_cargando = false;
};
