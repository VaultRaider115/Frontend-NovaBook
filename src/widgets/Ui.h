#pragma once

#include <QDateEdit>
#include <QDialog>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>

// Kit de componentes del diseño NovaBook (Figma). Los estilos viven en
// Theme::globalStyleSheet().
namespace Ui {

QString money(double value);
QString initials(const QString &fullName);
QString phone(qint64 number); // 10 dígitos agrupados: 844 111 2233
void repolish(QWidget *w);
void spaced(QWidget *w, qreal px); // espaciado entre letras (QSS no lo soporta)

QLabel *label(const QString &text, const char *objectName,
              QWidget *parent = nullptr);
QWidget *pageHeader(const QString &eyebrow, const QString &title,
                    const QString &subtitle, QWidget *parent = nullptr);
QFrame *card(QWidget *parent = nullptr);
QFrame *divider(QWidget *parent = nullptr, bool vertical = false);

// variant: "", primary, tint, outline, ghost, danger, approve, reject, small,
// page, pay
QPushButton *button(const QString &text, const char *variant = "",
                    QWidget *parent = nullptr, const QString &icon = QString());
QPushButton *iconButton(const QString &icon, const QString &tooltip,
                        QWidget *parent = nullptr);
void setCurrent(QPushButton *b, bool current);

// kind: libro, revista, ok, danger, gold, indigo, amber, mint, neutral, pending
QLabel *chip(const QString &text, const char *kind, QWidget *parent = nullptr);
QLabel *tipoChip(const QString &tipo, QWidget *parent = nullptr);
QLabel *avatar(const QString &fullName, const char *tone = "gray",
               QWidget *parent = nullptr);

QLineEdit *searchBox(const QString &placeholder, QWidget *parent = nullptr);
// Campo de EAN-13: solo dígitos y máximo 13 (también al pegar o escanear).
void eanInput(QLineEdit *e);
// Búsqueda de productos (EAN o título): título de hasta 30 caracteres; si
// empieza con un prefijo de EAN (977/978/979) o son solo dígitos, máximo 13
// dígitos. El tope cambia al escribir (el contador lo sigue).
void busquedaProducto(QLineEdit *e);
// Contador "12/60" para un campo de texto: fija el máximo (bloquea al llegar)
// y cambia de color al acercarse (ámbar, 90 %) y al llegar (rojo).
QLabel *contadorCaracteres(QLineEdit *edit, int max, QWidget *parent = nullptr);
// Igual para un texto de varias líneas: recorta lo que pase del máximo (al
// escribir o pegar) porque QPlainTextEdit no tiene maxLength.
QLabel *contadorCaracteres(QPlainTextEdit *edit, int max, QWidget *parent = nullptr);
// El campo con su contador debajo, a la derecha (para formularios y diálogos).
QWidget *conLimite(QLineEdit *edit, int max, QWidget *parent = nullptr);
QWidget *conLimite(QPlainTextEdit *edit, int max, QWidget *parent = nullptr);
// Diálogo de una nota de varias líneas con tope y contador (rechazos).
// Devuelve false si se canceló; `obligatoria` no deja aceptar vacía.
bool pedirTexto(QWidget *parent, const QString &titulo, const QString &etiqueta,
                const QString &placeholder, int max, bool obligatoria, QString &out);
void decorateDate(QDateEdit *d); // ícono de calendario a la izquierda
QFrame *stepper(QSpinBox *spin, QWidget *parent = nullptr);
QWidget *field(const QString &label, QWidget *input,
               QWidget *parent = nullptr); // etiqueta arriba
QWidget *rowField(const QString &label, QWidget *input,
                  QWidget *parent = nullptr); // etiqueta a la izquierda

QTableWidget *table(const QStringList &headers, int stretchColumn,
                    QWidget *parent = nullptr, int rowHeight = 60);
QTableWidgetItem *cellText(QTableWidget *t, int row, int col,
                           const QString &text, bool mono = false,
                           bool bold = false, const QString &color = QString());
void cellWidget(QTableWidget *t, int row, int col,
                QWidget *w); // alineado a la izquierda con margen
void cellWidgets(QTableWidget *t, int row, int col, const QList<QWidget *> &ws);
// Chips en una sola fila con scroll horizontal (p. ej. los géneros de un
// libro): no se corta ninguno aunque sean muchos; la columna queda de `ancho`.
QWidget *chipsConScroll(const QStringList &textos, const char *kind, int ancho, QWidget *parent = nullptr);
// Ensancha la columna si un widget de celda no cabe (ResizeToContents lo ignora).
void fitColumn(QTableWidget *t, int col, QWidget *content);

// Cantidad de piezas que no acepta 0 (hito 2): QSpinBox con mínimo 1 todavía
// deja teclear "0" mientras se edita; este rechaza la tecla. Si se borra todo,
// al salir del campo regresa al valor anterior.
class SpinSinCero : public QSpinBox {
public:
  using QSpinBox::QSpinBox;

protected:
  QValidator::State validate(QString &input, int &pos) const override {
    if (input.trimmed().startsWith(QLatin1Char('0')))
      return QValidator::Invalid;
    return QSpinBox::validate(input, pos);
  }
};

// Diálogo de formulario (tarjeta con encabezado, cuerpo en rejilla y pie
// Cancelar / Aceptar).
class FormDialog : public QDialog {
  Q_OBJECT
public:
  FormDialog(const QString &title, const QString &okText,
             QWidget *parent = nullptr, int width = 576);
  void addField(const QString &labelText, QWidget *input, int row, int col = 0,
                int colSpan = 1);
  QPushButton *okButton() const { return m_ok; }

private:
  QGridLayout *m_grid;
  QPushButton *m_ok;
};

} // namespace Ui
