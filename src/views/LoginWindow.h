#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "Models.h"

class MainWindow;

class LoginWindow : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow() override;

private slots:
    void onLoginClicked();
    void onLogoutRequested();
    void onSessionExpired(const QString &reason);

private:
    QLineEdit *m_txtUsuario;
    QLineEdit *m_txtPassword;
    QLabel *m_lblError;
    QPushButton *m_btnLogin;

    MainWindow *m_mainWindow = nullptr;
};
