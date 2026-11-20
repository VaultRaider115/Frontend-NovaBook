#include "JefeGenerosView.h"
#include "Limites.h"
#include "Ui.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Toast.h"
#include <QLineEdit>
#include <QPointer>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

JefeGenerosView::JefeGenerosView(QWidget *parent)
    : CatalogView("Jefatura · Gestión de géneros", "Catálogo y Gestión de Géneros",
                  "Administra los géneros literarios disponibles.",
                  "Buscar género...", "Registrar Nuevo Género",
                  {"Nombre de género", "Títulos asociados", "Acciones"}, 0, 0, parent)
{
    m_search->setValidator(new QRegularExpressionValidator(QRegularExpression("^[^\\p{N}]*$"), m_search));
}

void JefeGenerosView::cargarGeneros() {
    ApiClient::instance()->listarGeneros([this](bool ok, const QJsonValue &data, const QString &msg) {
        if (!ok || !data.isArray()) {
            Toast::showToast(this, "Error al cargar géneros: " + msg, Toast::Error);
            return;
        }
        m_generos.clear();
        clearRows();
        const QJsonArray arr = data.toArray();
        for (const auto &v : arr) m_generos.append(GeneroDto::fromJson(v.toObject()));

        for (const GeneroDto &g : std::as_const(m_generos)) {
            const int r = appendRow(g.genero_literario);
            m_table->setCellWidget(r, 0, nameCell(g.genero_literario));

            QStringList partes;
            if (g.total_libros >= 0) partes << QString("%1 %2").arg(g.total_libros).arg(g.total_libros == 1 ? "libro" : "libros");
            if (g.total_revistas >= 0) partes << QString("%1 %2").arg(g.total_revistas).arg(g.total_revistas == 1 ? "revista" : "revistas");
            Ui::cellText(m_table, r, 1, partes.isEmpty() ? "—" : partes.join(" · "), false, false, Theme::TextMuted);

            const bool enUso = (g.total_libros > 0) || (g.total_revistas > 0);
            auto actions = rowActions(g.genero_literario,
                [this, g]() { abrirFormulario(&g); },
                [this, g]() { eliminar(g); });

            if (enUso && actions.size() > 1) {
                actions.last()->setEnabled(false);
                actions.last()->setVisible(false);
            }
            Ui::cellWidgets(m_table, r, 2, actions);
        }
        finishRows(QString("Total de géneros: %1").arg(m_generos.size()));
    });
}

void JefeGenerosView::abrirFormulario(const GeneroDto *g) {
    const bool edit = g != nullptr;
    Ui::FormDialog dlg(edit ? "Editar Género" : "Registrar Nuevo Género", edit ? "Actualizar Género" : "Registrar Género", this);

    const QRegularExpression rxGenero("^[\\p{L} '\\-\\.]*$");
    auto *txt = new QLineEdit(edit ? g->genero_literario : QString(), &dlg);
    txt->setPlaceholderText("Ej: Romantasy");
    txt->setValidator(new QRegularExpressionValidator(rxGenero, txt));
    dlg.addField("Nombre del Género", Ui::conLimite(txt, Limites::kGenero, &dlg), 0, 0, 2);

    QPointer<QDialog> guard(&dlg);
    QPointer<JefeGenerosView> self(this);
    connect(dlg.okButton(), &QPushButton::clicked, &dlg, [&, edit, g, guard, self, rxGenero]() {
        const QString nombre = txt->text().trimmed();
        if (nombre.isEmpty()) {
            Toast::showToast(&dlg, "El nombre del género es obligatorio", Toast::Warning);
            txt->setFocus();
            return;
        }
        if (!rxGenero.match(nombre).hasMatch()) {
            Toast::showToast(&dlg, "El género solo debe contener letras y espacios", Toast::Warning);
            txt->setFocus();
            return;
        }

        for (const GeneroDto &item : std::as_const(m_generos)) {
            if (edit && item.id_genero == g->id_genero) continue;
            if (item.genero_literario.trimmed().compare(nombre, Qt::CaseInsensitive) == 0) {
                Toast::showToast(&dlg, "Ya existe un género registrado con ese nombre", Toast::Warning);
                txt->setFocus();
                return;
            }
        }

        dlg.okButton()->setEnabled(false);
        const QJsonObject req{{"genero_literario", nombre}};
        auto done = [self, guard, edit](bool ok, const QJsonValue &, const QString &msg) {
            if (!ok) {
                if (guard) static_cast<Ui::FormDialog *>(guard.data())->okButton()->setEnabled(true);
                Toast::showToast(guard ? guard.data() : static_cast<QWidget *>(self.data()), "No se pudo guardar el género: " + msg, Toast::Error);
                return;
            }
            if (self) {
                Toast::showToast(self, edit ? "Género actualizado" : "Género registrado", Toast::Success);
                self->cargarGeneros();
            }
            if (guard) guard->accept();
        };
        if (edit) ApiClient::instance()->actualizarGenero(g->id_genero, req, done);
        else ApiClient::instance()->crearGenero(req, done);
    });
    dlg.exec();
}

void JefeGenerosView::eliminar(const GeneroDto &g) {
    if (g.total_libros > 0 || g.total_revistas > 0) {
        Toast::showToast(this, "No puedes eliminar un género que tiene títulos asociados", Toast::Warning);
        return;
    }
    if (!confirmDelete(QString("el género '%1'").arg(g.genero_literario))) return;
    ApiClient::instance()->eliminarGenero(g.id_genero, [this, g](bool ok, const QJsonValue &, const QString &msg) {
        if (ok) {
            Toast::showToast(this, QString("Género '%1' eliminado").arg(g.genero_literario), Toast::Success);
            cargarGeneros();
        } else {
            Toast::showToast(this, "No se pudo eliminar el género: " + msg, Toast::Error);
        }
    });
}