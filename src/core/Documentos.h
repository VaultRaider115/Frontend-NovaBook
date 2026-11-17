#pragma once

#include <QByteArray>
#include <QJsonObject>
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
    int cantidad = 0;      // lo que dice el documento original
    int recibida = -1;     // lo que confirmó el Vendedor (-1: aún sin recibir)
    QString mueble;        // mueble de piso donde quedó (salida recibida)
};

struct Requisicion {
    QString folio;        // REQ-2026-15
    QString fechaHora;    // "2026-11-15 10:42:00"
    bool salida = true;   // true: Bodega → Piso (salida); false: Piso → Bodega (entrada)
    QString elaboro;      // nombre de quien la registró
    QString editorial;    // una sola editorial por documento (diagrama, pasos 2 y 6)
    QList<PartidaReq> partidas;
    // Recepción en piso (pasos 2 y 3)
    QString estado;          // PENDIENTE | CONFIRMADA | CORREGIDA | REGISTRADA
    QString recibio;
    QString fechaRecepcion;
    QString observaciones;

    // Desde GET /requisiciones/{folio} o la respuesta de la recepción.
    static Requisicion desdeJson(const QJsonObject &o);
};

// PDF de la requisición (mismo estilo que el acta de devolución).
// Si ya se recibió, lleva lo recibido y el mueble; si se corrigió, la marca CORREGIDA.
QByteArray pdfRequisicion(const Requisicion &req);

// Abre el programa de correo del equipo con el borrador que arma la API
// ({para, asunto, cuerpo}). Si no hay destinatario o no se pudo abrir, copia
// el texto al portapapeles y avisa. Devuelve true si se abrió el correo.
bool abrirCorreo(QWidget *parent, const QJsonObject &correo);

} // namespace Documentos
