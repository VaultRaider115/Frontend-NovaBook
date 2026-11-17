#pragma once

#include "RequisicionView.h"

class VendedorTrasladoView : public RequisicionView {
    Q_OBJECT

public:
    explicit VendedorTrasladoView(QWidget *parent = nullptr)
        : RequisicionView("Vendedor · movimiento", "Requisición de Entrada Piso de Venta > Bodega",
                          "Registrar y Devolver a Bodega", "Mercancía reingresada a Bodega",
                          /*salida=*/false, parent) {}
};
