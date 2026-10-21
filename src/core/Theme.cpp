#include "Theme.h"
#include <QHash>

namespace Theme {

// Sustituye {{Token}} por su valor para mantener el QSS legible.
static QString fill(QString qss) {
    const QHash<QString, QString> t = {
        {"BgDark", BgDark}, {"BgCard", BgCard}, {"BgInput", BgInput}, {"BgSidebar", BgSidebar},
        {"BgRowAlt", BgRowAlt}, {"BgHeader", BgHeader}, {"BgLabel", BgLabel},
        {"Accent", Accent}, {"AccentHover", AccentHover}, {"AccentDim", AccentDim},
        {"TextPrimary", TextPrimary}, {"TextMuted", TextMuted}, {"TextFaint", TextFaint},
        {"Border", Border}, {"Error", Error}, {"ErrorBg", ErrorBg}, {"Success", Success},
        {"Libro", Libro}, {"Revista", Revista}, {"FontUi", FontUi}, {"FontMono", FontMono},
    };
    for (auto it = t.cbegin(); it != t.cend(); ++it)
        qss.replace("{{" + it.key() + "}}", it.value());
    return qss;
}

QString globalStyleSheet() {
    return fill(R"(
        QWidget { color: {{TextPrimary}}; font-family: {{FontUi}}; font-size: 14px; }
        QMainWindow, QDialog, QStackedWidget, QScrollArea, #Page { background-color: {{BgDark}}; }
        QLabel { background: transparent; }
        QToolTip { background-color: {{BgCard}}; color: {{TextPrimary}}; border: 1px solid {{Border}}; padding: 4px 8px; }

        /* ---------- Inputs ---------- */
        QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QDateEdit, QComboBox {
            background-color: {{BgInput}}; color: {{TextPrimary}};
            border: 1px solid {{Border}}; border-radius: 8px;
            padding: 10px 14px; min-height: 20px;
            selection-background-color: {{Accent}}; selection-color: {{BgDark}};
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QSpinBox:focus,
        QDoubleSpinBox:focus, QDateEdit:focus, QComboBox:focus { border: 1px solid {{Accent}}; }
        QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled {
            background-color: #111113; color: {{TextFaint}};
        }
        QLineEdit[tone="gold"] { border: 1px solid rgba(212,175,55,70%); }
        QSpinBox[bare="true"], QSpinBox[bare="true"]:focus { border: none; background: transparent; padding: 0; }
        QComboBox::drop-down { border: none; width: 30px; }
        QComboBox::down-arrow { image: url(:/icons/chevron-down.svg); width: 14px; height: 14px; }
        QComboBox QAbstractItemView {
            background-color: {{BgCard}}; color: {{TextPrimary}}; border: 1px solid {{Border}};
            selection-background-color: rgba(212,163,70,18%); selection-color: {{TextPrimary}};
            outline: none; padding: 4px;
        }

        /* ---------- Botones ---------- */
        QPushButton, QToolButton {
            background-color: {{BgCard}}; color: {{TextPrimary}};
            border: 1px solid {{Border}}; border-radius: 8px;
            padding: 10px 20px; min-height: 20px; font-weight: 500;
        }
        QPushButton:hover, QToolButton:hover { background-color: #202024; }
        QPushButton:disabled { background-color: #131315; color: {{TextFaint}}; border-color: #1e1e21; }
        QPushButton[variant="primary"] { background-color: {{Accent}}; color: {{BgDark}}; border: none; font-weight: 600; }
        QPushButton[variant="primary"]:hover { background-color: {{AccentHover}}; }
        QPushButton[variant="primary"]:disabled { background-color: {{AccentDim}}; color: {{BgDark}}; }
        QPushButton[variant="tint"] {
            background-color: rgba(212,163,70,10%); color: {{Accent}};
            border: 1px solid rgba(212,163,70,40%); font-weight: 600;
        }
        QPushButton[variant="tint"]:hover { background-color: rgba(212,163,70,20%); }
        QPushButton[variant="outline"] { background-color: {{BgDark}}; color: {{TextMuted}}; font-weight: 600; }
        QPushButton[variant="outline"]:hover { color: {{TextPrimary}}; border-color: #3f3f46; }
        QPushButton[variant="ghost"] { background-color: transparent; color: {{TextMuted}}; border: none; font-weight: 600; }
        QPushButton[variant="ghost"]:hover { color: {{TextPrimary}}; background-color: rgba(255,255,255,5%); }
        QPushButton[variant="danger"] {
            background-color: rgba(255,100,103,10%); color: {{Error}};
            border: 1px solid rgba(255,100,103,30%); font-weight: 600;
        }
        QPushButton[variant="danger"]:hover { background-color: rgba(255,100,103,22%); }
        QPushButton[variant="approve"] {
            background-color: rgba(34,197,94,15%); color: #4ADE80;
            border: 1px solid rgba(34,197,94,30%); font-weight: 600; padding: 6px 12px; min-height: 16px;
        }
        QPushButton[variant="approve"]:hover { background-color: rgba(34,197,94,28%); }
        QPushButton[variant="reject"] {
            background-color: rgba(239,68,68,15%); color: #F87171;
            border: 1px solid rgba(239,68,68,30%); font-weight: 600; padding: 6px 12px; min-height: 16px;
        }
        QPushButton[variant="reject"]:hover { background-color: rgba(239,68,68,28%); }
        QPushButton[variant="small"] { padding: 6px 12px; min-height: 16px; font-size: 13px; }
        QPushButton[variant="icon"], QToolButton[variant="icon"] {
            background-color: transparent; border: 1px solid transparent; border-radius: 8px;
            padding: 0; min-height: 32px; min-width: 32px; max-width: 32px; max-height: 32px;
        }
        QPushButton[variant="icon"]:hover, QToolButton[variant="icon"]:hover {
            background-color: rgba(255,255,255,6%); border-color: {{Border}};
        }
        QPushButton[variant="page"] { padding: 4px 10px; min-height: 22px; font-size: 13px; }
        QPushButton[variant="page"][current="true"] {
            background-color: rgba(212,163,70,10%); color: {{Accent}}; border: 1px solid rgba(212,163,70,60%); font-weight: 600;
        }
        QPushButton[variant="pay"] { background-color: #111113; color: {{TextMuted}}; font-weight: 600; padding: 10px 20px; }
        QPushButton[variant="pay"][current="true"] {
            background-color: rgba(212,163,70,15%); color: #E0B865; border: 1px solid {{Accent}};
        }

        /* ---------- Tarjetas y tipografía de página ---------- */
        QFrame#Card { background-color: {{BgCard}}; border: 1px solid {{Border}}; border-radius: 12px; }
        QFrame#DialogCard { background-color: {{BgCard}}; border: 1px solid {{Border}}; border-radius: 12px; }
        QFrame#Divider { background-color: {{Border}}; border: none; max-height: 1px; min-height: 1px; }
        QFrame#VDivider { background-color: {{Border}}; border: none; max-width: 1px; min-width: 1px; }
        QFrame#Stepper { background-color: {{BgInput}}; border: 1px solid {{Border}}; border-radius: 8px; }
        QFrame#Highlight {
            background-color: rgba(212,163,70,7%); border: 1px solid rgba(212,163,70,40%); border-radius: 10px;
        }
        QLabel#Eyebrow { color: {{Accent}}; font-size: 12px; font-weight: 600; }
        QLabel#PageTitle { color: {{TextPrimary}}; font-size: 30px; font-weight: 600; }
        QLabel#PageSubtitle { color: {{TextMuted}}; font-size: 14px; }
        QLabel#DialogTitle { color: {{TextPrimary}}; font-size: 16px; font-weight: 600; }
        QLabel#FieldLabel { color: {{TextMuted}}; font-size: 12px; font-weight: 600; }
        QLabel#RowLabel {
            background-color: {{BgLabel}}; border: 1px solid {{Border}}; border-radius: 8px;
            color: {{TextMuted}}; font-size: 13px; padding: 12px 14px;
        }
        QLabel#Hint {
            background-color: #111113; border: 1px solid {{Border}}; border-radius: 8px;
            color: {{TextMuted}}; font-size: 13px; padding: 8px 14px;
        }
        QLabel#Muted { color: {{TextMuted}}; }
        QLabel#BigNumber { color: {{TextPrimary}}; font-size: 24px; font-weight: 600; }
        QLabel#Gold { color: {{Accent}}; }
        QLabel#Mono { color: {{TextMuted}}; font-family: {{FontMono}}; font-size: 13px; }

        /* Chips (tipo de producto, estado, rol) */
        QLabel[chip="libro"]   { border-radius: 6px; padding: 3px 10px; font-size: 11px; font-weight: 600; background-color: rgba(0,188,125,15%);  color: #00D492; }
        QLabel[chip="revista"] { border-radius: 6px; padding: 3px 10px; font-size: 11px; font-weight: 600; background-color: rgba(0,166,244,15%);  color: #00BCFF; }
        QLabel[chip="ok"]      { border-radius: 6px; padding: 3px 10px; font-size: 11px; font-weight: 600; background-color: rgba(0,188,125,10%);  color: #00D492; }
        QLabel[chip="danger"]  { border-radius: 6px; padding: 3px 10px; font-size: 11px; font-weight: 600; background-color: rgba(251,44,54,10%);  color: #FF6467; }
        QLabel[chip="gold"]    { border-radius: 6px; padding: 3px 10px; font-size: 11px; font-weight: 600; background-color: rgba(212,163,70,10%); color: #D4A346; }
        QLabel[chip="indigo"]  { border-radius: 6px; padding: 3px 10px; font-size: 11px; font-weight: 600; background-color: rgba(97,95,255,10%);  color: #A3B3FF; }
        QLabel[chip="amber"]   { border-radius: 6px; padding: 3px 10px; font-size: 11px; font-weight: 600; background-color: rgba(254,154,0,10%);  color: #FFD230; }
        QLabel[chip="mint"]    { border-radius: 6px; padding: 3px 10px; font-size: 11px; font-weight: 600; background-color: rgba(0,188,125,10%);  color: #5EE9B5; }
        QLabel[chip="neutral"] { border-radius: 6px; padding: 3px 10px; font-size: 11px; background-color: rgba(255,255,255,5%); color: {{TextMuted}}; }
        QLabel[chip="pending"] { border-radius: 6px; padding: 3px 10px; font-size: 11px; font-weight: 600; background-color: rgba(254,154,0,10%);  color: #FFD230; }
        QLabel[avatar="gold"], QLabel[avatar="gray"] { border-radius: 8px; font-size: 12px; font-weight: 600; min-width: 36px; max-width: 36px; min-height: 36px; max-height: 36px; qproperty-alignment: AlignCenter; }
        QLabel[avatar="gold"] { background-color: rgba(212,163,70,10%); color: {{Accent}}; }
        QLabel[avatar="gray"] { background-color: {{Border}}; color: {{TextPrimary}}; }

        /* ---------- Tablas ---------- */
        QTableWidget, QTableView, QListWidget, QListView, QTreeWidget, QTreeView {
            background-color: {{BgCard}}; alternate-background-color: {{BgCard}}; color: {{TextPrimary}};
            border: 1px solid {{Border}}; border-radius: 12px; gridline-color: transparent;
            selection-background-color: rgba(212,163,70,8%); selection-color: {{TextPrimary}}; outline: none;
        }
        QTableWidget::item, QTableView::item { padding: 8px 16px; border-bottom: 1px solid rgba(39,39,42,70%); }
        QTableWidget::item:selected, QTableView::item:selected { background-color: rgba(212,163,70,8%); color: {{TextPrimary}}; }
        QHeaderView { background-color: {{BgHeader}}; }
        QHeaderView::section {
            background-color: {{BgHeader}}; color: {{TextMuted}}; font-weight: 600; font-size: 11px;
            padding: 14px 16px; border: none; border-bottom: 1px solid {{Border}};
        }
        QTableCornerButton::section { background-color: {{BgHeader}}; border: none; }
        QListWidget::indicator, QCheckBox::indicator {
            width: 16px; height: 16px; border: 1px solid #52525B; border-radius: 4px; background-color: {{BgCard}};
        }
        QListWidget::indicator:checked, QCheckBox::indicator:checked {
            background-color: {{Accent}}; border-color: {{Accent}}; image: url(:/icons/check.svg);
        }
        QListWidget { padding: 6px; border-radius: 8px; background-color: {{BgInput}}; }
        QListWidget::item { padding: 6px; border-radius: 6px; }
        QListWidget::item:hover { background-color: rgba(255,255,255,4%); }

        /* ---------- Scrollbars ---------- */
        QScrollBar:vertical { background: transparent; width: 10px; margin: 0; }
        QScrollBar::handle:vertical { background: {{Border}}; min-height: 24px; border-radius: 5px; }
        QScrollBar::handle:vertical:hover { background: #3F3F46; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar:horizontal { background: transparent; height: 10px; margin: 0; }
        QScrollBar::handle:horizontal { background: {{Border}}; min-width: 24px; border-radius: 5px; }
        QScrollBar::handle:horizontal:hover { background: #3F3F46; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
        QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

        /* ---------- Calendario (QDateEdit) ---------- */
        QCalendarWidget QWidget { background-color: {{BgCard}}; color: {{TextPrimary}}; }
        QCalendarWidget QAbstractItemView { selection-background-color: {{Accent}}; selection-color: {{BgDark}}; }

        QMessageBox { background-color: {{BgCard}}; }
    )");
}

QString buttonStyle(const QString &bg, const QString &fg, const QString &hover) {
    return fill(QString(R"(
        QPushButton { background-color: %1; color: %2; border: none; border-radius: 8px; padding: 10px 20px; font-weight: 600; }
        QPushButton:hover { background-color: %3; }
    )").arg(bg, fg, hover));
}

QString secondaryButtonStyle() {
    return fill(R"(
        QPushButton { background-color: {{BgCard}}; color: {{TextPrimary}}; border: 1px solid {{Border}}; border-radius: 8px; padding: 10px 20px; font-weight: 500; }
        QPushButton:hover { background-color: #202024; border-color: #3F3F46; }
    )");
}

QString dangerButtonStyle() {
    return fill(R"(
        QPushButton { background-color: rgba(255,100,103,10%); color: {{Error}}; border: 1px solid rgba(255,100,103,30%); border-radius: 8px; padding: 10px 20px; font-weight: 600; }
        QPushButton:hover { background-color: rgba(255,100,103,22%); }
    )");
}

QString roleColor(int roleId) {
    switch (roleId) {
        case 1: return Role1;
        case 4: return Role4;
        case 2: return Role2;
        case 3: return Role3;
        default: return Accent;
    }
}

QString roleName(int roleId) {
    switch (roleId) {
        case 1: return "Jefe de Departamento";
        case 4: return "Gerente General";
        case 2: return "Personal de Bodega";
        case 3: return "Vendedor";
        default: return "Usuario";
    }
}

QString roleBadgeStyle(int roleId) {
    return QString("QLabel { background-color: rgba(255,255,255,5%); color: %1; border: 1px solid %1; "
                   "border-radius: 10px; padding: 3px 10px; font-size: 11px; font-weight: 600; }")
        .arg(roleColor(roleId));
}

} // namespace Theme
