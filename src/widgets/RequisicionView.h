#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include "ApiClient.h"
#include "Documentos.h"

// Pantalla de requisición por lista (Bodega → Piso y Piso → Bodega).
// Se registra como un solo documento con folio (todo o nada) y se imprime en PDF.
class RequisicionView : public QWidget {
    Q_OBJECT
public:
    // salida = true: Bodega → Piso de venta; false: Piso de venta → Bodega.
    RequisicionView(const QString &eyebrow, const QString &title, const QString &sendText,
                    const QString &addedMessage, bool salida, QWidget *parent = nullptr);

private:
    struct Partida { qint64 ean; QString tipo; QString nombre; int cantidad; };
    void agregar();
    void render();
    void enviarTodo();
    void imprimirUltima();

    QLineEdit *m_txtEan;
    QLabel *m_lblStatus;
    QSpinBox *m_spnCantidad;
    QTableWidget *m_table;
    QLabel *m_lblTotal;
    QLabel *m_lblFolio;
    QPushButton *m_btnEnviar;
    QPushButton *m_btnImprimir;
    QList<Partida> m_items;
    QString m_okMessage;
    bool m_salida;
    Documentos::Requisicion m_ultima;   // última requisición registrada (para reimprimir)
};
