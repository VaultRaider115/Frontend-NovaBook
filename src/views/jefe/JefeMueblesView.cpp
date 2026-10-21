#include "JefeMueblesView.h"
#include "Ui.h"
#include "Theme.h"
#include "ApiClient.h"
#include "Toast.h"
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
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
    ApiClient::instance()->listarGeneros([this](bool ok, const QJsonValue &data, const QString &) {
        if (!ok || !data.isArray()) return;
        m_generos.clear();
        const QJsonArray arr = data.toArray();
        for (const auto &v : arr) m_generos.append(GeneroDto::fromJson(v.toObject()));
    });

    ApiClient::instance()->listarMuebles([this](bool ok, const QJsonValue &data, const QString &msg) {
        if (!ok || !data.isArray()) {
            Toast::showToast(this, "Error al cargar muebles: " + msg, Toast::Error);
            return;
        }
        m_muebles.clear();
        clearRows();
        const QJsonArray arr = data.toArray();
        for (const auto &v : arr) m_muebles.append(MuebleDto::fromJson(v.toObject()));

        for (const MuebleDto &m : std::as_const(m_muebles)) {
            const int r = appendRow(m.codigo + " " + m.nombre + " " + m.area + " " + m.genero_asignado);
            Ui::cellText(m_table, r, 0, m.codigo.isEmpty() ? QString::number(m.id_mueble) : m.codigo, true);
            Ui::cellText(m_table, r, 1, m.nombre);
            const bool piso = m.area.contains("piso", Qt::CaseInsensitive);
            Ui::cellWidget(m_table, r, 2, Ui::chip(m.area.isEmpty() ? "—" : m.area, piso ? "indigo" : "amber"));
            Ui::cellText(m_table, r, 3, m.genero_asignado.isEmpty() ? "—" : m.genero_asignado);
            Ui::cellText(m_table, r, 4, m.capacidad_maxima > 0
                         ? QString("%1 / %2 piezas").arg(m.capacidad_actual).arg(m.capacidad_maxima) : "—");
            const bool llena = m.capacidad_maxima > 0 && m.capacidad_actual >= m.capacidad_maxima;
            Ui::cellWidget(m_table, r, 5, m.capacidad_maxima > 0
                           ? Ui::chip(llena ? "Capacidad Llena" : "Disponible", llena ? "danger" : "ok") : Ui::chip("—", "neutral"));
            Ui::cellWidgets(m_table, r, 6, rowActions(m.nombre,
                [this, m]() { abrirFormulario(&m); }, [this, m]() { eliminar(m); }));
        }
        finishRows(QString("Total de Muebles: %1").arg(m_muebles.size()), "ubicaciones registradas");
    });
}

void JefeMueblesView::abrirFormulario(const MuebleDto *m) {
    const bool edit = m != nullptr;
    Ui::FormDialog dlg(edit ? "Editar Mueble" : "Registrar Nuevo Mueble", edit ? "Actualizar Mueble" : "Registrar Mueble", this);

    auto *txtCodigo = new QLineEdit(edit ? m->codigo : QString(), &dlg);
    txtCodigo->setPlaceholderText("Ej. M-12 o BO-9");
    auto *txtNombre = new QLineEdit(edit ? m->nombre : QString(), &dlg);
    txtNombre->setPlaceholderText("Ej: Mueble 11 Autoayuda");

    auto *comboArea = new QComboBox(&dlg);
    comboArea->addItems({"Piso de Venta", "Bodega"});
    if (edit) comboArea->setCurrentText(m->area);

    auto *comboGenero = new QComboBox(&dlg);
    comboGenero->addItem("Sin género asignado", 0);
    for (const GeneroDto &g : std::as_const(m_generos)) comboGenero->addItem(g.genero_literario, g.id_genero);
    if (edit) comboGenero->setCurrentText(m->genero_asignado);

    auto *spnCap = new QSpinBox(&dlg);
    spnCap->setRange(1, 99999);
    spnCap->setValue(edit && m->capacidad_maxima > 0 ? m->capacidad_maxima : 50);

    auto *txtDesc = new QPlainTextEdit(edit ? m->descripcion : QString(), &dlg);
    txtDesc->setPlaceholderText("Ej. Junto al pasillo central frente al área de cajas...");
    txtDesc->setFixedHeight(80);

    dlg.addField("Código de mueble", txtCodigo, 0, 0);
    dlg.addField("Nombre de mueble", txtNombre, 0, 1);
    dlg.addField("Área", comboArea, 1, 0);
    dlg.addField("Género asignado", comboGenero, 1, 1);
    dlg.addField("Capacidad Máxima", spnCap, 2, 0, 2);
    dlg.addField("Descripción o Referencia de Ubicación", txtDesc, 3, 0, 2);

    QPointer<QDialog> guard(&dlg);
    connect(dlg.okButton(), &QPushButton::clicked, &dlg, [&, edit, m]() {
        if (txtCodigo->text().trimmed().isEmpty() || txtNombre->text().trimmed().isEmpty()) {
            Toast::showToast(&dlg, "Código y nombre del mueble son obligatorios", Toast::Warning);
            return;
        }
        QJsonObject req{{"codigo", txtCodigo->text().trimmed()}, {"nombre_mueble", txtNombre->text().trimmed()},
                        {"area", comboArea->currentText()}, {"capacidad_maxima", spnCap->value()},
                        {"descripcion", txtDesc->toPlainText().trimmed()}};
        if (comboGenero->currentData().toInt() > 0) req["id_genero"] = comboGenero->currentData().toInt();
        auto done = [this, guard, edit](bool ok, const QJsonValue &, const QString &msg) {
            if (!ok) {
                Toast::showToast(guard ? guard.data() : static_cast<QWidget *>(this), "No se pudo guardar el mueble: " + msg, Toast::Error);
                return;
            }
            Toast::showToast(this, edit ? "Mueble actualizado" : "Mueble registrado", Toast::Success);
            if (guard) guard->accept();
            cargarMuebles();
        };
        if (edit) ApiClient::instance()->actualizarMueble(m->id_mueble, req, done);
        else ApiClient::instance()->crearMueble(req, done);
    });
    dlg.exec();
}

void JefeMueblesView::eliminar(const MuebleDto &m) {
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
