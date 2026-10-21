#pragma once

#include "CatalogView.h"
#include "Models.h"

class JefeUsuariosView : public CatalogView {
    Q_OBJECT

public:
    explicit JefeUsuariosView(QWidget *parent = nullptr);
    void cargarUsuarios();

protected:
    void onNew() override { abrirFormulario(nullptr); }

private:
    void abrirFormulario(const UsuarioDto *existente);
    void darDeBaja(const UsuarioDto &u);

    QList<UsuarioDto> m_usuarios;
    int m_filtroEstado = 0; // 0 todos, 1 activos, 2 inactivos
    QList<QPushButton *> m_segmentos;
};
