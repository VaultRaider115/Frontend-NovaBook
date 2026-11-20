#include "BodegaAutoresView.h"
#include "Limites.h"
#include "Ui.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Toast.h"
#include <QAbstractItemView>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPointer>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

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
        {"canadiense", "CA"}, {"uruguayo", "UY"}, {"uruguaya", "UY"}, {"venezolano", "VE"},
        {"venezolana", "VE"}, {"cubano", "CU"}, {"cubana", "CU"}, {"ecuatoriano", "EC"},
        {"ecuatoriana", "EC"}, {"chino", "CN"}, {"china", "CN"}, {"coreano", "KR"},
        {"coreana", "KR"}, {"irlandés", "IE"}, {"irlandesa", "IE"}, {"sueco", "SE"},
        {"sueca", "SE"}, {"suizo", "CH"}, {"suiza", "CH"}, {"neerlandés", "NL"},
        {"neerlandesa", "NL"}, {"griego", "GR"}, {"griega", "GR"}, {"polaco", "PL"},
        {"polaca", "PL"}, {"boliviano", "BO"}, {"boliviana", "BO"}, {"paraguayo", "PY"},
        {"paraguaya", "PY"}, {"guatemalteco", "GT"}, {"guatemalteca", "GT"}, {"belga", "BE"},
        {"noruego", "NO"}, {"noruega", "NO"}, {"danés", "DK"}, {"danesa", "DK"},
        {"indio", "IN"}, {"india", "IN"}, {"puertorriqueño", "PR"}, {"costarricense", "CR"},
        {"panameño", "PA"}, {"dominicano", "DO"}, {"salvadoreño", "SV"}, {"hondureño", "HN"},
        {"nicaragüense", "NI"}, {"ucraniano", "UA"}, {"turco", "TR"}, {"egipcio", "EG"},
        {"israelí", "IL"}, {"escocés", "GB"}, {"otra", "GL"}
    };
    const QString key = nac.trimmed().toLower();
    return map.value(key, key.left(2).toUpper());
}

static QStringList listaNacionalidades() {
    return {
        "Alemán", "Argentino", "Austriaco", "Belga", "Boliviano",
        "Brasileño", "Británico", "Canadiense", "Chileno", "Chino",
        "Colombiano", "Coreano", "Costarricense", "Cubano", "Danés",
        "Dominicano", "Ecuatoriano", "Egipcio", "Escocés", "Español",
        "Estadounidense", "Francés", "Griego", "Guatemalteco", "Hondureño",
        "Indio", "Irlandés", "Israelí", "Italiano", "Japonés",
        "Mexicano", "Neerlandés", "Nicaragüense", "Noruego", "Panameño",
        "Paraguayo", "Peruano", "Polaco", "Portugués", "Puertorriqueño",
        "Ruso", "Salvadoreño", "Sueco", "Suizo", "Turco",
        "Ucraniano", "Uruguayo", "Venezolano", "Otra"
    };
}

BodegaAutoresView::BodegaAutoresView(const QString &eyebrow, QWidget *parent)
    : CatalogView(eyebrow, "Catálogo y Gestión de Autores",
                  "Administra los autores disponibles para el catálogo de libros de la tienda.",
                  "Buscar por nombre, apellido o nacionalidad...", "Registrar Nuevo Autor",
                  {"ID", "Autor", "Nacionalidad", "Títulos asociados", "Acciones"}, 1, 0, parent)
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

            const bool tieneLibros = (a.total_libros > 0);
            auto actions = rowActions(a.nombreCompleto(),
                [this, a]() { abrirFormulario(&a); },
                [this, a]() { eliminar(a); });

            if (tieneLibros && actions.size() > 1) {
                actions.last()->setEnabled(false);
                actions.last()->setVisible(false);
            }
            Ui::cellWidgets(m_table, r, 4, actions);
        }
        finishRows(QString("Total de autores: %1").arg(m_autores.size()));
    });
}

