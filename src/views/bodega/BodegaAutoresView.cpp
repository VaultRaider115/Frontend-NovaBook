#include "BodegaAutoresView.h"
#include "Ui.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Toast.h"
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPointer>

// Gentilicio → código de país para la etiqueta de nacionalidad del diseño.
static QString codigoPais(const QString &nac) {
    static const QHash<QString, QString> map = {
        {"mexicano", "MX"}, {"mexicana", "MX"}, {"colombiano", "CO"}, {"colombiana", "CO"},
        {"español", "ES"}, {"española", "ES"}, {"alemán", "DE"}, {"alemana", "DE"},
        {"francés", "FR"}, {"francesa", "FR"}, {"austriaco", "AT"}, {"austriaca", "AT"},
        {"argentino", "AR"}, {"argentina", "AR"}, {"estadounidense", "US"}, {"británico", "GB"},
        {"británica", "GB"}, {"inglés", "GB"}, {"inglesa", "GB"}, {"italiano", "IT"}, {"italiana", "IT"},
        {"chileno", "CL"}, {"chilena", "CL"}, {"peruano", "PE"}, {"peruana", "PE"},
        {"brasileño", "BR"}, {"brasileña", "BR"}, {"japonés", "JP"}, {"japonesa", "JP"},
        {"ruso", "RU"}, {"rusa", "RU"}, {"portugués", "PT"}, {"portuguesa", "PT"},
    };
    const QString key = nac.trimmed().toLower();
    return map.value(key, key.left(2).toUpper());
}

BodegaAutoresView::BodegaAutoresView(const QString &eyebrow, QWidget *parent)
    : CatalogView(eyebrow, "Catálogo y Gestión de Autores",
                  "Administra los autores disponibles para el catálogo de libros de la tienda.",
                  "Buscar por nombre, apellido o nacionalidad...", "Registrar Nuevo Autor",
                  {"ID", "Autor", "Nacionalidad", "Títulos asociados", "Estado", "Acciones"}, 1, 0, parent)
{
}

void BodegaAutoresView::cargarAutores() {
    ApiClient::instance()->listarAutores([this](bool ok, const QJsonValue &data, const QString &msg) {
        if (!ok || !data.isArray()) {
            Toast::showToast(this, "Error al cargar autores: " + msg, Toast::Error);
            return;
        }
        m_autores.clear();
        clearRows();
        const QJsonArray arr = data.toArray();
        for (const auto &v : arr) m_autores.append(AutorDto::fromJson(v.toObject()));

        for (const AutorDto &a : std::as_const(m_autores)) {
            const int r = appendRow(a.nombreCompleto() + " " + a.nacionalidad);
            Ui::cellText(m_table, r, 0, QString("%1").arg(a.id_autor, 3, 10, QChar('0')), true);
            m_table->setCellWidget(r, 1, nameCell(a.nombreCompleto()));

            if (a.nacionalidad.isEmpty()) {
                Ui::cellText(m_table, r, 2, "—", false, false, Theme::TextMuted);
            } else {
                auto *tag = Ui::chip(QString("%1  %2").arg(codigoPais(a.nacionalidad), a.nacionalidad), "neutral");
                Ui::cellWidget(m_table, r, 2, tag);
            }

            Ui::cellText(m_table, r, 3, a.total_libros < 0 ? "—"
                         : QString("%1 %2").arg(a.total_libros).arg(a.total_libros == 1 ? "título" : "títulos"),
                         false, false, Theme::TextMuted);

            if (a.estatus.isEmpty())
                Ui::cellText(m_table, r, 4, "—", false, false, Theme::TextMuted);
            else
                Ui::cellWidget(m_table, r, 4, Ui::chip(a.estatus, a.estatus.toLower() == "activo" ? "ok" : "neutral"));

            Ui::cellWidgets(m_table, r, 5, rowActions(a.nombreCompleto(),
                [this, a]() { abrirFormulario(&a); }, [this, a]() { eliminar(a); }));
        }
        finishRows(QString("Total de autores: %1").arg(m_autores.size()));
    });
}

void BodegaAutoresView::abrirFormulario(const AutorDto *a) {
    const bool edit = a != nullptr;
    Ui::FormDialog dlg(edit ? "Editar Autor" : "Registrar Nuevo Autor", edit ? "Actualizar Autor" : "Guardar Autor", this);
    auto *txtNombre = new QLineEdit(edit ? a->nombre : QString(), &dlg);
    txtNombre->setPlaceholderText("Ej: Carlos");
    auto *txtApellidos = new QLineEdit(edit ? a->apellidos : QString(), &dlg);
    txtApellidos->setPlaceholderText("Ej: Trejo");
    auto *txtNac = new QLineEdit(edit ? a->nacionalidad : QString(), &dlg);
    txtNac->setPlaceholderText("Ej. Mexicano, Colombiano, Español…");
    dlg.addField("Nombre(s) *", txtNombre, 0, 0);
    dlg.addField("Apellido(s) *", txtApellidos, 0, 1);
    dlg.addField("Nacionalidad *", txtNac, 1, 0, 2);

    QPointer<QDialog> guard(&dlg);
    connect(dlg.okButton(), &QPushButton::clicked, &dlg, [&, edit, a]() {
        const QString nom = txtNombre->text().trimmed(), ape = txtApellidos->text().trimmed();
        const QString nac = txtNac->text().trimmed();
        if (nom.isEmpty() || ape.isEmpty() || nac.isEmpty()) {
            Toast::showToast(&dlg, "Nombre, apellidos y nacionalidad son obligatorios", Toast::Warning);
            return;
        }
        QJsonObject req{{"nombre", nom}, {"apellidos", ape}, {"nacionalidad", nac}};
        if (edit && !a->biografia.isEmpty()) req["biografia"] = a->biografia; // el formulario no la edita; se conserva
        auto done = [this, guard, edit](bool ok, const QJsonValue &, const QString &msg) {
            if (!ok) {
                Toast::showToast(guard ? guard.data() : static_cast<QWidget *>(this), "No se pudo guardar el autor: " + msg, Toast::Error);
                return;
            }
            Toast::showToast(this, edit ? "Autor actualizado exitosamente" : "Autor registrado exitosamente", Toast::Success);
            if (guard) guard->accept();
            cargarAutores();
        };
        if (edit) ApiClient::instance()->actualizarAutor(a->id_autor, req, done);
        else ApiClient::instance()->crearAutor(req, done);
    });
    dlg.exec();
}

void BodegaAutoresView::eliminar(const AutorDto &a) {
    if (!confirmDelete(QString("al autor '%1' (fallará si tiene libros asociados)").arg(a.nombreCompleto()))) return;
    ApiClient::instance()->eliminarAutor(a.id_autor, [this, a](bool ok, const QJsonValue &, const QString &msg) {
        if (ok) {
            Toast::showToast(this, QString("Autor '%1' eliminado").arg(a.nombreCompleto()), Toast::Success);
            cargarAutores();
        } else {
            Toast::showToast(this, "No se pudo eliminar el autor: " + msg, Toast::Error);
        }
    });
}
