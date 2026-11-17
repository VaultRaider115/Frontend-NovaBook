#pragma once

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QWidget>
#include "Documentos.h"
#include "Models.h"

// Recepción de la requisición de salida (diagrama de procesos, pasos 2 y 3).
// El Vendedor abre una salida PENDIENTE, confirma lo que llegó de cada libro
// y elige el mueble del piso donde lo coloca. Si algo no coincide, captura lo
// recibido: la API corrige el documento (stock y movimientos) y se reimprime.
// Con puedeRecibir = false (Bodega) la pantalla es de seguimiento: ve el
// estado de sus salidas y reimprime.
class VendedorRecepcionView : public QWidget {
    Q_OBJECT

public:
    explicit VendedorRecepcionView(bool puedeRecibir, QWidget *parent = nullptr);
    void cargar();

private:
    struct Fila {
        QSpinBox *recibido = nullptr;
        QComboBox *mueble = nullptr;
        QLabel *estado = nullptr;
    };

    void cargarLista();
    void abrir(const QString &folio);
    void mostrarDetalle(const Documentos::Requisicion &req, const QJsonObject &json);
    void actualizarDiferencias();
    void enviar();
    void imprimir(const Documentos::Requisicion &req);

    bool m_puedeRecibir;
    QComboBox *m_comboEstado;
    QPushButton *m_btnActualizar;
    QTableWidget *m_lista;
    QLabel *m_lblPendientes;

    QFrame *m_detalle;
    QLabel *m_lblDetalleTitulo;
    QLabel *m_lblDetalleSub;
    QTableWidget *m_tabla;
    QLineEdit *m_txtObs;
    QLabel *m_lblResumen;
    QPushButton *m_btnCerrar;
    QPushButton *m_btnImprimir;
    QPushButton *m_btnEnviar;

    QList<MuebleDto> m_mueblesPiso;
    QList<Fila> m_filas;
    QJsonObject m_actualJson;
    Documentos::Requisicion m_actual;
};
