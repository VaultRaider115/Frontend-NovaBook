#pragma once

#include "CatalogView.h"
#include "Models.h"

class BodegaAutoresView : public CatalogView {
    Q_OBJECT

public:
    explicit BodegaAutoresView(const QString &eyebrow = "Jefatura · Gestión de autores", QWidget *parent = nullptr);
    void cargarAutores();

protected:
    void onNew() override { abrirFormulario(nullptr); }

private:
    void abrirFormulario(const AutorDto *existente);
    void eliminar(const AutorDto &a);

    QList<AutorDto> m_autores;
};
