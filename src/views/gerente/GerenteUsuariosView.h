#pragma once

#include "CatalogView.h"
#include "Models.h"

/**
 * RF01 — CRUD Usuarios completo (crear, consultar, modificar, eliminar).
 * Exclusivo del Gerente / Subgerente.
 */
class GerenteUsuariosView : public CatalogView {
    Q_OBJECT

public:
    explicit GerenteUsuariosView(QWidget *parent = nullptr);
    void cargarUsuarios();

protected:
    void onNew() override { abrirFormulario(nullptr); }

private:
    void abrirFormulario(const UsuarioDto *existente);
    void darDeBaja(const UsuarioDto &u);
    void reactivar(const UsuarioDto &u);

    QList<UsuarioDto> m_usuarios;
    int m_filtroEstado = 0; // 0 todos, 1 activos, 2 inactivos
    QList<QPushButton *> m_segmentos;
};
