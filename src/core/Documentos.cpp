#include "Documentos.h"
#include "Toast.h"
#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QJsonArray>
#include <QDate>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QLocale>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QStandardPaths>
#include <QUrl>

namespace Documentos {

Requisicion Requisicion::desdeJson(const QJsonObject &o) {
    Requisicion r;
    r.folio = o.value("folio").toString();
    r.fechaHora = o.value("fecha_hora").toString();
    r.salida = o.value("tipo").toString("SALIDA") == "SALIDA";
    r.elaboro = o.value("registro").toString();
    r.editorial = o.value("editorial").toString();
    r.estado = o.value("estado").toString();
    r.recibio = o.value("recibio").toString();
    r.fechaRecepcion = o.value("fecha_recepcion").toString();
    r.observaciones = o.value("observaciones").toString();
    for (const auto &v : o.value("items").toArray()) {
        const QJsonObject it = v.toObject();
        PartidaReq p;
        p.ean = it.value("codigo_ean").toVariant().toLongLong();
        p.tipo = it.value("tipo_producto").toString();
        p.titulo = it.value("titulo").toString();
        p.cantidad = it.value("cantidad").toInt();
        p.recibida = it.value("cantidad_recibida").isNull() ? -1 : it.value("cantidad_recibida").toInt(-1);
        if (p.recibida >= 0 && r.salida) p.mueble = it.value("mueble").toString();
        r.partidas.append(p);
    }
    return r;
}

bool abrirCorreo(QWidget *parent, const QJsonObject &correo) {
    const QString para = correo.value("para").toString().trimmed();
    const QString asunto = correo.value("asunto").toString();
    QString cuerpo = correo.value("cuerpo").toString();
    if (para.isEmpty()) {
        QApplication::clipboard()->setText(asunto + "\n\n" + cuerpo);
        Toast::showToast(parent, "El proveedor no tiene correo registrado: el texto del aviso se copió al portapapeles",
                         Toast::Warning, 7000);
        return false;
    }
    // mailto: con asunto y cuerpo codificados (saltos de línea como CRLF).
    cuerpo.replace("\n", "\r\n");
    const QByteArray url = "mailto:" + QUrl::toPercentEncoding(para, "@") + "?subject=" + QUrl::toPercentEncoding(asunto)
                           + "&body=" + QUrl::toPercentEncoding(cuerpo);
    if (QDesktopServices::openUrl(QUrl::fromEncoded(url))) {
        Toast::showToast(parent, "Se abrió el correo para " + para + ": revísalo y envíalo", Toast::Success, 6000);
        return true;
    }
    QApplication::clipboard()->setText(cuerpo);
    Toast::showToast(parent, "No hay un programa de correo configurado: el texto se copió al portapapeles para enviarlo a " + para,
                     Toast::Warning, 8000);
    return false;
}

bool guardarYAbrir(QWidget *parent, const QByteArray &pdf, const QString &nombreSugerido) {
    if (pdf.isEmpty()) {
        Toast::showToast(parent, "El documento llegó vacío", Toast::Error);
        return false;
    }
    const QString base = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).filePath("NovaBook");
    QDir().mkpath(base);
    const QString ruta = QFileDialog::getSaveFileName(parent, "Guardar PDF",
        QDir(base).filePath(nombreSugerido), "Archivos PDF (*.pdf)");
    if (ruta.isEmpty()) return false;

    QFile f(ruta);
    if (!f.open(QIODevice::WriteOnly) || f.write(pdf) != pdf.size()) {
        Toast::showToast(parent, "No se pudo guardar el PDF en " + ruta, Toast::Error);
        return false;
    }
    f.close();

    // Se abre en el visor predeterminado; desde ahí se imprime.
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(ruta)))
        Toast::showToast(parent, "PDF guardado: " + QFileInfo(ruta).fileName() + " (no se pudo abrir el visor)", Toast::Warning);
    else
        Toast::showToast(parent, "PDF guardado y abierto: " + QFileInfo(ruta).fileName(), Toast::Success);
    return true;
}

