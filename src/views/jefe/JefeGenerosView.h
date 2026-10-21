#pragma once

#include "CatalogView.h"
#include "Models.h"

class JefeGenerosView : public CatalogView {
    Q_OBJECT

public:
    explicit JefeGenerosView(QWidget *parent = nullptr);
    void cargarGeneros();

protected:
    void onNew() override { abrirFormulario(nullptr); }

private:
    void abrirFormulario(const GeneroDto *existente);
    void eliminar(const GeneroDto &g);

    QList<GeneroDto> m_generos;
};
