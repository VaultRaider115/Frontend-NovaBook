#include "GerenteUsuariosView.h"
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

GerenteUsuariosView::GerenteUsuariosView(QWidget *parent)
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

void GerenteUsuariosView::cargarUsuarios() {
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
        int gerentesActivos = 0;
        for (const UsuarioDto &u : std::as_const(m_usuarios)) {
            if (u.activo()) {
                ++activos;
                if (u.id_roles == 4) ++gerentesActivos;
            }
        }

        const qint64 miId = ApiClient::instance()->currentUser().id_usuarios;

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

            const bool activo = u.activo();
            const bool esMismoUsuario = (u.id_usuarios == miId);
            const bool esUnicoGerente = (u.id_roles == 4 && activo && gerentesActivos <= 1);
            const bool puedeDarDeBaja = !activo || (!esMismoUsuario && !esUnicoGerente);

            auto actions = rowActions(u.nombreCompleto(),
                [this, u]() { abrirFormulario(&u); },
                [this, u, puedeDarDeBaja, esMismoUsuario, esUnicoGerente]() {
                    if (!u.activo()) {
                        reactivar(u);
                        return;
                    }
                    if (esMismoUsuario) {
                        Toast::showToast(this, "No puedes dar de baja tu propio usuario", Toast::Warning);
                        return;
                    }
                    if (esUnicoGerente) {
                        Toast::showToast(this, "Debe permanecer al menos un gerente activo en el sistema", Toast::Warning);
                        return;
                    }
                    darDeBaja(u);
                },
                activo ? "trash" : "check-gold", activo ? "Eliminar" : "Reactivar");

            if (activo && !puedeDarDeBaja && actions.size() > 1) {
                actions.last()->setEnabled(false);
                actions.last()->setVisible(false);
            }
            Ui::cellWidgets(m_table, r, 5, actions);
        }
        finishRows(QString("Total de usuarios: %1 · %2 activos · %3 inactivos")
                       .arg(m_usuarios.size()).arg(activos).arg(m_usuarios.size() - activos));
    });
}

