#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QList>
#include "Models.h"

class BodegaProveedoresView : public QWidget {
    Q_OBJECT

public:
    // puedeEditar = false deja solo el alta (Bodega registra un proveedor nuevo
    // al recibir la compra); editar y dar de baja es del Jefe.
    explicit BodegaProveedoresView(const QString &kicker, bool puedeEditar, QWidget *parent = nullptr);
    void cargarProveedores();

private slots:
    void onCrearProveedor();
    void onEditarProveedor();
    void onEliminarProveedor();

private:
    QTableWidget *m_table;
    QPushButton *m_btnNuevo;
    QPushButton *m_btnEditar;
    QPushButton *m_btnEliminar;
    QPushButton *m_btnRecargar;
    QLabel *m_lblTotal;
    QList<ProveedorDto> m_proveedores;
};
