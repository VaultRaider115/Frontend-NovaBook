#pragma once

#include "RequisicionView.h"

class VendedorTrasladoView : public RequisicionView {
    Q_OBJECT

public:
    explicit VendedorTrasladoView(QWidget *parent = nullptr)
        : RequisicionView("Vendedor · movimiento", "Requisicion de Entrada Piso de Venta > Bodega",
                          "Registrar y Devolver a Bodega", "Mercancía reingresada a Bodega", parent) {}

protected:
    void trasladar(qint64 ean, const QString &tipo, int cantidad, ApiClient::ApiCallback cb) override {
        if (tipo == "libro") ApiClient::instance()->trasladoTiendaLibros(ean, cantidad, QString(), cb);
        else ApiClient::instance()->trasladoTiendaRevistas(ean, cantidad, QString(), cb);
    }
};
