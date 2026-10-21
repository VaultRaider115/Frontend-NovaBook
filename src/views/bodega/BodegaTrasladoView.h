#pragma once

#include "RequisicionView.h"

class BodegaTrasladoView : public RequisicionView {
    Q_OBJECT

public:
    explicit BodegaTrasladoView(QWidget *parent = nullptr)
        : RequisicionView("Bodega · movimiento", "Requisición de Salida Bodega > Piso de Venta",
                          "Registrar y Mandar a Piso", "Mercancía trasladada a Piso de Venta", parent) {}

protected:
    void trasladar(qint64 ean, const QString &tipo, int cantidad, ApiClient::ApiCallback cb) override {
        if (tipo == "libro") ApiClient::instance()->trasladoBodegaLibros(ean, cantidad, QString(), cb);
        else ApiClient::instance()->trasladoBodegaRevistas(ean, cantidad, QString(), cb);
    }
};