void BodegaAutoresView::abrirFormulario(const AutorDto *a) {
    const bool edit = a != nullptr;
    Ui::FormDialog dlg(edit ? "Editar Autor" : "Registrar Nuevo Autor", edit ? "Actualizar Autor" : "Guardar Autor", this);

    const QRegularExpression rxNombre("^[\\p{L} '\\-\\.]*$");

    auto *txtNombre = new QLineEdit(edit ? a->nombre : QString(), &dlg);
    txtNombre->setPlaceholderText("Ej: Carlos");
    txtNombre->setValidator(new QRegularExpressionValidator(rxNombre, txtNombre));

    auto *txtApellidos = new QLineEdit(edit ? a->apellidos : QString(), &dlg);
    txtApellidos->setPlaceholderText("Ej: Trejo");
    txtApellidos->setValidator(new QRegularExpressionValidator(rxNombre, txtApellidos));

    auto *comboNac = new QComboBox(&dlg);
    comboNac->setStyleSheet("QComboBox { combobox-popup: 0; }");
    comboNac->setMaxVisibleItems(10);
    comboNac->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    comboNac->addItem("Selecciona una nacionalidad...", "");
    for (const QString &nac : listaNacionalidades()) {
        if (nac == "Otra") continue;
        comboNac->addItem(nac, nac);
    }
    comboNac->addItem("Otra", "Otra");
    if (edit && !a->nacionalidad.trimmed().isEmpty()) {
        int idx = comboNac->findData(a->nacionalidad.trimmed(), Qt::MatchFixedString);
        if (idx < 0) {
            idx = comboNac->count() - 1;
            comboNac->insertItem(idx, a->nacionalidad.trimmed(), a->nacionalidad.trimmed());
        }
        comboNac->setCurrentIndex(idx);
    }

    dlg.addField("Nombre(s) *", Ui::conLimite(txtNombre, Limites::kAutorNombre, &dlg), 0, 0);
    dlg.addField("Apellido(s) *", Ui::conLimite(txtApellidos, Limites::kAutorApellidos, &dlg), 0, 1);
    dlg.addField("Nacionalidad *", comboNac, 1, 0, 2);

    QPointer<QDialog> guard(&dlg);
    QPointer<BodegaAutoresView> self(this);
    connect(dlg.okButton(), &QPushButton::clicked, &dlg, [&, edit, a, guard, self, rxNombre]() {
        const QString nom = txtNombre->text().trimmed();
        if (nom.isEmpty()) {
            Toast::showToast(&dlg, "El nombre es obligatorio", Toast::Warning);
            txtNombre->setFocus();
            return;
        }
        if (!rxNombre.match(nom).hasMatch()) {
            Toast::showToast(&dlg, "El nombre solo debe contener letras y espacios", Toast::Warning);
            txtNombre->setFocus();
            return;
        }

        const QString ape = txtApellidos->text().trimmed();
        if (ape.isEmpty()) {
            Toast::showToast(&dlg, "El apellido es obligatorio", Toast::Warning);
            txtApellidos->setFocus();
            return;
        }
        if (!rxNombre.match(ape).hasMatch()) {
            Toast::showToast(&dlg, "Los apellidos solo deben contener letras y espacios", Toast::Warning);
            txtApellidos->setFocus();
            return;
        }

        const QString nac = comboNac->currentData().toString().trimmed();
        if (nac.isEmpty()) {
            Toast::showToast(&dlg, "Selecciona una nacionalidad de la lista", Toast::Warning);
            comboNac->setFocus();
            return;
        }

        for (const AutorDto &item : std::as_const(m_autores)) {
            if (edit && item.id_autor == a->id_autor) continue;
            if (item.nombre.trimmed().compare(nom, Qt::CaseInsensitive) == 0 &&
                item.apellidos.trimmed().compare(ape, Qt::CaseInsensitive) == 0) {
                Toast::showToast(&dlg, "Ya existe un autor registrado con ese nombre y apellidos", Toast::Warning);
                txtNombre->setFocus();
                return;
            }
        }

        QJsonObject req{{"nombre", nom}, {"apellidos", ape}, {"nacionalidad", nac}};
        if (edit && !a->biografia.isEmpty()) req["biografia"] = a->biografia;

        dlg.okButton()->setEnabled(false);

        auto done = [self, guard, edit](bool ok, const QJsonValue &, const QString &msg) {
            if (!ok) {
                if (guard) static_cast<Ui::FormDialog *>(guard.data())->okButton()->setEnabled(true);
                Toast::showToast(guard ? guard.data() : static_cast<QWidget *>(self.data()), "No se pudo guardar el autor: " + msg, Toast::Error);
                return;
            }
            if (self) {
                Toast::showToast(self, edit ? "Autor actualizado exitosamente" : "Autor registrado exitosamente", Toast::Success);
                self->cargarAutores();
            }
            if (guard) guard->accept();
        };
        if (edit) ApiClient::instance()->actualizarAutor(a->id_autor, req, done);
        else ApiClient::instance()->crearAutor(req, done);
    });
    dlg.exec();
}

void BodegaAutoresView::eliminar(const AutorDto &a) {
    if (a.total_libros > 0) {
        Toast::showToast(this, "No puedes eliminar un autor que tiene libros asociados", Toast::Warning);
        return;
    }
    if (!confirmDelete(QString("al autor '%1'").arg(a.nombreCompleto()))) return;
    ApiClient::instance()->eliminarAutor(a.id_autor, [this, a](bool ok, const QJsonValue &, const QString &msg) {
        if (ok) {
            Toast::showToast(this, QString("Autor '%1' eliminado").arg(a.nombreCompleto()), Toast::Success);
            cargarAutores();
        } else {
            Toast::showToast(this, "No se pudo eliminar el autor: " + msg, Toast::Error);
        }
    });
}