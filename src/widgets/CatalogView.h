#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHBoxLayout>
#include <functional>

// Barra "Mostrando X de N · Anterior 1 2 Siguiente" de los catálogos y la bitácora.
class PagerBar : public QWidget {
    Q_OBJECT
public:
    explicit PagerBar(int pageSize, QWidget *parent = nullptr);
    int page() const { return m_page; }
    int pageSize() const { return m_pageSize; }
    // total = filas que cumplen el filtro; noun = "ubicaciones", "eventos"...
    void update(int total, const QString &noun, int shownInPage);
    void reset() { m_page = 0; }

signals:
    void pageChanged();

private:
    void goTo(int page);
    int m_pageSize;
    int m_page = 0;
    int m_pages = 1;
    QLabel *m_info;
    QHBoxLayout *m_btns;
};

// Plantilla de pantalla de catálogo: encabezado, barra de herramientas (buscador + total + botón nuevo),
// tabla y paginador opcional. Las subclases cargan datos y pintan las filas.
class CatalogView : public QWidget {
    Q_OBJECT
public:
    CatalogView(const QString &eyebrow, const QString &title, const QString &subtitle,
                const QString &searchPlaceholder, const QString &newText,
                const QStringList &headers, int stretchColumn, int pageSize = 0, QWidget *parent = nullptr);

protected:
    virtual void onNew() = 0;

    void clearRows();
    int appendRow(const QString &searchKey);          // devuelve el índice de la fila nueva
    void finishRows(const QString &totalText, const QString &noun = QString());
    QList<QWidget *> rowActions(const QString &what, std::function<void()> edit, std::function<void()> del,
                                const QString &actionIcon = "trash", const QString &actionName = "Eliminar");
    QWidget *nameCell(const QString &name, const QString &sub = QString());
    bool confirmDelete(const QString &what);

    void refilter() { applyVisibility(); }

    QTableWidget *m_table;
    QHBoxLayout *m_toolsLayout;
    std::function<bool(int row)> m_rowFilter;   // filtro extra opcional (p. ej. por estado)
    QLineEdit *m_search;
    QPushButton *m_btnNew;
    QLabel *m_lblTotal;

private:
    void applyVisibility();
    QStringList m_keys;
    PagerBar *m_pager = nullptr;
    QString m_noun;
};
