#pragma once

#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QWidget>

// Revisión del inventario mensual y solicitudes de editorial (diagrama, paso 7).
// El Jefe aprueba (ajusta el piso con las diferencias) o rechaza el conteo, y
// registra los títulos que una editorial pidió de vuelta por correo.
class JefeInventarioView : public QWidget {
    Q_OBJECT

public:
    explicit JefeInventarioView(QWidget *parent = nullptr);
    void cargar();

private:
    void mostrarSeccion(int i);
    void cargarInventarios();
    void cargarDetalle(qint64 id);
    void revisar(bool aprobar);
    void cargarSolicitudes();
    void nuevaSolicitud();

    QList<QPushButton *> m_segmentos;
    QStackedWidget *m_stack;

    // Inventarios
    QTableWidget *m_tablaInv;
    QLabel *m_lblDetalle;
    QPushButton *m_btnAprobar;
    QPushButton *m_btnRechazar;
    QTableWidget *m_tablaDet;
    QList<qint64> m_ids;
    qint64 m_idSel = 0;

    // Solicitudes
    QTableWidget *m_tablaSol;
};
