#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

class QWidget;

// Documentos imprimibles: guardar un PDF y abrirlo en el visor del sistema
// (desde ahí se imprime), y generar el PDF de una requisición.
namespace Documentos {

// Pide dónde guardar (por defecto Documentos/NovaBook), escribe el PDF y lo abre.
// Devuelve false si el usuario cancela o no se pudo escribir (ya avisó con un toast).
bool guardarYAbrir(QWidget *parent, const QByteArray &pdf, const QString &nombreSugerido);

struct PartidaReq {
    qint64 ean = 0;
    QString tipo;     // "Libro" / "Revista"
    QString titulo;
    int cantidad = 0;
};

struct Requisicion {
    QString folio;        // REQ-2026-15
    QString fechaHora;    // "2026-11-15 10:42:00"
    bool salida = true;   // true: Bodega → Piso (salida); false: Piso → Bodega (entrada)
    QString elaboro;      // nombre de quien la registró
    QString editorial;    // una sola editorial por documento (diagrama, pasos 2 y 6)
    QList<PartidaReq> partidas;
};

// PDF de la requisición (mismo estilo que el acta de devolución).
QByteArray pdfRequisicion(const Requisicion &req);

} // namespace Documentos
