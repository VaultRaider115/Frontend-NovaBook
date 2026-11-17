#include "Ui.h"
#include "Theme.h"
#include <QAction>
#include <QIcon>
#include <QLocale>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QStyle>
#include <QVBoxLayout>
#include <QRegularExpressionValidator>

namespace Ui {

QString money(double value) {
    return "$" + QLocale(QLocale::English).toString(value, 'f', 2);
}

QString phone(qint64 number) {
    // 8441112233 → "844 111 2233" (diseño 12)
    const QString s = QString::number(number);
    if (s.size() != 10) return s;
    return QString("%1 %2 %3").arg(s.left(3), s.mid(3, 3), s.mid(6));
}

QString initials(const QString &fullName) {
    // Como en los diseños: iniciales de las dos primeras palabras
    // ("Valeria Morales Gómez" → VM); una sola palabra → dos letras ("Fantasia" → FA).
    // Los conectores no cuentan: "Defecto de fábrica" → DF, "Logística y Envío" → LE.
    static const QStringList conectores = {"de", "del", "la", "las", "los", "el", "y", "e", "o", "u", "a", "en"};
    QStringList parts;
    for (const QString &p : fullName.simplified().split(' ', Qt::SkipEmptyParts))
        if (!conectores.contains(p.toLower())) parts << p;
    if (parts.isEmpty()) return "?";
    if (parts.size() == 1) return parts.first().left(2).toUpper();
    return (parts.at(0).left(1) + parts.at(1).left(1)).toUpper();
}

void repolish(QWidget *w) {
    w->style()->unpolish(w);
    w->style()->polish(w);
}

QLabel *label(const QString &text, const char *objectName, QWidget *parent) {
    auto *l = new QLabel(text, parent);
    l->setObjectName(objectName);
    return l;
}

void spaced(QWidget *w, qreal px) {
    // QSS no soporta letter-spacing; se aplica sobre la fuente.
    QFont f = w->font();
    f.setLetterSpacing(QFont::AbsoluteSpacing, px);
    w->setFont(f);
}

QWidget *pageHeader(const QString &eyebrow, const QString &title, const QString &subtitle, QWidget *parent) {
    auto *w = new QWidget(parent);
    auto *lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(6);
    if (!eyebrow.isEmpty()) {
        auto *eb = label(eyebrow.toUpper(), "Eyebrow", w);
        spaced(eb, 1.5);
        lay->addWidget(eb);
    }
    lay->addWidget(label(title, "PageTitle", w));
    if (!subtitle.isEmpty()) {
        auto *s = label(subtitle, "PageSubtitle", w);
        s->setWordWrap(true);
        lay->addWidget(s);
    }
    return w;
}

QFrame *card(QWidget *parent) {
    auto *f = new QFrame(parent);
    f->setObjectName("Card");
    return f;
}

QFrame *divider(QWidget *parent, bool vertical) {
    auto *f = new QFrame(parent);
    f->setObjectName(vertical ? "VDivider" : "Divider");
    return f;
}

QPushButton *button(const QString &text, const char *variant, QWidget *parent, const QString &icon) {
    auto *b = new QPushButton(text, parent);
    if (variant && *variant) b->setProperty("variant", variant);
    if (!icon.isEmpty()) {
        b->setIcon(QIcon(":/icons/" + icon + ".svg"));
        b->setIconSize(QSize(16, 16));
    }
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

QPushButton *iconButton(const QString &icon, const QString &tooltip, QWidget *parent) {
    auto *b = new QPushButton(parent);
    b->setProperty("variant", "icon");
    b->setIcon(QIcon(":/icons/" + icon + ".svg"));
    b->setIconSize(QSize(16, 16));
    b->setToolTip(tooltip);
    b->setAccessibleName(tooltip);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

void setCurrent(QPushButton *b, bool current) {
    b->setProperty("current", current);
    repolish(b);
}

QLabel *chip(const QString &text, const char *kind, QWidget *parent) {
    auto *l = new QLabel(text, parent);
    l->setProperty("chip", kind);
    l->setAlignment(Qt::AlignCenter);
    l->ensurePolished();            // aplica el stylesheet (fuente y padding del chip)
    // Ancho fijo = texto + padding horizontal (10px por lado) + margen de redondeo.
    // Un mínimo mayor que el sizeHint hacía que el chip se pintara recortado.
    const int w = l->fontMetrics().horizontalAdvance(text) + 2 * 10 + 8;
    l->setFixedSize(w, 22);
    return l;
}

QLabel *tipoChip(const QString &tipo, QWidget *parent) {
    const bool libro = tipo.compare("libro", Qt::CaseInsensitive) == 0;
    return chip(libro ? "LIBRO" : "REVISTA", libro ? "libro" : "revista", parent);
}

QLabel *avatar(const QString &fullName, const char *tone, QWidget *parent) {
    auto *l = new QLabel(initials(fullName), parent);
    l->setProperty("avatar", tone);
    l->setFixedSize(36, 36);
    return l;
}

void decorateDate(QDateEdit *d) {
    // Ícono de calendario a la izquierda (diseños 7 y 20). QDateEdit no expone
    // su QLineEdit, pero es hijo directo y acepta acciones.
    if (auto *le = d->findChild<QLineEdit *>())
        le->addAction(QIcon(":/icons/calendar-gold.svg"), QLineEdit::LeadingPosition);
}

QLineEdit *searchBox(const QString &placeholder, QWidget *parent) {
    auto *e = new QLineEdit(parent);
    e->setPlaceholderText(placeholder);
    e->addAction(QIcon(":/icons/search.svg"), QLineEdit::LeadingPosition);
    e->setClearButtonEnabled(true);
    return e;
}

void eanInput(QLineEdit *e) {
    e->setMaxLength(13);
    e->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{0,13}"), e));
}

QFrame *stepper(QSpinBox *spin, QWidget *parent) {
    auto *f = new QFrame(parent);
    f->setObjectName("Stepper");
    auto *lay = new QHBoxLayout(f);
    lay->setContentsMargins(4, 0, 4, 0);
    lay->setSpacing(0);

    spin->setParent(f);
    spin->setProperty("bare", true);
    spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    spin->setAlignment(Qt::AlignCenter);

    auto *minus = iconButton("minus-muted", "Disminuir", f);
    auto *plus = iconButton("plus-muted", "Aumentar", f);
    QObject::connect(minus, &QPushButton::clicked, spin, [spin]() { spin->stepDown(); });
    QObject::connect(plus, &QPushButton::clicked, spin, [spin]() { spin->stepUp(); });

    lay->addWidget(minus);
    lay->addWidget(spin, 1);
    lay->addWidget(plus);
    f->setMinimumWidth(150);
    f->setFixedHeight(44);
    return f;
}

static QWidget *fieldImpl(const QString &labelText, QWidget *input, QWidget *parent, bool upper) {
    auto *w = new QWidget(parent);
    auto *lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(6);
    auto *l = label(upper ? labelText.toUpper() : labelText, "FieldLabel", w);
    if (upper) spaced(l, 0.8);
    lay->addWidget(l);
    lay->addWidget(input);
    return w;
}

// En las pantallas los diseños usan etiquetas en mayúsculas espaciadas
// ("CÓDIGO EAN-13", "CANTIDAD"); en los diálogos van en tipo oración.
QWidget *field(const QString &labelText, QWidget *input, QWidget *parent) {
    return fieldImpl(labelText, input, parent, true);
}

QWidget *rowField(const QString &labelText, QWidget *input, QWidget *parent) {
    auto *w = new QWidget(parent);
    auto *lay = new QHBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(16);
    auto *l = label(labelText, "RowLabel", w);
    l->setFixedWidth(188);
    l->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    lay->addWidget(l, 0, Qt::AlignTop);
    lay->addWidget(input, 1);
    return w;
}

QTableWidget *table(const QStringList &headers, int stretchColumn, QWidget *parent, int rowHeight) {
    auto *t = new QTableWidget(parent);
    t->setColumnCount(headers.size());
    QStringList up;
    for (const QString &h : headers) up << h.toUpper();
    t->setHorizontalHeaderLabels(up);
    auto *hh = t->horizontalHeader();
    hh->setHighlightSections(false);
    hh->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    for (int i = 0; i < headers.size(); ++i)
        hh->setSectionResizeMode(i, i == stretchColumn ? QHeaderView::Stretch : QHeaderView::ResizeToContents);
    t->verticalHeader()->setVisible(false);
    t->verticalHeader()->setDefaultSectionSize(rowHeight);
    t->setShowGrid(false);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setFocusPolicy(Qt::NoFocus);
    t->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    return t;
}

QTableWidgetItem *cellText(QTableWidget *t, int row, int col, const QString &text,
                           bool mono, bool bold, const QString &color) {
    auto *it = new QTableWidgetItem(text);
    QFont f = it->font();
    if (mono) f.setFamilies({"Consolas", "Geist Mono", "DejaVu Sans Mono"});
    if (bold) f.setWeight(QFont::DemiBold);
    it->setFont(f);
    it->setForeground(QColor(color.isEmpty() ? (mono ? Theme::TextMuted : Theme::TextPrimary) : color));
    t->setItem(row, col, it);
    return it;
}

void cellWidgets(QTableWidget *t, int row, int col, const QList<QWidget *> &ws) {
    auto *host = new QWidget();
    auto *lay = new QHBoxLayout(host);
    lay->setContentsMargins(0, 0, 0, 0);   // el padding del ítem ya da el margen
    lay->setSpacing(6);
    for (auto *w : ws) lay->addWidget(w);
    lay->addStretch();
    t->setCellWidget(row, col, host);
    fitColumn(t, col, host);
}

void fitColumn(QTableWidget *t, int col, QWidget *content) {
    // ResizeToContents solo mide el texto de las celdas, no los widgets (chips,
    // botones); sin esto el contenido queda recortado por la derecha.
    auto *hh = t->horizontalHeader();
    if (hh->sectionResizeMode(col) == QHeaderView::Stretch) return;
    // El delegado coloca los widgets de celda dentro del padding de
    // QTableWidget::item (ver Theme.cpp), así que se suma.
    constexpr int kItemPadX = 12;
    const int need = content->sizeHint().width() + 2 * kItemPadX;
    if (t->columnWidth(col) < need) {
        hh->setSectionResizeMode(col, QHeaderView::Interactive);
        t->setColumnWidth(col, need);
    }
}

void cellWidget(QTableWidget *t, int row, int col, QWidget *w) {
    cellWidgets(t, row, col, {w});
}

FormDialog::FormDialog(const QString &title, const QString &okText, QWidget *parent, int width)
    : QDialog(parent)
{
    setWindowTitle(title);
    setModal(true);
    setFixedWidth(width);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *head = new QWidget(this);
    auto *hl = new QHBoxLayout(head);
    hl->setContentsMargins(24, 16, 16, 16);
    hl->addWidget(label(title, "DialogTitle", head));
    hl->addStretch();
    auto *close = iconButton("x", "Cerrar", head);
    connect(close, &QPushButton::clicked, this, &QDialog::reject);
    hl->addWidget(close);
    outer->addWidget(head);
    outer->addWidget(divider(this));

    auto *body = new QWidget(this);
    m_grid = new QGridLayout(body);
    m_grid->setContentsMargins(24, 20, 24, 8);
    m_grid->setHorizontalSpacing(16);
    m_grid->setVerticalSpacing(16);
    outer->addWidget(body);

    auto *foot = new QWidget(this);
    auto *fl = new QHBoxLayout(foot);
    fl->setContentsMargins(24, 8, 24, 24);
    fl->setSpacing(12);
    fl->addStretch();
    auto *cancel = button("Cancelar", "", foot);
    m_ok = button(okText, "primary", foot, "check-dark");   // "✓ Guardar …" como en los diseños
    m_ok->setDefault(true);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    fl->addWidget(cancel);
    fl->addWidget(m_ok);
    outer->addWidget(foot);
}

void FormDialog::addField(const QString &labelText, QWidget *input, int row, int col, int colSpan) {
    m_grid->addWidget(fieldImpl(labelText, input, this, false), row, col, 1, colSpan);
}

} // namespace Ui
