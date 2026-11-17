#include "JefeMotivosView.h"
#include "Ui.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Toast.h"
#include <QHeaderView>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

JefeMotivosView::JefeMotivosView(QWidget *parent)
    : CatalogView("Jefatura · Gestión de motivos", "Catálogo y Gestión de Motivos",
                  "Administra los motivos de devoluciones.",
                  "Buscar motivo...", "Registrar Nuevo Motivo",
                  {"Nombre de motivo", "Descripción", "Acciones"}, 1, 0, parent)
{
    auto *hh = m_table->horizontalHeader();
    hh->setSectionResizeMode(0, QHeaderView::Interactive);
    m_table->setColumnWidth(0, 265);
    hh->setSectionResizeMode(1, QHeaderView::Stretch);
    hh->setSectionResizeMode(2, QHeaderView::ResizeToContents);
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

            const bool enUso = (m.total_devoluciones > 0);
            auto actions = rowActions(m.nombre,
                [this, m]() { abrirFormulario(&m); },
                [this, m]() { eliminar(m); });

            if (enUso && actions.size() > 1) {
                actions.last()->setEnabled(false);
                actions.last()->setVisible(false);
            }
            Ui::cellWidgets(m_table, r, 2, actions);
        }
        finishRows(QString("Total de motivos: %1").arg(m_motivos.size()));
    });
}

void JefeMotivosView::abrirFormulario(const MotivoDto *m) {
    const bool edit = m != nullptr;
    Ui::FormDialog dlg(edit ? "Editar Motivo" : "Registrar Nuevo Motivo", edit ? "Actualizar Motivo" : "Registrar Motivo", this);

    const QRegularExpression rxMotivo("^[\\p{L} '\\-\\.,/()]*$");
    auto *txtNombre = new QLineEdit(edit ? m->nombre : QString(), &dlg);
    txtNombre->setPlaceholderText("Ej: Finalización de contrato");
    txtNombre->setMaxLength(50);
    txtNombre->setValidator(new QRegularExpressionValidator(rxMotivo, txtNombre));

    auto *txtDesc = new QPlainTextEdit(edit ? m->descripcion : QString(), &dlg);
    txtDesc->setPlaceholderText("Ej: Fin del contrato anual de libros (máx. 200 caracteres).");
    txtDesc->setFixedHeight(90);

    dlg.addField("Nombre del motivo", txtNombre, 0, 0, 2);
    dlg.addField("Descripción del motivo", txtDesc, 1, 0, 2);

    QPointer<QDialog> guard(&dlg);
    QPointer<JefeMotivosView> self(this);
    connect(dlg.okButton(), &QPushButton::clicked, &dlg, [&, edit, m, guard, self, rxMotivo]() {
        const QString nombre = txtNombre->text().trimmed();
        if (nombre.isEmpty()) {
            Toast::showToast(&dlg, "El nombre del motivo es obligatorio", Toast::Warning);
            txtNombre->setFocus();
            return;
        }
        if (!rxMotivo.match(nombre).hasMatch()) {
            Toast::showToast(&dlg, "El nombre del motivo solo debe contener letras y signos básicos", Toast::Warning);
            txtNombre->setFocus();
            return;
        }

        const QString desc = txtDesc->toPlainText().trimmed();
        if (desc.size() > 200) {
            Toast::showToast(&dlg, "La descripción no puede exceder 200 caracteres", Toast::Warning);
            txtDesc->setFocus();
            return;
        }

        for (const MotivoDto &item : std::as_const(m_motivos)) {
            if (edit && item.id_motivo == m->id_motivo) continue;
            if (item.nombre.trimmed().compare(nombre, Qt::CaseInsensitive) == 0) {
                Toast::showToast(&dlg, "Ya existe un motivo registrado con ese nombre", Toast::Warning);
                txtNombre->setFocus();
                return;
            }
        }

        dlg.okButton()->setEnabled(false);
        QJsonObject req{{"nombre_motivo", nombre}};
        req["descripcion"] = desc;

        auto done = [self, guard, edit](bool ok, const QJsonValue &, const QString &msg) {
            if (!ok) {
                if (guard) static_cast<Ui::FormDialog *>(guard.data())->okButton()->setEnabled(true);
                Toast::showToast(guard ? guard.data() : static_cast<QWidget *>(self.data()), "No se pudo guardar el motivo: " + msg, Toast::Error);
                return;
            }
            if (self) {
                Toast::showToast(self, edit ? "Motivo actualizado" : "Motivo registrado", Toast::Success);
                self->cargarMotivos();
            }
            if (guard) guard->accept();
        };
        if (edit) ApiClient::instance()->actualizarMotivo(m->id_motivo, req, done);
        else ApiClient::instance()->crearMotivo(req, done);
    });
    dlg.exec();
}

void JefeMotivosView::eliminar(const MotivoDto &m) {
    if (m.total_devoluciones > 0) {
        Toast::showToast(this, "No puedes eliminar un motivo que ya fue usado en devoluciones", Toast::Warning);
        return;
    }
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