#include "JefeGenerosView.h"
#include "Ui.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Toast.h"
#include <QLineEdit>
#include <QPointer>

JefeGenerosView::JefeGenerosView(QWidget *parent)
    : CatalogView("Jefatura · Gestión de géneros", "Catálogo y Gestión de Géneros",
                  "Administra los géneros literarios disponibles.",
                  "Buscar género...", "Registrar Nuevo Género",
                  {"Nombre de género", "Títulos asociados", "Acciones"}, 0, 0, parent)
{
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

            Ui::cellWidgets(m_table, r, 2, rowActions(g.genero_literario,
                [this, g]() { abrirFormulario(&g); }, [this, g]() { eliminar(g); }));
        }
        finishRows(QString("Total de géneros: %1").arg(m_generos.size()));
    });
}

void JefeGenerosView::abrirFormulario(const GeneroDto *g) {
    const bool edit = g != nullptr;
    Ui::FormDialog dlg(edit ? "Editar Género" : "Registrar Nuevo Género", edit ? "Actualizar Género" : "Registrar Género", this);
    auto *txt = new QLineEdit(edit ? g->genero_literario : QString(), &dlg);
    txt->setPlaceholderText("Ej: Romantasy");
    dlg.addField("Nombre del Género", txt, 0, 0, 2);

    QPointer<QDialog> guard(&dlg);
    connect(dlg.okButton(), &QPushButton::clicked, &dlg, [&, edit, g]() {
        const QString nombre = txt->text().trimmed();
        if (nombre.isEmpty()) {
            Toast::showToast(&dlg, "El nombre del género es obligatorio", Toast::Warning);
            return;
        }
        const QJsonObject req{{"genero_literario", nombre}};
        auto done = [this, guard, edit](bool ok, const QJsonValue &, const QString &msg) {
            if (!ok) {
                Toast::showToast(guard ? guard.data() : static_cast<QWidget *>(this), "No se pudo guardar el género: " + msg, Toast::Error);
                return;
            }
            Toast::showToast(this, edit ? "Género actualizado" : "Género registrado", Toast::Success);
            if (guard) guard->accept();
            cargarGeneros();
        };
        if (edit) ApiClient::instance()->actualizarGenero(g->id_genero, req, done);
        else ApiClient::instance()->crearGenero(req, done);
    });
    dlg.exec();
}

void JefeGenerosView::eliminar(const GeneroDto &g) {
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
