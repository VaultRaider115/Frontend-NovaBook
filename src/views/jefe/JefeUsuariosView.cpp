#include "JefeUsuariosView.h"
#include "Ui.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Toast.h"
#include <QAction>
#include <QComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QPointer>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

JefeUsuariosView::JefeUsuariosView(QWidget *parent)
    : CatalogView("Jefatura · Gestión de usuarios", "Gestión de Usuarios",
                  "Consulta, modifica y da de baja al personal de la librería.",
                  "Buscar por ID, nombre, rol o teléfono...", "Registrar Nuevo Usuario",
                  {"Número de empleado", "Colaborador", "Rol en sistema", "Teléfono", "Estado", "Acciones"}, 1, 0, parent)
{
    // RF16: El Jefe de Departamento NO puede crear usuarios, solo consultar/modificar/eliminar.
    m_btnNew->hide();
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
    m_table->setColumnWidth(5, 120);
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

        const qint64 miId = ApiClient::instance()->currentUser().id_usuarios;
        int activos = 0;
        for (const UsuarioDto &u : std::as_const(m_usuarios)) {
            const int r = appendRow(QString("%1 %2 %3 %4").arg(u.id_usuarios).arg(u.nombreCompleto(), Theme::roleName(u.id_roles)).arg(u.telefono));
            Ui::cellText(m_table, r, 0, QString::number(u.id_usuarios), true);

            auto *name = new QWidget();
            auto *nl = new QHBoxLayout(name);
            nl->setContentsMargins(0, 0, 0, 0);
            nl->setSpacing(12);
            nl->addWidget(Ui::avatar(u.nombreCompleto(), "gray", name));
            nl->addWidget(new QLabel(u.nombreCompleto(), name));
            nl->addStretch();
            m_table->setCellWidget(r, 1, name);

            Ui::cellWidget(m_table, r, 2, Ui::chip(Theme::roleName(u.id_roles), chipRol(u.id_roles).toUtf8().constData()));
            Ui::cellText(m_table, r, 3, u.telefono > 0 ? Ui::phone(u.telefono) : "—", false, false, Theme::TextMuted);
            Ui::cellWidget(m_table, r, 4, Ui::chip(u.activo() ? "● Activo" : "Inactivo", u.activo() ? "ok" : "neutral"));
            if (u.activo()) ++activos;

            if (u.id_roles == 4) {
                // Las cuentas de Gerente solo las administra el Gerente
                Ui::cellText(m_table, r, 5, "Solo Gerente", false, false, Theme::TextMuted);
                continue;
            }
            const bool activo = u.activo();
            const bool esMismoUsuario = (u.id_usuarios == miId);

            auto actions = rowActions(u.nombreCompleto(),
                [this, u]() { abrirFormulario(&u); },
                [this, u, esMismoUsuario]() {
                    if (!u.activo()) {
                        reactivar(u);
                        return;
                    }
                    if (esMismoUsuario) {
                        Toast::showToast(this, "No puedes dar de baja tu propio usuario", Toast::Warning);
                        return;
                    }
                    darDeBaja(u);
                },
                activo ? "trash" : "check-gold", activo ? "Eliminar" : "Reactivar");

            if (activo && esMismoUsuario && actions.size() > 1) {
                actions.last()->setEnabled(false);
                actions.last()->setVisible(false);
            }
            Ui::cellWidgets(m_table, r, 5, actions);
        }
        finishRows(QString("Total de usuarios: %1 · %2 activos · %3 inactivos")
                       .arg(m_usuarios.size()).arg(activos).arg(m_usuarios.size() - activos));
    });
}

