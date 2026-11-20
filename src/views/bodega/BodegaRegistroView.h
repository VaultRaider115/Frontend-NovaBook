#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QTimer>
#include "Models.h"

class BodegaRegistroView : public QWidget {
    Q_OBJECT

public:
    explicit BodegaRegistroView(QWidget *parent = nullptr);
    void cargarCatalogos();
    // Alta desde Recepción de mercancía: deja el EAN puesto (y bloqueado) y
    // elige el proveedor de la factura en cuanto se carguen los catálogos.
    void prepararAlta(const QString &ean, int idProveedor);

signals:
    // Se emite al guardar el producto (antes de limpiar el formulario).
    void productoRegistrado(qint64 ean, const QString &titulo, const QString &tipo);

private slots:
    void onEanChanged(const QString &text);
    void verificarLibroExistente();
    void onRegistrar();
    void limpiarCampos();

private:
    enum class DetectedType { Desconocido, Libro, Revista };
    DetectedType m_tipoDetectado = DetectedType::Desconocido;

    QLineEdit *m_txtEan;
    QLabel *m_lblEanStatus;
    QLineEdit *m_txtSku;
    QLineEdit *m_txtNombre;
    QDoubleSpinBox *m_spnPrecio;

    // Selector Múltiple de Géneros (Relación N:M)
    QListWidget *m_listGeneros;

    // Campos dinámicos de Libro
    QWidget *m_panelLibro;
    QComboBox *m_comboAutor;

    // Campos dinámicos de Revista
    QWidget *m_panelRevista;
    QComboBox *m_comboAutorEditorial;
    QSpinBox *m_spnEdicion;
    QComboBox *m_comboPeriodicidad;

    QComboBox *m_comboMueble;
    QComboBox *m_comboProveedor;
    QPushButton *m_btnGuardar;
    QPushButton *m_btnLimpiar;
    QTimer *m_timerVerificarEan;

    QList<GeneroDto> m_generos;
    QList<AutorDto> m_autores;
    QList<ProveedorDto> m_proveedores;
    int m_proveedorInicial = 0;   // prepararAlta: proveedor a elegir al cargar
    void elegirProveedorInicial();
};
