#include "LoginWindow.h"
#include "MainWindow.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Limites.h"
#include "Ui.h"
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QIcon>
#include <QGuiApplication>
#include <QScreen>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

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
    // Diseño 10: "NOVA BOOK" en mayúsculas espaciadas, BOOK en naranja.
    auto *brand = new QLabel("NOVA <span style=\"color:#EFA65A;\">BOOK</span>", this);
    brand->setTextFormat(Qt::RichText);
    brand->setAlignment(Qt::AlignCenter);
    brand->setStyleSheet("font-size: 54px; font-weight: 800; color: #E5E5E5;");
    Ui::spaced(brand, 8);
    content->addWidget(brand);

    auto *sub = new QLabel("Sistema de Gestión de Librería", this);
    sub->setAlignment(Qt::AlignCenter);
    sub->setStyleSheet("font-size: 15px; color: #9CA3AF;");
    Ui::spaced(sub, 0.5);
    content->addWidget(sub);

    // Píldora: el radio debe ser <= media altura o Qt dibuja esquinas rectas.
    auto *pill = new QLabel("Sanborns · Saltillo", this);
    pill->setAlignment(Qt::AlignCenter);
    pill->setFixedHeight(36);
    pill->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    pill->setStyleSheet("background-color: #1C1C1D; border: 1px solid #3F3F46; border-radius: 18px; "
                        "color: #FFC964; font-size: 13px; padding: 0px 16px;");
    content->addWidget(pill, 0, Qt::AlignHCenter);

    // Tarjeta de sesión
    auto *card = new QFrame(this);
    card->setObjectName("LoginCard");
    card->setStyleSheet("#LoginCard { background-color: #141414; border: 1px solid #232323; border-radius: 12px; }");
    auto *cl = new QVBoxLayout(card);
    cl->setContentsMargins(32, 32, 32, 32);
    cl->setSpacing(14);

    auto *cardTitle = new QLabel("INICIAR SESIÓN", card);
    cardTitle->setStyleSheet("font-size: 14px; font-weight: 700; color: #F5F5F5;");
    Ui::spaced(cardTitle, 1.5);
    cl->addWidget(cardTitle);

    // Alto fijo en vez de padding vertical: así el ícono del ojo queda centrado.
    const QString inputQss = "QLineEdit { background-color: #1C1C1C; border: 1px solid #2A2A2A; border-radius: 8px; "
                             "padding: 0px 18px; min-height: 46px; max-height: 46px; font-size: 15px; } "
                             "QLineEdit:focus { border: 1px solid #D4A346; }";

    m_txtUsuario = new QLineEdit(card);
    m_txtUsuario->setStyleSheet(inputQss);
    m_txtUsuario->setAccessibleName("Número de empleado");
    m_txtUsuario->setMaxLength(18);
    m_txtUsuario->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{0,18}"), m_txtUsuario));
    m_txtUsuario->setInputMethodHints(Qt::ImhDigitsOnly);
    cl->addWidget(Ui::field("Número de Empleado", m_txtUsuario, card));

    m_txtPassword = new QLineEdit(card);
    m_txtPassword->setEchoMode(QLineEdit::Password);
    m_txtPassword->setMaxLength(Limites::kPassword);
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

    m_btnLogin = Ui::button("INGRESAR AL SISTEMA", "primary", card);
    m_btnLogin->setStyleSheet("QPushButton { padding: 14px; font-size: 15px; font-weight: 700; }");
    Ui::spaced(m_btnLogin, 1.5);
    cl->addWidget(m_btnLogin);
    content->addWidget(card);

    // Aviso legal
    auto *legal = new QLabel("NOVABOOK · USO EXCLUSIVO PARA PERSONAL AUTORIZADO DE SANBORNS", this);
    legal->setAlignment(Qt::AlignCenter);
    legal->setWordWrap(true);
    legal->setStyleSheet("background-color: #141414; border-radius: 12px; color: #999292; "
                         "font-size: 13px; font-weight: 700; padding: 22px 48px;");
    Ui::spaced(legal, 1.5);
    content->addWidget(legal);
    content->addStretch();

    connect(m_btnLogin, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(m_txtUsuario, &QLineEdit::returnPressed, this, [this]() { m_txtPassword->setFocus(); });
    connect(m_txtPassword, &QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);
    connect(ApiClient::instance(), &ApiClient::sessionExpired, this, &LoginWindow::onSessionExpired);
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
    if (pass.size() > 16) {
        m_lblError->setText("La contraseña no puede exceder 16 caracteres.");
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

void LoginWindow::onSessionExpired(const QString &reason) {
    // Cierra diálogos abiertos (formularios, confirmaciones) que ya no tienen sesión.
    // El límite evita un ciclo si algún modal se resiste a cerrarse.
    for (int i = 0; i < 10; ++i) {
        auto *dlg = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dlg) break;
        dlg->reject();
    }

    // Mostrar el login antes de ocultar la principal para que la app no se cierre.
    m_txtPassword->clear();
    m_lblError->setText(reason);
    m_lblError->show();
    show();
    raise();
    activateWindow();
    if (m_mainWindow) {
        m_mainWindow->hide();
    }
    m_txtPassword->setFocus();
}
