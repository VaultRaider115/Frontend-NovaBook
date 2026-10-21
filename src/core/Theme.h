#pragma once

#include <QString>
#include <QColor>

// Tokens tomados del diseño de Figma "Proyecto S" (zinc + dorado Sanborns).
namespace Theme {
    inline const QString BgDark       = "#0D0D0D"; // fondo de pantalla / inputs
    inline const QString BgCard       = "#18181B"; // tarjetas y tablas
    inline const QString BgInput      = "#0D0D0D";
    inline const QString BgSidebar    = "#111113";
    inline const QString BgRowAlt     = "#1F1F23";
    inline const QString BgHeader     = "#141416"; // cabecera de tabla
    inline const QString BgLabel      = "#1F1F23"; // etiqueta lateral de formularios
    inline const QString Accent       = "#D4A346";
    inline const QString AccentHover  = "#E0B865";
    inline const QString AccentDim    = "#8A6A24";
    inline const QString TextPrimary  = "#FAFAFA";
    inline const QString TextMuted    = "#A1A1AA";
    inline const QString TextFaint    = "#71717A"; // placeholders
    inline const QString TextSidebar  = "#A1A1AA";
    inline const QString Border       = "#27272A";
    inline const QString BorderFocus  = "#D4A346";
    inline const QString Error        = "#FF6467";
    inline const QString ErrorBg      = "#2A1214";
    inline const QString Success      = "#00D492";
    inline const QString SuccessBg    = "#0B2A20";
    inline const QString Warning      = "#FFD230";
    inline const QString Libro        = "#00D492"; // chip LIBRO
    inline const QString Revista      = "#00BCFF"; // chip REVISTA
    inline const QString Role1        = "#A3B3FF"; // Jefe de Departamento — índigo
    inline const QString Role2        = "#FFD230"; // Personal de Bodega — ámbar
    inline const QString Role3        = "#5EE9B5"; // Vendedor — menta
    inline const QString Role4        = "#D4A346"; // Gerente — dorado

    inline const QString FontUi       = "'Segoe UI', 'Geist', 'Inter', 'DejaVu Sans', sans-serif";
    inline const QString FontMono     = "'Consolas', 'Geist Mono', 'DejaVu Sans Mono', monospace";

    QString globalStyleSheet();
    QString buttonStyle(const QString &bg = Accent, const QString &fg = BgDark, const QString &hover = AccentHover);
    QString secondaryButtonStyle();
    QString dangerButtonStyle();
    QString roleBadgeStyle(int roleId);
    QString roleColor(int roleId);
    QString roleName(int roleId);
}
