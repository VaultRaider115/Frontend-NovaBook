#pragma once

#include "CatalogView.h"
#include "Models.h"

class JefeMueblesView : public CatalogView {
    Q_OBJECT

public:
    explicit JefeMueblesView(QWidget *parent = nullptr);
    void cargarMuebles();

protected:
    void onNew() override { abrirFormulario(nullptr); }

private:
    void abrirFormulario(const MuebleDto *existente);
    void eliminar(const MuebleDto &m);

    QList<MuebleDto> m_muebles;
    QList<GeneroDto> m_generos; // para el selector de género asignado
};
