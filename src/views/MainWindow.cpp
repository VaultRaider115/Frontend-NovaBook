#include "MainWindow.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Ui.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QMessageBox>
#include <QGuiApplication>
#include <QScreen>

// Jefe Views
#include "JefeDashboardView.h"
#include "JefeBitacoraView.h"
#include "JefeDevolucionesView.h"
#include "JefeUsuariosView.h"
#include "JefeGenerosView.h"
#include "JefeMotivosView.h"
#include "JefeMueblesView.h"

// Gerente Views
#include "GerenteDashboardView.h"
#include "GerenteUsuariosView.h"

// Bodega Views
#include "BodegaDashboardView.h"
#include "BodegaRegistroView.h"
#include "BodegaComprasView.h"
#include "BodegaAutoresView.h"
#include "BodegaProveedoresView.h"
#include "BodegaTrasladoView.h"
#include "BodegaInventarioView.h"

// Vendedor Views
#include "VendedorDashboardView.h"
#include "VendedorConsultaView.h"
#include "VendedorVentasView.h"
#include "VendedorTrasladoView.h"
#include "VendedorDevolucionView.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("NovaBook — Sistema de Gestión");
    resize(1360, 820);
    setMinimumSize(1100, 680);

    const QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    move((screenGeometry.width() - width()) / 2, (screenGeometry.height() - height()) / 2);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setupSidebar();
    mainLayout->addWidget(m_sidebarWidget);

    m_stack = new QStackedWidget(this);
    mainLayout->addWidget(m_stack, 1);
}

