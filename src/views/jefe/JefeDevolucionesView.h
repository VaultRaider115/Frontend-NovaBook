#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QDateEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include "Models.h"

class JefeDevolucionesView : public QWidget {
    Q_OBJECT

public:
    // puedeAprobar = false: modo consulta para el Gerente (ve el historial y
    // descarga actas; aprobar/rechazar es solo del Jefe).
    explicit JefeDevolucionesView(bool puedeAprobar = true, QWidget *parent = nullptr);
    void cargarDevoluciones();

private:
    bool m_puedeAprobar;
    void evaluar(const Devolucion &dev, bool aprobar);
    void verActaPdf(const Devolucion &dev);

    QDateEdit *m_fecha;
    QCheckBox *m_chkTodasFechas;
    QComboBox *m_comboEstado;
    QPushButton *m_btnHoy;
    QPushButton *m_btnBuscar;
    QTableWidget *m_table;
    QLabel *m_lblTotal;
    QList<Devolucion> m_devoluciones;
};
