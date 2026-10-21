#include "JefeUsuariosView.h"
#include "Ui.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Toast.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPointer>

JefeUsuariosView::JefeUsuariosView(QWidget *parent)
    : CatalogView("Gerencia · Catálogo de usuarios", "Administración y Gestión de Usuarios",
                  "Control de acceso del personal de la librería.",
                  "Buscar por ID, nombre, rol o teléfono...", "Registrar Nuevo Usuario",
                  {"Número de empleado", "Colaborador", "Rol en sistema", "Teléfono", "Estado", "Acciones"}, 1, 0, parent)
{
    // Filtro segmentado Todos / Activos / Inactivos
    auto *seg = new QWidget(this);
    auto *sl = new QHBoxLayout(seg);
    sl->setContentsMargins(0, 0, 0, 0);
    sl->setSpacing(6);
    const QStringList nombres = {"Todos", "Activos", "Inactivos"};
    for (int i = 0; i < nombres.size(); ++i) {
        auto *b = Ui::button(nombres.at(i), "page", seg);
        Ui::setCurrent(b, i == 0);
        connect(b, &QPushButton::clicked, this, [this, i]() {
            m_filtroEstado = i;
            for (int k = 0; k < m_segmentos.size(); ++k) Ui::setCurrent(m_segmentos.at(k), k == i);
            refilter();
        });
        m_segmentos << b;
        sl->addWidget(b);
    }
    m_toolsLayout->insertWidget(1, seg);

    m_rowFilter = [this](int row) {
        if (row >= m_usuarios.size() || m_filtroEstado == 0) return true;
        return m_usuarios.at(row).activo() == (m_filtroEstado == 1);
    };
}

static QString chipRol(int rol) {
    switch (rol) {
        case 4: return "gold";
        case 1: return "indigo";
        case 2: return "amber";
        default: return "mint";
    }
}

void JefeUsuariosView::cargarUsuarios() {
    ApiClient::instance()->listarUsuarios([this](bool ok, const QJsonValue &data, const QString &msg) {
        if (!ok || !data.isArray()) {
            Toast::showToast(this, "Error al obtener usuarios: " + msg, Toast::Error);
            return;
        }
        m_usuarios.clear();
        clearRows();
        const QJsonArray arr = data.toArray();
        for (const auto &v : arr) m_usuarios.append(UsuarioDto::fromJson(v.toObject()));

        int activos = 0;
        for (const UsuarioDto &u : std::as_const(m_usuarios)) {
            const int r = appendRow(QString("%1 %2 %3 %4").arg(u.id_usuarios).arg(u.nombreCompleto(), Theme::roleName(u.id_roles)).arg(u.telefono));
            Ui::cellText(m_table, r, 0, QString::number(u.id_usuarios), true);

            auto *name = new QWidget();
            auto *nl = new QHBoxLayout(name);
            nl->setContentsMargins(16, 0, 16, 0);
            nl->setSpacing(12);
            nl->addWidget(Ui::avatar(u.nombreCompleto(), "gray", name));
            nl->addWidget(new QLabel(u.nombreCompleto(), name));
            nl->addStretch();
            m_table->setCellWidget(r, 1, name);

            Ui::cellWidget(m_table, r, 2, Ui::chip(Theme::roleName(u.id_roles), chipRol(u.id_roles).toUtf8().constData()));
            Ui::cellText(m_table, r, 3, u.telefono > 0 ? QString::number(u.telefono) : "—", false, false, Theme::TextMuted);
            Ui::cellWidget(m_table, r, 4, Ui::chip(u.activo() ? "● Activo" : "Inactivo", u.activo() ? "ok" : "neutral"));
            if (u.activo()) ++activos;

            Ui::cellWidgets(m_table, r, 5, rowActions(u.nombreCompleto(),
                [this, u]() { abrirFormulario(&u); }, [this, u]() { darDeBaja(u); }));
        }
        finishRows(QString("Total de usuarios: %1 · %2 activos · %3 inactivos")
                       .arg(m_usuarios.size()).arg(activos).arg(m_usuarios.size() - activos));
    });
}