void MainWindow::setupSidebar() {
    m_sidebarWidget = new QWidget(this);
    m_sidebarWidget->setObjectName("Sidebar");
    m_sidebarWidget->setFixedWidth(256);
    m_sidebarWidget->setStyleSheet(QString(R"(
        #Sidebar { background-color: %1; border-right: 1px solid %2; }
        QPushButton[nav="true"] {
            background-color: transparent; color: %3; text-align: left;
            padding: 11px 14px; border: none; border-radius: 8px; font-weight: 500;
        }
        QPushButton[nav="true"]:hover { background-color: rgba(255,255,255,5%); color: %4; }
        QPushButton[nav="true"][current="true"] {
            background-color: rgba(212,163,70,10%); color: %5; font-weight: 600;
        }
    )").arg(Theme::BgSidebar, Theme::Border, Theme::TextSidebar, Theme::TextPrimary, Theme::Accent));

    auto *sb = new QVBoxLayout(m_sidebarWidget);
    sb->setContentsMargins(16, 24, 16, 20);
    sb->setSpacing(14);

    auto *brand = new QLabel("Nova Book", m_sidebarWidget);
    brand->setStyleSheet("font-size: 22px; font-weight: 800; color: #FFFFFF;");
    sb->addWidget(brand);

    auto *loc = new QLabel("Sanborns · Saltillo", m_sidebarWidget);
    loc->setStyleSheet(QString("font-size: 12px; color: %1;").arg(Theme::Accent));
    sb->addWidget(loc);

    sb->addWidget(Ui::divider(m_sidebarWidget));

    m_lblUserName = new QLabel("Cargando...", m_sidebarWidget);
    m_lblUserName->setStyleSheet("font-size: 14px; font-weight: 600;");
    m_lblUserName->setWordWrap(true);
    sb->addWidget(m_lblUserName);

    m_lblRoleBadge = new QLabel("Rol", m_sidebarWidget);
    m_lblRoleBadge->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    sb->addWidget(m_lblRoleBadge);

    sb->addWidget(Ui::divider(m_sidebarWidget));

    auto *navContainer = new QWidget(m_sidebarWidget);
    m_navLayout = new QVBoxLayout(navContainer);
    m_navLayout->setContentsMargins(0, 0, 0, 0);
    m_navLayout->setSpacing(4);
    sb->addWidget(navContainer);
    sb->addStretch();

    m_btnLogout = Ui::button("Cerrar Sesión", "outline", m_sidebarWidget);
    sb->addWidget(m_btnLogout);

    connect(m_btnLogout, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
}

void MainWindow::initUser(const UsuarioDto &user) {
    m_user = user;
    m_lblUserName->setText(user.nombreCompleto());
    m_lblRoleBadge->setText(Theme::roleName(user.id_roles));
    m_lblRoleBadge->setStyleSheet(Theme::roleBadgeStyle(user.id_roles));
    buildRoleViews();
}

void MainWindow::addPage(const QString &navText, QWidget *view, std::function<void()> onShow) {
    const int index = m_stack->addWidget(view);
    auto *btn = new QPushButton(navText, m_sidebarWidget);
    btn->setProperty("nav", true);
    btn->setCursor(Qt::PointingHandCursor);
    connect(btn, &QPushButton::clicked, this, [this, index]() { onNavButtonClicked(index); });
    m_navButtons.append(btn);
    m_navLayout->addWidget(btn);
    m_onShow.append(std::move(onShow));
}

void MainWindow::updateActiveNavButton(int activeIndex) {
    for (int i = 0; i < m_navButtons.size(); ++i)
        Ui::setCurrent(m_navButtons.at(i), i == activeIndex);
}

void MainWindow::buildRoleViews() {
    for (auto *b : std::as_const(m_navButtons)) {
        m_navLayout->removeWidget(b);
        delete b;
    }
    m_navButtons.clear();
    m_onShow.clear();
    while (m_stack->count() > 0) {
        QWidget *w = m_stack->widget(0);
        m_stack->removeWidget(w);
        delete w;
    }

    if (m_user.id_roles == 4) {
        // --- GERENTE / SUBGERENTE  (RF01: CRUD Usuarios) ---
        auto *dash     = new GerenteDashboardView(m_stack);
        auto *usuarios = new GerenteUsuariosView(m_stack);

        addPage("Panel Principal",     dash,     [dash]()     { dash->refreshData(); });
        addPage("Gestión de Usuarios", usuarios, [usuarios]() { usuarios->cargarUsuarios(); });

    } else if (m_user.id_roles == 1) {
        // --- JEFE DE DEPARTAMENTO ---
        // RF03: Consultar Movimiento Diario
        // RF02: Consultar Historial de Devoluciones
        // RF04: Aprobar Devoluciones
        // RF16: Gestionar Usuarios (consultar, modificar, eliminar)
        // RF18: Gestionar Catálogo de Autores
        // RF19: Gestionar Catálogo de Géneros
        // RF20: Gestionar Motivos de Devolución
        // RF21: Gestionar Muebles y Estantería
        auto *dash     = new JefeDashboardView(m_stack);
        auto *bitacora = new JefeBitacoraView(m_stack);
        auto *devs     = new JefeDevolucionesView(m_stack);
        auto *usuarios = new JefeUsuariosView(m_stack);
        auto *autores  = new BodegaAutoresView("Jefatura · Gestión de autores", m_stack);
        auto *generos  = new JefeGenerosView(m_stack);
        auto *motivos  = new JefeMotivosView(m_stack);
        auto *muebles  = new JefeMueblesView(m_stack);

        addPage("Panel Principal",            dash,     [dash]()     { dash->refreshData(); });
        addPage("Historial de Movimientos",   bitacora, [bitacora]() { bitacora->cargarMovimientos(); });
        addPage("Aprobación de Devoluciones", devs,     [devs]()     { devs->cargarDevoluciones(); });
        addPage("Gestión de Usuarios",        usuarios, [usuarios]() { usuarios->cargarUsuarios(); });
        addPage("Catálogo de Autores",        autores,  [autores]()  { autores->cargarAutores(); });
        addPage("Catálogo de Géneros",        generos,  [generos]()  { generos->cargarGeneros(); });
        addPage("Catálogo de Motivos",        motivos,  [motivos]()  { motivos->cargarMotivos(); });
        addPage("Muebles y Estantería",       muebles,  [muebles]()  { muebles->cargarMuebles(); });

    } else if (m_user.id_roles == 2) {
        // --- BODEGA ---
        auto *dash = new BodegaDashboardView(m_stack);
        auto *registro = new BodegaRegistroView(m_stack);
        auto *compras = new BodegaComprasView(m_stack);
        auto *autores = new BodegaAutoresView("Bodega · Gestión de autores", m_stack);
        auto *prov = new BodegaProveedoresView(m_stack);
        auto *traslado = new BodegaTrasladoView(m_stack);
        auto *inv = new BodegaInventarioView(m_stack);

        addPage("Panel Principal", dash, [dash]() { dash->refreshData(); });
        addPage("Alta de Mercancía (EAN)", registro, [registro]() { registro->cargarCatalogos(); });
        addPage("Compras a Proveedor", compras, [compras]() { compras->cargarProveedores(); });
        addPage("Autores", autores, [autores]() { autores->cargarAutores(); });
        addPage("Proveedores", prov, [prov]() { prov->cargarProveedores(); });
        addPage("Requisición de Salida", traslado);
        addPage("Consultar Inventario", inv, [inv]() { inv->cargarInventario(); });

    } else {
        // --- VENDEDOR ---
        auto *dash = new VendedorDashboardView(m_stack);
        auto *consulta = new VendedorConsultaView(m_stack);
        auto *ventas = new VendedorVentasView(m_stack);
        auto *traslado = new VendedorTrasladoView(m_stack);
        auto *devolucion = new VendedorDevolucionView(m_stack);

        addPage("Panel Principal", dash, [dash]() { dash->refreshData(); });
        addPage("Consulta de Existencias", consulta, [consulta]() { consulta->buscarExistencias(); });
        addPage("Punto de Venta", ventas);
        addPage("Requisición de Entrada", traslado);
        addPage("Devoluciones a Proveedor", devolucion, [devolucion]() { devolucion->cargarCatalogos(); });
    }

    onNavButtonClicked(0);
}

void MainWindow::onNavButtonClicked(int index) {
    if (index < 0 || index >= m_stack->count()) return;
    m_stack->setCurrentIndex(index);
    updateActiveNavButton(index);
    if (index < m_onShow.size() && m_onShow.at(index)) m_onShow.at(index)();
}

void MainWindow::onLogoutClicked() {
    if (QMessageBox::question(this, "Cerrar Sesión", "¿Deseas cerrar tu sesión actual?",
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        ApiClient::instance()->logout();
        emit logoutRequested();
    }
}
