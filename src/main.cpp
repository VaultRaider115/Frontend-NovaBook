#include <QApplication>
#include <QPalette>
#include <QStyleFactory>
#include "Theme.h"
#include "LoginWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Application metadata
    QApplication::setApplicationName("NovaBook");
    QApplication::setOrganizationName("Sanborns");
    QApplication::setApplicationVersion("1.0.0");

    // Fusion + paleta oscura: los widgets sin estilo propio (diálogos, calendario, etc.) heredan el tema.
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette pal;
    pal.setColor(QPalette::Window, QColor(Theme::BgDark));
    pal.setColor(QPalette::WindowText, QColor(Theme::TextPrimary));
    pal.setColor(QPalette::Base, QColor(Theme::BgInput));
    pal.setColor(QPalette::AlternateBase, QColor(Theme::BgCard));
    pal.setColor(QPalette::Text, QColor(Theme::TextPrimary));
    pal.setColor(QPalette::Button, QColor(Theme::BgCard));
    pal.setColor(QPalette::ButtonText, QColor(Theme::TextPrimary));
    pal.setColor(QPalette::PlaceholderText, QColor(Theme::TextFaint));
    pal.setColor(QPalette::Highlight, QColor(Theme::Accent));
    pal.setColor(QPalette::HighlightedText, QColor(Theme::BgDark));
    app.setPalette(pal);

    // Hoja de estilos global del diseño NovaBook
    app.setStyleSheet(Theme::globalStyleSheet());

    // Launch Login Window
    LoginWindow loginWindow;
    loginWindow.show();

    return app.exec();
}
