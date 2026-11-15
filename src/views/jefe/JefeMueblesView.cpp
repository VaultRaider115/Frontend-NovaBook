#include "JefeMueblesView.h"
#include "Ui.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Toast.h"
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSpinBox>

JefeMueblesView::JefeMueblesView(QWidget *parent)
    : CatalogView("Jefatura · Manejo de muebles", "Catálogo y Gestión de Muebles y Estantería",
                  "Administra los exhibidores de piso y racks en bodega.",
                  "Buscar por código de mueble, área o género asignado...", "Registrar Nuevo Mueble",
                  {"Código", "Nombre del mueble", "Área", "Género asignado", "Capacidad estimada", "Estado", "Acciones"},
                  1, 8, parent)
{
}

void JefeMueblesView::cargarMuebles() {
    ApiClient::instance()->listarGeneros([this](bool ok, const QJsonValue &data, const QString &msg) {
        if (!ok || !data.isArray()) {
            Toast::showToast(this, "Error al cargar géneros para muebles: " + msg, Toast::Error);
            return;
        }
        m_generos.clear();
        const QJsonArray arr = data.toArray();
        for (const auto &v : arr) m_generos.append(GeneroDto::fromJson(v.toObject()));

        ApiClient::instance()->listarMuebles([this](bool mueblesOk, const QJsonValue &mueblesData, const QString &mueblesMsg) {
            if (!mueblesOk || !mueblesData.isArray()) {
                Toast::showToast(this, "Error al cargar muebles: " + mueblesMsg, Toast::Error);
                return;
            }
            m_muebles.clear();
            clearRows();
            const QJsonArray mueblesArr = mueblesData.toArray();
            for (const auto &v : mueblesArr) m_muebles.append(MuebleDto::fromJson(v.toObject()));

            for (const MuebleDto &m : std::as_const(m_muebles)) {
                const int r = appendRow(m.codigo + " " + m.nombre + " " + m.area + " " + m.genero_asignado);
                Ui::cellText(m_table, r, 0, m.codigo.isEmpty() ? QString::number(m.id_mueble) : m.codigo, true);
                Ui::cellText(m_table, r, 1, m.nombre);
                const bool piso = m.area.contains("piso", Qt::CaseInsensitive);
                Ui::cellWidget(m_table, r, 2, Ui::chip(m.area.isEmpty() ? "—" : m.area, piso ? "indigo" : "purple"));
                Ui::cellText(m_table, r, 3, m.genero_asignado.isEmpty() ? "—" : m.genero_asignado);
                if (m.capacidad_maxima > 0) {
                    // Diseño 18: "35 / 50 piezas" con barra de ocupación debajo.
                    auto *cap = new QWidget();
                    auto *cl = new QVBoxLayout(cap);
                    cl->setContentsMargins(0, 0, 0, 0);
                    cl->setSpacing(6);
                    cl->addWidget(new QLabel(QString("<b>%1</b> <span style=\"color:%3;\">/ %2 piezas</span>")
                                                 .arg(m.capacidad_actual).arg(m.capacidad_maxima).arg(Theme::TextMuted), cap));
                    auto *bar = new QProgressBar(cap);
                    bar->setObjectName("CapBar");
                    bar->setRange(0, m.capacidad_maxima);
                    bar->setValue(qMin(m.capacidad_actual, m.capacidad_maxima));
                    bar->setTextVisible(false);
                    bar->setFixedSize(130, 4);
                    cl->addWidget(bar);
                    Ui::cellWidget(m_table, r, 4, cap);
                } else {
                    Ui::cellText(m_table, r, 4, "—");
                }
                const bool llena = m.capacidad_maxima > 0 && m.capacidad_actual >= m.capacidad_maxima;
                Ui::cellWidget(m_table, r, 5, m.capacidad_maxima > 0
                               ? Ui::chip(llena ? "Capacidad Llena" : "Disponible", llena ? "amber" : "ok") : Ui::chip("—", "neutral"));

                const bool tieneStock = (m.capacidad_actual > 0);
                auto actions = rowActions(m.nombre,
                    [this, m]() { abrirFormulario(&m); },
                    [this, m]() { eliminar(m); });

                if (tieneStock && actions.size() > 1) {
                    actions.last()->setEnabled(false);
                    actions.last()->setVisible(false);
                }
                Ui::cellWidgets(m_table, r, 6, actions);
            }
            finishRows(QString("Total de Muebles: %1").arg(m_muebles.size()), "ubicaciones registradas");
        });
    });
}

