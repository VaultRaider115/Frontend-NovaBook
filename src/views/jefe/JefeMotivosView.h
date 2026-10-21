#pragma once

#include "CatalogView.h"
#include "Models.h"

class JefeMotivosView : public CatalogView {
    Q_OBJECT

public:
    explicit JefeMotivosView(QWidget *parent = nullptr);
    void cargarMotivos();

protected:
    void onNew() override { abrirFormulario(nullptr); }

private:
    void abrirFormulario(const MotivoDto *existente);
    void eliminar(const MotivoDto &m);

    QList<MotivoDto> m_motivos;
};