void GerenteUsuariosView::abrirFormulario(const UsuarioDto *u) {
    const bool edit = u != nullptr;
    int gerentesActivos = 0;
    for (const UsuarioDto &item : std::as_const(m_usuarios)) {
        if (item.activo() && item.id_roles == 4) ++gerentesActivos;
    }
    const bool esMismoUsuario = edit && u->id_usuarios == ApiClient::instance()->currentUser().id_usuarios;
    const bool esUnicoGerente = edit && u->id_roles == 4 && u->activo() && gerentesActivos <= 1;
    const bool bloquearCambioRol = esMismoUsuario || esUnicoGerente;

    Ui::FormDialog dlg(edit ? "Modificar Empleado" : "Registrar Nuevo Empleado", edit ? "Actualizar Empleado" : "Registrar Empleado", this);

    auto *txtId = new QLineEdit(edit ? QString::number(u->id_usuarios) : QString(), &dlg);
    txtId->setPlaceholderText("Ej: 350925976");
    txtId->setMaxLength(18);
    txtId->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]{0,18}"), txtId));
    txtId->setEnabled(!edit);

    const QRegularExpression rxNombre("^[\\p{L} '\\-\\.]*$");
    auto *txtNombre = new QLineEdit(edit ? u->nombre : QString(), &dlg);
    txtNombre->setPlaceholderText("Ej: Maria");
    txtNombre->setMaxLength(50);
    txtNombre->setValidator(new QRegularExpressionValidator(rxNombre, txtNombre));

    auto *comboRol = new QComboBox(&dlg);
    comboRol->addItem("Vendedor", 3);
    comboRol->addItem("Personal de Bodega", 2);
    comboRol->addItem("Jefe de Departamento", 1);
    comboRol->addItem("Gerente General", 4);
    if (edit) comboRol->setCurrentIndex(comboRol->findData(u->id_roles));
    if (bloquearCambioRol) {
        comboRol->setEnabled(false);
        comboRol->setToolTip(esMismoUsuario ? "No puedes cambiar tu propio rol"
                                            : "No puedes cambiar el rol del único gerente activo del sistema");
    }

    auto *txtPat = new QLineEdit(edit ? u->apellido_paterno : QString(), &dlg);
    txtPat->setPlaceholderText("Ej: Antonieta");
    txtPat->setMaxLength(30);
    txtPat->setValidator(new QRegularExpressionValidator(rxNombre, txtPat));

    auto *txtMat = new QLineEdit(edit ? u->apellido_materno : QString(), &dlg);
    txtMat->setPlaceholderText("Ej: de Las Nieves");
    txtMat->setMaxLength(30);
    txtMat->setValidator(new QRegularExpressionValidator(rxNombre, txtMat));

    auto *txtTel = new QLineEdit(edit && u->telefono > 0 ? QString::number(u->telefono) : QString(), &dlg);
    txtTel->setPlaceholderText("10 dígitos, ej. 8443509259");
    txtTel->setMaxLength(10);
    // Solo dígitos y sin 0 inicial (mismas reglas que la API).
    txtTel->setValidator(new QRegularExpressionValidator(QRegularExpression("[1-9][0-9]{0,9}"), txtTel));

    auto *txtPass = new QLineEdit(&dlg);
    txtPass->setEchoMode(QLineEdit::Password);
    txtPass->setMaxLength(16);
    txtPass->setPlaceholderText(edit ? "Vacío conserva la actual (máx. 16)" : "Máximo 16 caracteres");
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
    dlg.addField(edit ? "Nueva contraseña" : "Contraseña", txtPass, 2, 1);
    dlg.addField("Rol en sistema", comboRol, 3, 0, 2);

    QPointer<QDialog> guard(&dlg);
    QPointer<GerenteUsuariosView> self(this);
    connect(dlg.okButton(), &QPushButton::clicked, &dlg, [&, edit, u, guard, self, bloquearCambioRol, rxNombre]() {
        bool okId = false;
        const qint64 id = txtId->text().trimmed().toLongLong(&okId);
        if (!okId || id <= 0) {
            Toast::showToast(&dlg, "El número de empleado debe ser numérico y mayor a 0", Toast::Warning);
            txtId->setFocus();
            return;
        }
        if (!edit) {
            for (const UsuarioDto &item : std::as_const(m_usuarios)) {
                if (item.id_usuarios == id) {
                    Toast::showToast(&dlg, "Ya existe un empleado registrado con ese número", Toast::Warning);
                    txtId->setFocus();
                    return;
                }
            }
        }

        const QString nombre = txtNombre->text().simplified();   // sin espacios sobrantes, como la API
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

        const QString pat = txtPat->text().simplified();
        if (!pat.isEmpty() && !rxNombre.match(pat).hasMatch()) {
            Toast::showToast(&dlg, "El apellido paterno solo debe contener letras y espacios", Toast::Warning);
            txtPat->setFocus();
            return;
        }

        const QString mat = txtMat->text().simplified();
        if (!mat.isEmpty() && !rxNombre.match(mat).hasMatch()) {
            Toast::showToast(&dlg, "El apellido materno solo debe contener letras y espacios", Toast::Warning);
            txtMat->setFocus();
            return;
        }

        const QString telStr = txtTel->text().trimmed();
        // Mismos mensajes que la API.
        if (telStr.startsWith('0')) {
            Toast::showToast(&dlg, "El teléfono no puede empezar con 0", Toast::Warning);
            txtTel->setFocus();
            return;
        }
        if (!telStr.isEmpty() && telStr.size() != 10) {
            Toast::showToast(&dlg, QString("El teléfono debe tener exactamente 10 dígitos (tiene %1)").arg(telStr.size()),
                             Toast::Warning);
            txtTel->setFocus();
            return;
        }

        const QString pass = txtPass->text();
        if (!edit && pass.trimmed().isEmpty()) {
            Toast::showToast(&dlg, "La contraseña es obligatoria", Toast::Warning);
            txtPass->setFocus();
            return;
        }
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

        if (bloquearCambioRol && comboRol->currentData().toInt() != u->id_roles) {
            Toast::showToast(&dlg, "Debe permanecer al menos un gerente activo en el sistema", Toast::Warning);
            return;
        }

        QJsonObject obj;
        if (!edit) obj["id_usuarios"] = id;
        obj["id_roles"] = comboRol->currentData().toInt();
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

        auto done = [self, guard, edit](bool ok, const QJsonValue &, const QString &msg) {
            if (!ok) {
                if (guard) {
                    guard->findChild<QPushButton *>(); // mantiene diálogo vivo
                    static_cast<Ui::FormDialog *>(guard.data())->okButton()->setEnabled(true);
                }
                Toast::showToast(guard ? guard.data() : static_cast<QWidget *>(self.data()), "No se pudo guardar el usuario: " + msg, Toast::Error);
                return;
            }
            if (self) {
                Toast::showToast(self, edit ? "Usuario actualizado correctamente" : "Usuario creado exitosamente", Toast::Success);
                self->cargarUsuarios();
            }
            if (guard) guard->accept();
        };
        if (edit) ApiClient::instance()->actualizarUsuario(u->id_usuarios, obj, done);
        else ApiClient::instance()->crearUsuario(obj, done);
    });
    dlg.exec();
}

void GerenteUsuariosView::darDeBaja(const UsuarioDto &u) {
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

void GerenteUsuariosView::reactivar(const UsuarioDto &u) {
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
