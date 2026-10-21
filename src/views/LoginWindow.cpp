#include "LoginWindow.h"
#include "MainWindow.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Ui.h"
#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QIcon>
#include <QGuiApplication>
#include <QScreen>

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("LoginRoot");
    setWindowTitle("NovaBook — Iniciar Sesión");
    setFixedSize(650, 780);
    setAttribute(Qt::WA_QuitOnClose, true);
    setStyleSheet("#LoginRoot { background-color: #0A0A0A; }");

    const QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    move((screenGeometry.width() - width()) / 2, (screenGeometry.height() - height()) / 2);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Barra dorada superior
    auto *topBar = new QFrame(this);
    topBar->setFixedHeight(6);
    topBar->setStyleSheet(QString("background-color: %1;").arg(Theme::Accent));
    root->addWidget(topBar);

    auto *content = new QVBoxLayout();
    content->setContentsMargins(75, 48, 75, 32);
    content->setSpacing(22);
    root->addLayout(content, 1);

    // Encabezado
    auto *brand = new QLabel("Nova Book", this);
    brand->setAlignment(Qt::AlignCenter);
    brand->setStyleSheet("font-size: 54px; font-weight: 800; color: #FFFFFF;");
    content->addWidget(brand);

    auto *sub = new QLabel("Sistema de Gestión de Librería", this);
    sub->setAlignment(Qt::AlignCenter);
    sub->setStyleSheet("font-size: 15px; color: #9CA3AF;");
    content->addWidget(sub);

    auto *pill = new QLabel("Sanborns · Saltillo", this);
    pill->setAlignment(Qt::AlignCenter);
    pill->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    pill->setStyleSheet("background-color: #1C1C1D; border: 1px solid #3F3F46; border-radius: 16px; "
                        "color: #FFC964; font-size: 13px; padding: 6px 14px;");
    content->addWidget(pill, 0, Qt::AlignHCenter);

    // Tarjeta de sesión
    auto *card = new QFrame(this);
    card->setObjectName("LoginCard");
    card->setStyleSheet("#LoginCard { background-color: #141414; border: 1px solid #232323; border-radius: 12px; }");
    auto *cl = new QVBoxLayout(card);
    cl->setContentsMargins(32, 32, 32, 32);
    cl->setSpacing(14);

    auto *cardTitle = new QLabel("Iniciar sesión", card);
    cardTitle->setStyleSheet("font-size: 14px; font-weight: 700; color: #7A7A7A;");
    cl->addWidget(cardTitle);

    const QString inputQss = "QLineEdit { background-color: #1C1C1C; border: 1px solid #2A2A2A; border-radius: 8px; "
                             "padding: 14px 20px; font-size: 15px; } QLineEdit:focus { border: 1px solid #D4A346; }";

    m_txtUsuario = new QLineEdit(card);
    m_txtUsuario->setPlaceholderText("Número de empleado");
    m_txtUsuario->setStyleSheet(inputQss);
    m_txtUsuario->setAccessibleName("ID de empleado");
    cl->addWidget(Ui::field("ID de Empleado", m_txtUsuario, card));

    m_txtPassword = new QLineEdit(card);
    m_txtPassword->setEchoMode(QLineEdit::Password);
    m_txtPassword->setPlaceholderText("Ingresa tu contraseña");
    m_txtPassword->setStyleSheet(inputQss);
    m_txtPassword->setAccessibleName("Contraseña");
    auto *eye = m_txtPassword->addAction(QIcon(":/icons/eye.svg"), QLineEdit::TrailingPosition);
    eye->setToolTip("Mostrar contraseña");
    connect(eye, &QAction::triggered, this, [this, eye]() {
        const bool hidden = m_txtPassword->echoMode() == QLineEdit::Password;
        m_txtPassword->setEchoMode(hidden ? QLineEdit::Normal : QLineEdit::Password);
        eye->setIcon(QIcon(hidden ? ":/icons/eye-off.svg" : ":/icons/eye.svg"));
        eye->setToolTip(hidden ? "Ocultar contraseña" : "Mostrar contraseña");
    });
    cl->addWidget(Ui::field("Contraseña", m_txtPassword, card));

    m_lblError = new QLabel(card);
    m_lblError->setStyleSheet(QString("color: %1; font-size: 12px;").arg(Theme::Error));
    m_lblError->setWordWrap(true);
    m_lblError->hide();
    cl->addWidget(m_lblError);

    m_btnLogin = Ui::button("Ingresar al sistema", "primary", card);
    m_btnLogin->setStyleSheet("QPushButton { padding: 14px; font-size: 15px; font-weight: 700; }");
    cl->addWidget(m_btnLogin);
    content->addWidget(card);

    // Aviso legal
    auto *legal = new QLabel("NovaBook · Uso exclusivo para personal autorizado de Sanborns", this);
    legal->setAlignment(Qt::AlignCenter);
    legal->setWordWrap(true);
    legal->setStyleSheet("background-color: #141414; border-radius: 12px; color: #999292; "
                         "font-size: 13px; font-weight: 700; padding: 22px 32px;");
    content->addWidget(legal);
    content->addStretch();

    connect(m_btnLogin, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(m_txtUsuario, &QLineEdit::returnPressed, this, [this]() { m_txtPassword->setFocus(); });
    connect(m_txtPassword, &QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);
}

LoginWindow::~LoginWindow() {
    delete m_mainWindow;
}

void LoginWindow::onLoginClicked() {
    m_lblError->hide();
    const QString idStr = m_txtUsuario->text().trimmed();
    const QString pass = m_txtPassword->text();

    bool okNum = false;
    const qint64 id = idStr.toLongLong(&okNum);

    if (!okNum || id <= 0) {
        m_lblError->setText("El ID de empleado debe ser numérico.");
        m_lblError->show();
        return;
    }
    if (pass.isEmpty()) {
        m_lblError->setText("Ingresa tu contraseña.");
        m_lblError->show();
        return;
    }

    m_btnLogin->setEnabled(false);
    m_btnLogin->setText("Conectando...");

    ApiClient::instance()->login(id, pass, [this](bool success, const QJsonValue &, const QString &msg) {
        m_btnLogin->setEnabled(true);
        m_btnLogin->setText("Ingresar al sistema");

        if (!success) {
            m_lblError->setText(msg);
            m_lblError->show();
            return;
        }

        hide();
        if (!m_mainWindow) {
            m_mainWindow = new MainWindow();
            connect(m_mainWindow, &MainWindow::logoutRequested, this, &LoginWindow::onLogoutRequested);
        }
        m_mainWindow->initUser(ApiClient::instance()->currentUser());
        m_mainWindow->show();
    });
}

void LoginWindow::onLogoutRequested() {
    if (m_mainWindow) {
        m_mainWindow->hide();
    }
    m_txtPassword->clear();
    m_lblError->hide();
    show();
    m_txtUsuario->setFocus();
}
