#include "JefeDashboardView.h"
#include "StatCard.h"
#include "Theme.h"
#include "Ui.h"
#include "ApiClient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>

JefeDashboardView::JefeDashboardView(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 28, 32, 28);
    layout->setSpacing(24);

    // Header
    layout->addWidget(Ui::pageHeader("Jefatura · Panel", "Panel de Control — Jefe de Departamento", "Supervisión global de operaciones, autorizaciones y usuarios de Librería NovaBook", this));


    // Metrics Row
    auto *cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(16);

    m_cardMovimientos = new StatCard("Movimientos Diarios", "...", "Eventos registrados hoy", Theme::Role1, this);
    m_cardDevoluciones = new StatCard("Devoluciones", "...", "En espera de autorización", Theme::Warning, this);
    m_cardUsuarios = new StatCard("Usuarios Activos", "...", "Personal registrado", Theme::Role3, this);

    cardsLayout->addWidget(m_cardMovimientos);
    cardsLayout->addWidget(m_cardDevoluciones);
    cardsLayout->addWidget(m_cardUsuarios);
    layout->addLayout(cardsLayout);

    // Information Banner / Summary Card
    auto *infoCard = Ui::card(this);

    auto *infoLayout = new QVBoxLayout(infoCard);
    infoLayout->setSpacing(12);

    auto *infoTitle = new QLabel("Módulos Principales", infoCard);
    infoTitle->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;").arg(Theme::Accent));

    auto *infoDesc = new QLabel(
        "• <b>Historial del Día:</b> Consulta la bitácora completa de movimientos filtrada por fecha.<br>"
        "• <b>Devoluciones:</b> Evalúa las solicitudes de devolución de proveedores generadas por vendedores.<br>"
        "• <b>Gestión de Usuarios:</b> Crea, actualiza y da de baja usuarios y credenciales del sistema.",
        infoCard
    );
    infoDesc->setStyleSheet(QString("color: %1; font-size: 13px; line-height: 1.6;").arg(Theme::TextPrimary));

    infoLayout->addWidget(infoTitle);
    infoLayout->addWidget(infoDesc);
    layout->addWidget(infoCard);

    layout->addStretch();
}

void JefeDashboardView::refreshData() {
    // 1. Movimientos
    ApiClient::instance()->consultarMovimientoDiario("", [this](bool ok, const QJsonValue &data, const QString &) {
        if (ok && data.isObject()) {
            ResumenMovimientoDiario res = ResumenMovimientoDiario::fromJson(data.toObject());
            m_cardMovimientos->setValue(QString::number(res.total_eventos));
        } else {
            m_cardMovimientos->setValue("0");
        }
    });

    // 2. Devoluciones pendientes
    ApiClient::instance()->consultarHistorialDevoluciones("", "Pendiente", 0, [this](bool ok, const QJsonValue &data, const QString &) {
        if (ok && data.isArray()) {
            m_cardDevoluciones->setValue(QString::number(data.toArray().size()));
        } else {
            m_cardDevoluciones->setValue("0");
        }
    });

    // 3. Usuarios
    ApiClient::instance()->listarUsuarios([this](bool ok, const QJsonValue &data, const QString &) {
        if (ok && data.isArray()) {
            m_cardUsuarios->setValue(QString::number(data.toArray().size()));
        } else {
            m_cardUsuarios->setValue("0");
        }
    });
}
