#include "Ui.h"
#include "Theme.h"
#include <QAction>
#include <QIcon>
#include <QLocale>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QStyle>
#include <QVBoxLayout>

namespace Ui {

QString money(double value) {
    return "$" + QLocale(QLocale::English).toString(value, 'f', 2);
}

QString initials(const QString &fullName) {
    const QStringList parts = fullName.simplified().split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return "?";
    QString r = parts.first().left(1);
    if (parts.size() > 1) r += parts.last().left(1);
    return r.toUpper();
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

QWidget *pageHeader(const QString &eyebrow, const QString &title, const QString &subtitle, QWidget *parent) {
    auto *w = new QWidget(parent);
    auto *lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(6);
    if (!eyebrow.isEmpty()) lay->addWidget(label(eyebrow.toUpper(), "Eyebrow", w));
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
    l->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
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

QLineEdit *searchBox(const QString &placeholder, QWidget *parent) {
    auto *e = new QLineEdit(parent);
    e->setPlaceholderText(placeholder);
    e->addAction(QIcon(":/icons/search.svg"), QLineEdit::LeadingPosition);
    e->setClearButtonEnabled(true);
    return e;
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

QWidget *field(const QString &labelText, QWidget *input, QWidget *parent) {
    auto *w = new QWidget(parent);
    auto *lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(6);
    lay->addWidget(label(labelText, "FieldLabel", w));
    lay->addWidget(input);
    return w;
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
    lay->setContentsMargins(16, 0, 16, 0);
    lay->setSpacing(6);
    for (auto *w : ws) lay->addWidget(w);
    lay->addStretch();
    t->setCellWidget(row, col, host);
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
    m_ok = button(okText, "primary", foot);
    m_ok->setDefault(true);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    fl->addWidget(cancel);
    fl->addWidget(m_ok);
    outer->addWidget(foot);
}

void FormDialog::addField(const QString &labelText, QWidget *input, int row, int col, int colSpan) {
    m_grid->addWidget(field(labelText, input, this), row, col, 1, colSpan);
}

} // namespace Ui
