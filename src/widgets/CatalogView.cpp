#include "CatalogView.h"
#include "Ui.h"
#include <QMessageBox>
#include <QRegularExpression>
#include <QVBoxLayout>

PagerBar::PagerBar(int pageSize, QWidget *parent) : QWidget(parent), m_pageSize(pageSize) {
    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(20, 12, 20, 12);
    m_info = Ui::label(QString(), "Muted", this);
    lay->addWidget(m_info);
    lay->addStretch();
    m_btns = new QHBoxLayout();
    m_btns->setSpacing(6);
    lay->addLayout(m_btns);
}

void PagerBar::goTo(int page) {
    if (page < 0 || page >= m_pages || page == m_page) return;
    m_page = page;
    emit pageChanged();
}

void PagerBar::update(int total, const QString &noun, int shownInPage) {
    m_pages = qMax(1, (total + m_pageSize - 1) / m_pageSize);
    if (m_page >= m_pages) m_page = m_pages - 1;
    m_info->setText(QString("Mostrando %1 de %2 %3").arg(shownInPage).arg(total).arg(noun));

    while (auto *item = m_btns->takeAt(0)) {
        item->widget()->hide();
        item->widget()->deleteLater();
        delete item;
    }
    auto *prev = Ui::button("Anterior", "page", this);
    prev->setEnabled(m_page > 0);
    connect(prev, &QPushButton::clicked, this, [this]() { goTo(m_page - 1); });
    m_btns->addWidget(prev);
    for (int p = 0; p < m_pages; ++p) {
        auto *b = Ui::button(QString::number(p + 1), "page", this);
        Ui::setCurrent(b, p == m_page);
        connect(b, &QPushButton::clicked, this, [this, p]() { goTo(p); });
        m_btns->addWidget(b);
    }
    auto *next = Ui::button("Siguiente", "page", this);
    next->setEnabled(m_page < m_pages - 1);
    connect(next, &QPushButton::clicked, this, [this]() { goTo(m_page + 1); });
    m_btns->addWidget(next);
}

CatalogView::CatalogView(const QString &eyebrow, const QString &title, const QString &subtitle,
                         const QString &searchPlaceholder, const QString &newText,
                         const QStringList &headers, int stretchColumn, int pageSize, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 32, 40, 32);
    layout->setSpacing(24);
    layout->addWidget(Ui::pageHeader(eyebrow, title, subtitle, this));

    auto *tools = Ui::card(this);
    auto *tl = new QHBoxLayout(tools);
    m_toolsLayout = tl;
    tl->setContentsMargins(12, 12, 12, 12);
    tl->setSpacing(12);
    m_search = Ui::searchBox(searchPlaceholder, tools);
    tl->addWidget(m_search, 1);
    m_lblTotal = Ui::label(QString(), "Muted", tools);
    tl->addWidget(m_lblTotal);
    m_btnNew = Ui::button(newText, "primary", tools, "plus-dark");
    tl->addWidget(m_btnNew);
    layout->addWidget(tools);

    m_table = Ui::table(headers, stretchColumn, this);
    layout->addWidget(m_table, 1);

    if (pageSize > 0) {
        m_pager = new PagerBar(pageSize, this);
        layout->addWidget(m_pager);
        connect(m_pager, &PagerBar::pageChanged, this, &CatalogView::applyVisibility);
    }

    connect(m_search, &QLineEdit::textChanged, this, [this]() {
        if (m_pager) m_pager->reset();
        applyVisibility();
    });
    connect(m_btnNew, &QPushButton::clicked, this, [this]() { onNew(); });
}

void CatalogView::clearRows() {
    m_table->clearContents();
    m_table->setRowCount(0);
    m_keys.clear();
}

int CatalogView::appendRow(const QString &searchKey) {
    const int row = m_table->rowCount();
    m_table->insertRow(row);
    m_keys << searchKey.toLower();
    return row;
}

void CatalogView::finishRows(const QString &totalText, const QString &noun) {
    // Diseños 1/12/18: el número del total va resaltado ("Total de autores: 5").
    QString html = totalText.toHtmlEscaped();
    html.replace(QRegularExpression("(\\d+)"), "<b style=\"color:#FFFFFF;\">\\1</b>");
    m_lblTotal->setText(html);
    m_noun = noun;
    applyVisibility();
}

void CatalogView::applyVisibility() {
    const QString q = m_search->text().trimmed().toLower();
    QList<int> matches;
    for (int i = 0; i < m_keys.size(); ++i)
        if ((q.isEmpty() || m_keys.at(i).contains(q)) && (!m_rowFilter || m_rowFilter(i))) matches << i;

    int from = 0, to = matches.size();
    if (m_pager) {
        m_pager->update(matches.size(), m_noun, 0);
        from = m_pager->page() * m_pager->pageSize();
        to = qMin(matches.size(), from + m_pager->pageSize());
        m_pager->update(matches.size(), m_noun, to - from);
    }
    for (int i = 0; i < m_table->rowCount(); ++i) {
        const int pos = matches.indexOf(i);
        m_table->setRowHidden(i, pos < from || pos >= to || pos < 0);
    }
}

QList<QWidget *> CatalogView::rowActions(const QString &what, std::function<void()> edit, std::function<void()> del,
                                         const QString &actionIcon, const QString &actionName) {
    auto *e = Ui::iconButton("edit", "Editar " + what);
    auto *d = Ui::iconButton(actionIcon, actionName + " " + what);
    connect(e, &QPushButton::clicked, this, [edit]() { edit(); });
    connect(d, &QPushButton::clicked, this, [del]() { del(); });
    return {e, d};
}

QWidget *CatalogView::nameCell(const QString &name, const QString &sub) {
    auto *w = new QWidget();
    auto *l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);   // el padding del ítem ya da el margen
    l->setSpacing(12);
    l->addWidget(Ui::avatar(name, "gold", w));
    l->addWidget(new QLabel(name, w));
    if (!sub.isEmpty()) l->addWidget(Ui::label(sub, "Muted", w));
    l->addStretch();
    return w;
}

bool CatalogView::confirmDelete(const QString &what) {
    return QMessageBox::question(this, "Confirmar eliminación",
                                 QString("¿Seguro que deseas eliminar %1?").arg(what),
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}
