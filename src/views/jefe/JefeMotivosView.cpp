#include "JefeMotivosView.h"
#include "Ui.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Toast.h"
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>

JefeMotivosView::JefeMotivosView(QWidget *parent)
    : CatalogView("Jefatura · Gestión de motivos", "Catálogo y Gestión de Motivos",
                  "Administra los motivos de devoluciones.",
                  "Buscar motivo...", "Registrar Nuevo Motivo",
                  {"Nombre de motivo", "Descripción", "Acciones"}, 1, 0, parent)
{
}

void JefeMotivosView::cargarMotivos() {
    ApiClient::instance()->listarMotivos([this](bool ok, const QJsonValue &data, const QString &msg) {
        if (!ok || !data.isArray()) {
            Toast::showToast(this, "Error al cargar motivos: " + msg, Toast::Error);
            return;
        }
        m_motivos.clear();
        clearRows();
        const QJsonArray arr = data.toArray();
        for (const auto &v : arr) m_motivos.append(MotivoDto::fromJson(v.toObject()));

        for (const MotivoDto &m : std::as_const(m_motivos)) {
            const int r = appendRow(m.nombre + " " + m.descripcion);
            m_table->setCellWidget(r, 0, nameCell(m.nombre));
            Ui::cellText(m_table, r, 1, m.descripcion.isEmpty() ? "—" : m.descripcion, false, false, Theme::TextMuted);
            Ui::cellWidgets(m_table, r, 2, rowActions(m.nombre,
                [this, m]() { abrirFormulario(&m); }, [this, m]() { eliminar(m); }));
        }
        finishRows(QString("Total de motivos: %1").arg(m_motivos.size()));
    });
}

void JefeMotivosView::abrirFormulario(const MotivoDto *m) {
    const bool edit = m != nullptr;
    Ui::FormDialog dlg(edit ? "Editar Motivo" : "Registrar Nuevo Motivo", edit ? "Actualizar Motivo" : "Registrar Motivo", this);
    auto *txtNombre = new QLineEdit(edit ? m->nombre : QString(), &dlg);
    txtNombre->setPlaceholderText("Ej: Finalización de contrato");
    auto *txtDesc = new QPlainTextEdit(edit ? m->descripcion : QString(), &dlg);
    txtDesc->setPlaceholderText("Ej: Fin del contrato anual de libros.");
    txtDesc->setFixedHeight(90);
    dlg.addField("Nombre del motivo", txtNombre, 0, 0, 2);
    dlg.addField("Descripción del motivo", txtDesc, 1, 0, 2);

    QPointer<QDialog> guard(&dlg);
    connect(dlg.okButton(), &QPushButton::clicked, &dlg, [&, edit, m]() {
        const QString nombre = txtNombre->text().trimmed();
        if (nombre.isEmpty()) {
            Toast::showToast(&dlg, "El nombre del motivo es obligatorio", Toast::Warning);
            return;
        }
        QJsonObject req{{"nombre_motivo", nombre}};
        req["descripcion"] = txtDesc->toPlainText().trimmed();
        auto done = [this, guard, edit](bool ok, const QJsonValue &, const QString &msg) {
            if (!ok) {
                Toast::showToast(guard ? guard.data() : static_cast<QWidget *>(this), "No se pudo guardar el motivo: " + msg, Toast::Error);
                return;
            }
            Toast::showToast(this, edit ? "Motivo actualizado" : "Motivo registrado", Toast::Success);
            if (guard) guard->accept();
            cargarMotivos();
        };
        if (edit) ApiClient::instance()->actualizarMotivo(m->id_motivo, req, done);
        else ApiClient::instance()->crearMotivo(req, done);
    });
    dlg.exec();
}

void JefeMotivosView::eliminar(const MotivoDto &m) {
    if (!confirmDelete(QString("el motivo '%1'").arg(m.nombre))) return;
    ApiClient::instance()->eliminarMotivo(m.id_motivo, [this, m](bool ok, const QJsonValue &, const QString &msg) {
        if (ok) {
            Toast::showToast(this, QString("Motivo '%1' eliminado").arg(m.nombre), Toast::Success);
            cargarMotivos();
        } else {
            Toast::showToast(this, "No se pudo eliminar el motivo: " + msg, Toast::Error);
        }
    });
}