void JefeUsuariosView::abrirFormulario(const UsuarioDto *u) {
    const bool edit = u != nullptr;
    Ui::FormDialog dlg(edit ? "Modificar Empleado" : "Registrar Nuevo Empleado", edit ? "Actualizar Empleado" : "Registrar Empleado", this);

    auto *txtId = new QLineEdit(edit ? QString::number(u->id_usuarios) : QString(), &dlg);
    txtId->setPlaceholderText("Ej: 350925976");
    txtId->setEnabled(!edit);
    auto *txtNombre = new QLineEdit(edit ? u->nombre : QString(), &dlg);
    txtNombre->setPlaceholderText("Ej: Maria");
    auto *comboRol = new QComboBox(&dlg);
    comboRol->addItem("Vendedor", 3);
    comboRol->addItem("Personal de Bodega", 2);
    comboRol->addItem("Jefe de Departamento", 1);
    comboRol->addItem("Gerente General", 4);
    if (edit) comboRol->setCurrentIndex(comboRol->findData(u->id_roles));
    auto *txtPat = new QLineEdit(edit ? u->apellido_paterno : QString(), &dlg);
    txtPat->setPlaceholderText("Ej: Antonieta");
    auto *txtMat = new QLineEdit(edit ? u->apellido_materno : QString(), &dlg);
    txtMat->setPlaceholderText("Ej: de Las Nieves");
    auto *txtTel = new QLineEdit(edit && u->telefono > 0 ? QString::number(u->telefono) : QString(), &dlg);
    txtTel->setPlaceholderText("Ej: 8443509259");
    auto *txtPass = new QLineEdit(&dlg);
    txtPass->setEchoMode(QLineEdit::Password);
    txtPass->setPlaceholderText(edit ? "Dejar vacío para conservar la actual" : "Contraseña");

    dlg.addField("Número de Empleado", txtId, 0, 0);
    dlg.addField("Nombre", txtNombre, 0, 1);
    dlg.addField("Rol en sistema", comboRol, 1, 0, 2);
    dlg.addField("Apellido Paterno", txtPat, 2, 0);
    dlg.addField("Apellido Materno", txtMat, 2, 1);
    dlg.addField("Teléfono", txtTel, 3, 0);
    dlg.addField(edit ? "Nueva contraseña" : "Contraseña", txtPass, 3, 1);

    QPointer<QDialog> guard(&dlg);
    connect(dlg.okButton(), &QPushButton::clicked, &dlg, [&, edit, u]() {
        bool okId = false;
        const qint64 id = txtId->text().trimmed().toLongLong(&okId);
        if (!okId || id <= 0) {
            Toast::showToast(&dlg, "El número de empleado debe ser numérico y válido", Toast::Warning);
            return;
        }
        if (txtNombre->text().trimmed().isEmpty() || (!edit && txtPass->text().isEmpty())) {
            Toast::showToast(&dlg, "Nombre y contraseña son obligatorios", Toast::Warning);
            return;
        }
        QJsonObject obj;
        if (!edit) obj["id_usuarios"] = id;
        obj["id_roles"] = comboRol->currentData().toInt();
        obj["nombre"] = txtNombre->text().trimmed();
        obj["apellido_paterno"] = txtPat->text().trimmed();
        obj["apellido_materno"] = txtMat->text().trimmed();
        if (!txtTel->text().trimmed().isEmpty()) obj["telefono"] = txtTel->text().trimmed().toLongLong();
        if (!txtPass->text().isEmpty()) obj["contrasena"] = txtPass->text();

        auto done = [this, guard, edit](bool ok, const QJsonValue &, const QString &msg) {
            if (!ok) {
                Toast::showToast(guard ? guard.data() : static_cast<QWidget *>(this), "No se pudo guardar el usuario: " + msg, Toast::Error);
                return;
            }
            Toast::showToast(this, edit ? "Usuario actualizado correctamente" : "Usuario creado exitosamente", Toast::Success);
            if (guard) guard->accept();
            cargarUsuarios();
        };
        if (edit) ApiClient::instance()->actualizarUsuario(u->id_usuarios, obj, done);
        else ApiClient::instance()->crearUsuario(obj, done);
    });
    dlg.exec();
}

void JefeUsuariosView::darDeBaja(const UsuarioDto &u) {
    if (!confirmDelete(QString("(dar de baja) al usuario %1 (%2)").arg(u.nombreCompleto()).arg(u.id_usuarios))) return;
    ApiClient::instance()->eliminarUsuario(u.id_usuarios, [this](bool ok, const QJsonValue &, const QString &msg) {
        if (ok) {
            Toast::showToast(this, "Usuario dado de baja exitosamente", Toast::Success);
            cargarUsuarios();
        } else {
            Toast::showToast(this, "Error al dar de baja: " + msg, Toast::Error);
        }
    });
}
