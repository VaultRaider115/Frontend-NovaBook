#pragma once

#include <QDialog>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>

// Kit de componentes del diseño NovaBook (Figma). Los estilos viven en Theme::globalStyleSheet().
namespace Ui {

QString money(double value);
QString initials(const QString &fullName);
void repolish(QWidget *w);

QLabel *label(const QString &text, const char *objectName, QWidget *parent = nullptr);
QWidget *pageHeader(const QString &eyebrow, const QString &title, const QString &subtitle, QWidget *parent = nullptr);
QFrame *card(QWidget *parent = nullptr);
QFrame *divider(QWidget *parent = nullptr, bool vertical = false);

// variant: "", primary, tint, outline, ghost, danger, approve, reject, small, page, pay
QPushButton *button(const QString &text, const char *variant = "", QWidget *parent = nullptr, const QString &icon = QString());
QPushButton *iconButton(const QString &icon, const QString &tooltip, QWidget *parent = nullptr);
void setCurrent(QPushButton *b, bool current);

// kind: libro, revista, ok, danger, gold, indigo, amber, mint, neutral, pending
QLabel *chip(const QString &text, const char *kind, QWidget *parent = nullptr);
QLabel *tipoChip(const QString &tipo, QWidget *parent = nullptr);
QLabel *avatar(const QString &fullName, const char *tone = "gray", QWidget *parent = nullptr);

QLineEdit *searchBox(const QString &placeholder, QWidget *parent = nullptr);
QFrame *stepper(QSpinBox *spin, QWidget *parent = nullptr);
QWidget *field(const QString &label, QWidget *input, QWidget *parent = nullptr);   // etiqueta arriba
QWidget *rowField(const QString &label, QWidget *input, QWidget *parent = nullptr); // etiqueta a la izquierda

QTableWidget *table(const QStringList &headers, int stretchColumn, QWidget *parent = nullptr, int rowHeight = 60);
QTableWidgetItem *cellText(QTableWidget *t, int row, int col, const QString &text,
                           bool mono = false, bool bold = false, const QString &color = QString());
void cellWidget(QTableWidget *t, int row, int col, QWidget *w);              // alineado a la izquierda con margen
void cellWidgets(QTableWidget *t, int row, int col, const QList<QWidget *> &ws);

// Diálogo de formulario (tarjeta con encabezado, cuerpo en rejilla y pie Cancelar / Aceptar).
class FormDialog : public QDialog {
    Q_OBJECT
public:
    FormDialog(const QString &title, const QString &okText, QWidget *parent = nullptr, int width = 576);
    void addField(const QString &labelText, QWidget *input, int row, int col = 0, int colSpan = 1);
    QPushButton *okButton() const { return m_ok; }

private:
    QGridLayout *m_grid;
    QPushButton *m_ok;
};

} // namespace Ui