// ---------------------------------------------------------------------------
// Requisición en PDF. Se dibuja en puntos tipográficos sobre hoja carta con
// el mismo aspecto que el acta (docs/disenos/21.png).
// ---------------------------------------------------------------------------
namespace {
constexpr qreal kAncho = 612, kAlto = 792, kMargen = 50, kDerecha = kAncho - kMargen;
constexpr qreal kColEan = kMargen, kColTipo = 150, kColDesc = 205, kColCantDer = kDerecha, kFila = 26;

QFont fuente(qreal pt, bool negrita) {
    QFont f("Arial");
    f.setPointSizeF(pt);
    f.setBold(negrita);
    return f;
}

void texto(QPainter &p, qreal x, qreal yBase, const QString &s, qreal pt, bool negrita, int gris = 25,
           Qt::Alignment al = Qt::AlignLeft) {
    p.setFont(fuente(pt, negrita));
    p.setPen(QColor(gris, gris, gris));
    const qreal w = QFontMetricsF(p.font()).horizontalAdvance(s);
    if (al & Qt::AlignRight) x -= w;
    else if (al & Qt::AlignHCenter) x -= w / 2;
    p.drawText(QPointF(x, yBase), s);
}

void linea(QPainter &p, qreal x1, qreal y, qreal x2, qreal grosor, int gris) {
    p.setPen(QPen(QColor(gris, gris, gris), grosor));
    p.drawLine(QPointF(x1, y), QPointF(x2, y));
}

QString fechaLarga(const QString &fechaHora) {
    const QDateTime dt = QDateTime::fromString(fechaHora.left(19), "yyyy-MM-dd HH:mm:ss");
    if (!dt.isValid()) return fechaHora;
    return QLocale(QLocale::Spanish, QLocale::Mexico).toString(dt, "d 'de' MMMM 'de' yyyy, HH:mm");
}
} // namespace

