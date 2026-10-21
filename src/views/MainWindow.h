#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QList>
#include <functional>
#include "Models.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void initUser(const UsuarioDto &user);

signals:
    void logoutRequested();

private slots:
    void onLogoutClicked();
    void onNavButtonClicked(int index);

private:
    void setupSidebar();
    void buildRoleViews();
    // Registra una página del menú; onShow se ejecuta cada vez que se abre (carga de datos).
    void addPage(const QString &navText, QWidget *view, std::function<void()> onShow = nullptr);
    void updateActiveNavButton(int activeIndex);

    UsuarioDto m_user;

    QWidget *m_sidebarWidget;
    QVBoxLayout *m_navLayout;
    QLabel *m_lblUserName;
    QLabel *m_lblRoleBadge;
    QPushButton *m_btnLogout;

    QStackedWidget *m_stack;
    QList<QPushButton *> m_navButtons;
    QList<std::function<void()>> m_onShow;
};