void JefeMueblesView::abrirFormulario(const MuebleDto *m) {
    const bool edit = m != nullptr;
    const bool tieneStock = edit && m->capacidad_actual > 0;

    Ui::FormDialog dlg(edit ? "Editar Mueble" : "Registrar Nuevo Mueble", edit ? "Actualizar Mueble" : "Registrar Mueble", this);

    auto *txtCodigo = new QLineEdit(edit ? m->codigo : QString(), &dlg);
    txtCodigo->setPlaceholderText("Ej. M-12 o BO-9");
    txtCodigo->setMaxLength(10);
    txtCodigo->setValidator(new QRegularExpressionValidator(QRegularExpression("[A-Za-z0-9\\-]{0,20}"), txtCodigo));

    auto *txtNombre = new QLineEdit(edit ? m->nombre : QString(), &dlg);
    txtNombre->setPlaceholderText("Ej: Mueble 11 Autoayuda");
    txtNombre->setMaxLength(50);

    auto *comboArea = new QComboBox(&dlg);
    comboArea->addItem("Piso de Venta", 1);
    comboArea->addItem("Bodega", 2);
    if (edit) comboArea->setCurrentIndex(comboArea->findData(m->id_ubicacion));
    if (tieneStock) {
        comboArea->setEnabled(false);
        comboArea->setToolTip("No puedes cambiar el área de un mueble que tiene piezas almacenadas");
    }

    auto *comboGenero = new QComboBox(&dlg);
    for (const GeneroDto &g : std::as_const(m_generos)) comboGenero->addItem(g.genero_literario, g.id_genero);
    if (edit) comboGenero->setCurrentIndex(comboGenero->findData(m->id_genero));
    if (comboGenero->count() == 0) {
        Toast::showToast(this, "No hay géneros disponibles para asignar al mueble", Toast::Warning);
        return;
    }
    if (tieneStock) {
        comboGenero->setEnabled(false);
        comboGenero->setToolTip("No puedes cambiar el género de un mueble que tiene piezas almacenadas");
    }

    auto *spnCap = new QSpinBox(&dlg);
    const int minCap = tieneStock ? m->capacidad_actual : 1;
    spnCap->setRange(minCap, 99999);
    spnCap->setValue(edit && m->capacidad_maxima > 0 ? qMax(minCap, m->capacidad_maxima) : 50);

    auto *txtDesc = new QPlainTextEdit(edit ? m->descripcion : QString(), &dlg);
    txtDesc->setPlaceholderText("Ej. Junto al pasillo central frente al área de cajas (máx. 200 caracteres)...");
    txtDesc->setFixedHeight(80);

    dlg.addField("Código de mueble", txtCodigo, 0, 0);
    dlg.addField("Nombre de mueble", txtNombre, 0, 1);
    dlg.addField("Área", comboArea, 1, 0);
    dlg.addField("Género asignado", comboGenero, 1, 1);
    dlg.addField(tieneStock ? QString("Capacidad Máxima (mín. %1)").arg(minCap) : "Capacidad Máxima", spnCap, 2, 0, 2);
    dlg.addField("Descripción o Referencia de Ubicación", txtDesc, 3, 0, 2);

    QPointer<QDialog> guard(&dlg);
    QPointer<JefeMueblesView> self(this);
    connect(dlg.okButton(), &QPushButton::clicked, &dlg, [&, edit, m, guard, self, tieneStock]() {
        const QString codigo = txtCodigo->text().trimmed().toUpper();
        if (codigo.isEmpty()) {
            Toast::showToast(&dlg, "El código del mueble es obligatorio", Toast::Warning);
            txtCodigo->setFocus();
            return;
        }
        const QString nombre = txtNombre->text().trimmed();
        if (nombre.isEmpty()) {
            Toast::showToast(&dlg, "El nombre del mueble es obligatorio", Toast::Warning);
            txtNombre->setFocus();
            return;
        }
        if (tieneStock && spnCap->value() < m->capacidad_actual) {
            Toast::showToast(&dlg, QString("La capacidad no puede ser menor a las %1 piezas actuales").arg(m->capacidad_actual), Toast::Warning);
            spnCap->setFocus();
            return;
        }
        const QString desc = txtDesc->toPlainText().trimmed();
        if (desc.size() > 200) {
            Toast::showToast(&dlg, "La descripción no puede exceder 200 caracteres", Toast::Warning);
            txtDesc->setFocus();
            return;
        }

        for (const MuebleDto &item : std::as_const(m_muebles)) {
            if (edit && item.id_mueble == m->id_mueble) continue;
            if (item.codigo.trimmed().compare(codigo, Qt::CaseInsensitive) == 0) {
                Toast::showToast(&dlg, "Ya existe un mueble registrado con ese código", Toast::Warning);
                txtCodigo->setFocus();
                return;
            }
            if (item.nombre.trimmed().compare(nombre, Qt::CaseInsensitive) == 0) {
                Toast::showToast(&dlg, "Ya existe un mueble registrado con ese nombre", Toast::Warning);
                txtNombre->setFocus();
                return;
            }
        }

        dlg.okButton()->setEnabled(false);
        QJsonObject req{{"codigo_mueble", codigo}, {"nombre_mueble", nombre},
                        {"id_ubicacion", comboArea->currentData().toInt()}, {"id_genero", comboGenero->currentData().toInt()},
                        {"capacidad", spnCap->value()},
                        {"descripcion", desc}};

        auto done = [self, guard, edit](bool ok, const QJsonValue &, const QString &msg) {
            if (!ok) {
                if (guard) static_cast<Ui::FormDialog *>(guard.data())->okButton()->setEnabled(true);
                Toast::showToast(guard ? guard.data() : static_cast<QWidget *>(self.data()), "No se pudo guardar el mueble: " + msg, Toast::Error);
                return;
            }
            if (self) {
                Toast::showToast(self, edit ? "Mueble actualizado" : "Mueble registrado", Toast::Success);
                self->cargarMuebles();
            }
            if (guard) guard->accept();
        };
        if (edit) ApiClient::instance()->actualizarMueble(m->id_mueble, req, done);
        else ApiClient::instance()->crearMueble(req, done);
    });
    dlg.exec();
}

void JefeMueblesView::eliminar(const MuebleDto &m) {
    if (m.capacidad_actual > 0) {
        Toast::showToast(this, "No puedes eliminar un mueble que tiene piezas almacenadas", Toast::Warning);
        return;
    }
    if (!confirmDelete(QString("el mueble '%1'").arg(m.nombre))) return;
    ApiClient::instance()->eliminarMueble(m.id_mueble, [this, m](bool ok, const QJsonValue &, const QString &msg) {
        if (ok) {
            Toast::showToast(this, QString("Mueble '%1' eliminado").arg(m.nombre), Toast::Success);
            cargarMuebles();
        } else {
            Toast::showToast(this, "No se pudo eliminar el mueble: " + msg, Toast::Error);
        }
    });
}