QByteArray pdfRequisicion(const Requisicion &req) {
    QByteArray out;
    QBuffer buf(&out);
    buf.open(QIODevice::WriteOnly);

    QPdfWriter pdf(&buf);
    pdf.setTitle(QString("Requisición %1").arg(req.folio));
    pdf.setCreator("NovaBook");
    pdf.setPageLayout(QPageLayout(QPageSize(QPageSize::Letter), QPageLayout::Portrait, QMarginsF(0, 0, 0, 0)));
    pdf.setResolution(72);   // 1 unidad = 1 punto

    QPainter p(&pdf);
    p.setRenderHint(QPainter::Antialiasing);

    const QString titulo = req.salida ? "Requisición de Salida Bodega > Piso de Venta"
                                      : "Requisición de Entrada Piso de Venta > Bodega";
    const bool recibida = !req.partidas.isEmpty() && req.partidas.first().recibida >= 0;
    const bool corregida = req.estado == "CORREGIDA";
    const bool conMueble = recibida && req.salida;
    // Con mueble, la descripción cede espacio a la columna MUEBLE.
    constexpr qreal kColMueble = 398;
    const qreal finDesc = conMueble ? kColMueble - 10 : kColCantDer - 50;
    auto cantidadDe = [](const PartidaReq &it) { return it.recibida >= 0 ? it.recibida : it.cantidad; };
    int total = 0;
    for (const auto &it : req.partidas) total += cantidadDe(it);

    auto encabezadoTabla = [&](qreal y) {
        texto(p, kColEan + 4, y, "EAN", 7.5, true, 90);
        texto(p, kColTipo, y, "TIPO", 7.5, true, 90);
        texto(p, kColDesc, y, "DESCRIPCIÓN", 7.5, true, 90);
        if (conMueble) texto(p, kColMueble, y, "MUEBLE", 7.5, true, 90);
        texto(p, kColCantDer, y, recibida ? "RECIBIDO" : "CANT.", 7.5, true, 90, Qt::AlignRight);
        linea(p, kMargen, y + 7, kDerecha, 1.2, 25);
        return y + 7;
    };

    qreal y = 70;
    texto(p, kMargen, y, titulo, 15, true, 20);
    if (corregida) {
        // Sello: el documento se reimprimió con lo que realmente llegó.
        const QString sello = "CORREGIDA";
        const qreal w = QFontMetricsF(fuente(9, true), &pdf).horizontalAdvance(sello) + 16;
        p.setPen(QPen(QColor(180, 83, 9), 1.2));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(kDerecha - w, y - 14, w, 19), 3, 3);
        p.setFont(fuente(9, true));
        p.setPen(QColor(180, 83, 9));
        p.drawText(QPointF(kDerecha - w + 8, y), sello);
    }
    y += 18;
    linea(p, kMargen, y, kDerecha, 0.5, 215);
    y += 20;
    texto(p, kMargen, y, "Elaboró: ", 9.5, true);
    // Métricas del dispositivo PDF (72 dpi), no de la pantalla.
    texto(p, kMargen + QFontMetricsF(fuente(9.5, true), &pdf).horizontalAdvance("Elaboró: "), y,
          req.elaboro.isEmpty() ? "—" : req.elaboro, 9.5, false);
    texto(p, kDerecha, y, "Folio: #" + req.folio, 9.5, true, 25, Qt::AlignRight);
    y += 13;
    QString ruta = req.salida ? "Origen: Bodega · Destino: Piso de venta"
                              : "Origen: Piso de venta · Destino: Bodega";
    if (!req.editorial.isEmpty()) ruta = "Editorial: " + req.editorial + " · " + ruta;
    texto(p, kMargen, y, ruta, 8, false, 115);
    texto(p, kDerecha, y, fechaLarga(req.fechaHora), 8, false, 115, Qt::AlignRight);
    y += 12;
    linea(p, kMargen, y, kDerecha, 0.5, 215);

    y += 24;
    y = encabezadoTabla(y);
    for (const auto &it : req.partidas) {
        if (y + kFila > kAlto - 150) {
            pdf.newPage();
            y = encabezadoTabla(70);
        }
        const qreal base = y + 17;
        texto(p, kColEan + 4, base, QString::number(it.ean), 9, false);
        texto(p, kColTipo, base, it.tipo, 9, false);
        const QFontMetricsF fm(fuente(9, true), &pdf);
        texto(p, kColDesc, base, fm.elidedText(it.titulo, Qt::ElideRight, finDesc - kColDesc), 9, true);
        if (conMueble) {
            const QFontMetricsF fmM(fuente(8.5, false), &pdf);
            texto(p, kColMueble, base, fmM.elidedText(it.mueble, Qt::ElideRight, kColCantDer - kColMueble - 60), 8.5, false, 60);
        }
        const bool difiere = it.recibida >= 0 && it.recibida != it.cantidad;
        const QString cant = QString::number(cantidadDe(it));
        texto(p, kColCantDer, base, cant, 9, difiere, 25, Qt::AlignRight);
        if (difiere) {
            // Lo que decía el documento original, tachado a la izquierda.
            const QString antes = QString::number(it.cantidad);
            const qreal xA = kColCantDer - QFontMetricsF(fuente(9, true), &pdf).horizontalAdvance(cant) - 8;
            texto(p, xA, base, antes, 8, false, 140, Qt::AlignRight);
            linea(p, xA - QFontMetricsF(fuente(8, false), &pdf).horizontalAdvance(antes), base - 3, xA, 0.6, 140);
        }
        y += kFila;
        linea(p, kMargen, y, kDerecha, 0.5, 225);
    }

    y += 18;
    texto(p, kDerecha, y, QString("Piezas totales: %1").arg(total), 9.5, true, 25, Qt::AlignRight);
    const QFontMetricsF fmNota(fuente(8, false), &pdf);
    if (recibida) {
        QString nota = corregida ? "Corregida al recibir: la cantidad tachada es la del documento original."
                                 : "Recibida en piso sin diferencias.";
        if (!req.recibio.isEmpty())
            nota += " Recibió: " + req.recibio + (req.fechaRecepcion.isEmpty() ? "" : ", " + fechaLarga(req.fechaRecepcion)) + ".";
        y += 16;
        texto(p, kMargen, y, fmNota.elidedText(nota, Qt::ElideRight, kDerecha - kMargen), 8, false, 90);
    }
    if (!req.observaciones.trimmed().isEmpty()) {
        y += 13;
        texto(p, kMargen, y, fmNota.elidedText("Observaciones: " + req.observaciones.trimmed(), Qt::ElideRight, kDerecha - kMargen),
              8, false, 90);
    }

    y += 70;
    const qreal mitad = (kMargen + kDerecha) / 2;
    linea(p, kMargen, y, mitad - 16, 0.6, 90);
    linea(p, mitad + 16, y, kDerecha, 0.6, 90);
    texto(p, (kMargen + mitad - 16) / 2, y + 13, req.salida ? "Entrega (Bodega)" : "Entrega (Piso de venta)",
          8.5, false, 50, Qt::AlignHCenter);
    texto(p, (mitad + 16 + kDerecha) / 2, y + 13, req.salida ? "Recibe (Piso de venta)" : "Recibe (Bodega)",
          8.5, false, 50, Qt::AlignHCenter);

    p.end();
    return out;
}

} // namespace Documentos
