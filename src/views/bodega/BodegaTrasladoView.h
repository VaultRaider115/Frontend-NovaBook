#pragma once

#include "RequisicionView.h"

class BodegaTrasladoView : public RequisicionView {
    Q_OBJECT

public:
    explicit BodegaTrasladoView(QWidget *parent = nullptr)
        : RequisicionView("Bodega · movimiento", "Requisición de Salida Bodega > Piso de Venta",
                          "Registrar y Mandar a Piso", "Mercancía trasladada a Piso de Venta",
                          /*salida=*/true, parent) {}
};