void JefeUsuariosView::abrirFormulario(const UsuarioDto *u) {
    const bool edit = u != nullptr;
    if (!edit) {
        Toast::showToast(this, "Solo un Gerente General puede registrar nuevos usuarios", Toast::Warning);
        return;
    }
    if (u->id_roles == 4) {
        Toast::showToast(this, "No tienes permisos para modificar a un Gerente General", Toast::Warning);
        return;
    }

    const qint64 miId = ApiClient::instance()->currentUser().id_usuarios;
    const bool esMismoUsuario = (u->id_usuarios == miId);

    Ui::FormDialog dlg("Modificar Empleado", "Actualizar Empleado", this);

    auto *txtId = new QLineEdit(QString::number(u->id_usuarios), &dlg);
    txtId->setMaxLength(18);
    txtId->setEnabled(false);

    const QRegularExpression rxNombre("^[\\p{L} '\\-\\.]+$");
    auto *txtNombre = new QLineEdit(u->nombre, &dlg);
    txtNombre->setPlaceholderText("Ej: Maria");
    txtNombre->setMaxLength(50);

    auto *comboRol = new QComboBox(&dlg);
    comboRol->addItem("Vendedor", 3);
    comboRol->addItem("Personal de Bodega", 2);
    comboRol->addItem("Jefe de Departamento", 1);
    comboRol->setCurrentIndex(comboRol->findData(u->id_roles));
    // Solo el Gerente asigna o cambia roles; la jefatura únicamente edita datos.
    comboRol->setEnabled(false);
    comboRol->setToolTip(esMismoUsuario ? "No puedes cambiar tu propio rol"
                                        : "Solo el Gerente puede asignar o cambiar roles");

    auto *txtPat = new QLineEdit(u->apellido_paterno, &dlg);
    txtPat->setPlaceholderText("Ej: Antonieta");
    txtPat->setMaxLength(30);

    auto *txtMat = new QLineEdit(u->apellido_materno, &dlg);
    txtMat->setPlaceholderText("Ej: de Las Nieves");
    txtMat->setMaxLength(30);

    auto *txtTel = new QLineEdit(u->telefono > 0 ? QString::number(u->telefono) : QString(), &dlg);
    txtTel->setPlaceholderText("10 dígitos, ej. 8443509259");
    txtTel->setMaxLength(10);
    txtTel->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{0,10}"), txtTel));

    auto *txtPass = new QLineEdit(&dlg);
    txtPass->setEchoMode(QLineEdit::Password);
    txtPass->setMaxLength(16);
    txtPass->setPlaceholderText("Vacío conserva la actual (máx. 16)");
    auto *eye = txtPass->addAction(QIcon(":/icons/eye.svg"), QLineEdit::TrailingPosition);
    eye->setToolTip("Mostrar contraseña");
    connect(eye, &QAction::triggered, &dlg, [txtPass, eye]() {
        const bool hidden = txtPass->echoMode() == QLineEdit::Password;
        txtPass->setEchoMode(hidden ? QLineEdit::Normal : QLineEdit::Password);
        eye->setIcon(QIcon(hidden ? ":/icons/eye-off.svg" : ":/icons/eye.svg"));
        eye->setToolTip(hidden ? "Ocultar contraseña" : "Mostrar contraseña");
    });

    dlg.addField("Número de Empleado", txtId, 0, 0);
    dlg.addField("Nombre", txtNombre, 0, 1);
    dlg.addField("Apellido Paterno", txtPat, 1, 0);
    dlg.addField("Apellido Materno", txtMat, 1, 1);
    dlg.addField("Teléfono", txtTel, 2, 0);
    dlg.addField("Nueva contraseña", txtPass, 2, 1);
    dlg.addField("Rol en sistema", comboRol, 3, 0, 2);

    QPointer<QDialog> guard(&dlg);
    QPointer<JefeUsuariosView> self(this);
    connect(dlg.okButton(), &QPushButton::clicked, &dlg, [&, u, guard, self, esMismoUsuario, rxNombre]() {
        const QString nombre = txtNombre->text().trimmed();
        if (nombre.isEmpty()) {
            Toast::showToast(&dlg, "El nombre es obligatorio", Toast::Warning);
            txtNombre->setFocus();
            return;
        }
        if (!rxNombre.match(nombre).hasMatch()) {
            Toast::showToast(&dlg, "El nombre solo debe contener letras y espacios", Toast::Warning);
            txtNombre->setFocus();
            return;
        }

        const QString pat = txtPat->text().trimmed();
        if (!pat.isEmpty() && !rxNombre.match(pat).hasMatch()) {
            Toast::showToast(&dlg, "El apellido paterno solo debe contener letras y espacios", Toast::Warning);
            txtPat->setFocus();
            return;
        }

        const QString mat = txtMat->text().trimmed();
        if (!mat.isEmpty() && !rxNombre.match(mat).hasMatch()) {
            Toast::showToast(&dlg, "El apellido materno solo debe contener letras y espacios", Toast::Warning);
            txtMat->setFocus();
            return;
        }

        const QString telStr = txtTel->text().trimmed();
        if (!telStr.isEmpty() && (telStr.size() != 10 || telStr.startsWith('0'))) {
            Toast::showToast(&dlg, "El teléfono debe tener 10 dígitos válidos (sin iniciar en 0)", Toast::Warning);
            txtTel->setFocus();
            return;
        }

        const QString pass = txtPass->text();
        if (!pass.isEmpty() && pass.trimmed().isEmpty()) {
            Toast::showToast(&dlg, "La contraseña no puede contener solo espacios", Toast::Warning);
            txtPass->setFocus();
            return;
        }
        if (pass.size() > 16) {
            Toast::showToast(&dlg, "La contraseña no puede exceder 16 caracteres", Toast::Warning);
            txtPass->setFocus();
            return;
        }

        // El rol no se envía: el Jefe no puede cambiarlo y el backend lo rechazaría.
        QJsonObject obj;
        obj["nombre"] = nombre;
        obj["apellido_paterno"] = pat;
        obj["apellido_materno"] = mat;
        if (!telStr.isEmpty()) {
            bool okTelefono = false;
            const qint64 telefono = telStr.toLongLong(&okTelefono);
            if (!okTelefono || telefono < 1000000000LL) {
                Toast::showToast(&dlg, "El teléfono debe ser numérico de 10 dígitos", Toast::Warning);
                txtTel->setFocus();
                return;
            }
            obj["telefono"] = telefono;
        }
        if (!pass.isEmpty()) obj["contrasena"] = pass;

        dlg.okButton()->setEnabled(false);

        auto done = [self, guard](bool ok, const QJsonValue &, const QString &msg) {
            if (!ok) {
                if (guard) static_cast<Ui::FormDialog *>(guard.data())->okButton()->setEnabled(true);
                Toast::showToast(guard ? guard.data() : static_cast<QWidget *>(self.data()), "No se pudo guardar el usuario: " + msg, Toast::Error);
                return;
            }
            if (self) {
                Toast::showToast(self, "Usuario actualizado correctamente", Toast::Success);
                self->cargarUsuarios();
            }
            if (guard) guard->accept();
        };
        ApiClient::instance()->actualizarUsuario(u->id_usuarios, obj, done);
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

void JefeUsuariosView::reactivar(const UsuarioDto &u) {
    QJsonObject obj;
    obj["estatus"] = true;
    ApiClient::instance()->actualizarUsuario(u.id_usuarios, obj, [this](bool ok, const QJsonValue &, const QString &msg) {
        if (ok) {
            Toast::showToast(this, "Usuario reactivado exitosamente", Toast::Success);
            cargarUsuarios();
        } else {
            Toast::showToast(this, "Error al reactivar: " + msg, Toast::Error);
        }
    });
}
