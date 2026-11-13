#include "GerenteDashboardView.h"
#include "StatCard.h"
#include "Theme.h"
#include "Ui.h"
#include "ApiClient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

GerenteDashboardView::GerenteDashboardView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 28, 32, 28);
    layout->setSpacing(24);

    // Header
    layout->addWidget(Ui::pageHeader("Gerencia · Panel",
                                     "Panel de Control",
                                     "Administración integral de usuarios del sistema NovaBook",
                                     this));

    // Metrics Row
    auto *cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(16);

    m_cardUsuarios = new StatCard("Total de Usuarios", "...", "Personal registrado en el sistema", Theme::Role4, this);
    m_cardActivos  = new StatCard("Usuarios Activos", "...", "Personal habilitado", Theme::Success, this);
    m_cardInactivos = new StatCard("Usuarios Inactivos", "...", "Personal dado de baja", Theme::TextMuted, this);

    cardsLayout->addWidget(m_cardUsuarios);
    cardsLayout->addWidget(m_cardActivos);
    cardsLayout->addWidget(m_cardInactivos);
    layout->addLayout(cardsLayout);

    // Information Banner
    auto *infoCard = Ui::card(this);
    auto *infoLayout = new QVBoxLayout(infoCard);
    infoLayout->setSpacing(12);

    auto *infoTitle = new QLabel("Módulo del Gerente", infoCard);
    infoTitle->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;").arg(Theme::Accent));

    auto *infoDesc = new QLabel(
        "• <b>Gestión de Usuarios:</b> Crea, consulta, modifica y elimina cuentas de empleados del sistema.<br>"
        "• <b>Control de Acceso:</b> Asigna roles (Vendedor, Personal de Bodega, Jefe de Departamento, Gerente) y gestiona credenciales.<br>"
        "• <b>Alta y Baja:</b> Registra nuevos colaboradores o da de baja a los existentes.",
        infoCard
    );
    infoDesc->setStyleSheet(QString("color: %1; font-size: 13px; line-height: 1.6;").arg(Theme::TextPrimary));
    infoDesc->setWordWrap(true);

    infoLayout->addWidget(infoTitle);
    infoLayout->addWidget(infoDesc);
    layout->addWidget(infoCard);

    layout->addStretch();
}

void GerenteDashboardView::refreshData() {
    ApiClient::instance()->listarUsuarios([this](bool ok, const QJsonValue &data, const QString &) {
        if (ok && data.isArray()) {
            const QJsonArray arr = data.toArray();
            int total = arr.size();
            int activos = 0;
            for (const auto &v : arr) {
                if (v.toObject().value("estatus").toBool(true))
                    ++activos;
            }
            m_cardUsuarios->setValue(QString::number(total));
            m_cardActivos->setValue(QString::number(activos));
            m_cardInactivos->setValue(QString::number(total - activos));
        } else {
            m_cardUsuarios->setValue("0");
            m_cardActivos->setValue("0");
            m_cardInactivos->setValue("0");
        }
    });
}
