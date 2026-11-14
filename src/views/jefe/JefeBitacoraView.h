#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QDateEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "Models.h"
#include <QCheckBox>

class PagerBar;

class JefeBitacoraView : public QWidget {
    Q_OBJECT

public:
    explicit JefeBitacoraView(QWidget *parent = nullptr);
    void cargarMovimientos();

private:
    void renderPagina();
    void aplicarFiltros();
    
    QDateEdit *m_desde;
    QDateEdit *m_hasta;
    QComboBox *m_comboTipo;
    QCheckBox *m_chkOcultarConsultas;
    QPushButton *m_btnBuscar;
    QTableWidget *m_table;
    QLabel *m_lblTotal;
    PagerBar *m_pager;

    QList<MovimientoDiarioItem> m_todos;     // todo el rango consultado
    QList<MovimientoDiarioItem> m_filtrados; // tras filtrar por tipo de evento
};
