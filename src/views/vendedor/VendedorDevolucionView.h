#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QList>
#include <QPair>
#include "Models.h"

struct ReturnItemDraft {
    qint64 ean = 0;
    QString tipo = "libro";
    QString nombre;
    int cantidad = 1;
    QString motivo;
    int id_motivo = 0;
};

class VendedorDevolucionView : public QWidget {
    Q_OBJECT

public:
    explicit VendedorDevolucionView(QWidget *parent = nullptr);
    void cargarCatalogos();   // proveedores y motivos

private slots:
    void onEanChanged(const QString &text);
    void onAgregarArticulo();
    void onEnviarDevolucion();
    void onDescargarPdf();

private:
    void renderItems();
    void actualizarProveedor();

    QComboBox *m_comboProveedor;
    QLabel *m_lblRfc;

    QLineEdit *m_txtEan;
    QLabel *m_lblEanStatus;
    QSpinBox *m_spnCantidad;
    QComboBox *m_comboMotivo;
    QPushButton *m_btnAgregar;

    QTableWidget *m_tableItems;
    QLabel *m_lblTotal;
    QLabel *m_lblLote;
    QPushButton *m_btnEnviar;
    QPushButton *m_btnPdf;

    QList<ReturnItemDraft> m_items;
    QList<ProveedorDto> m_proveedores;
    QList<QPair<qint64, QString>> m_ultimas; // (id_devolucion, tipo) de la última solicitud registrada
};
