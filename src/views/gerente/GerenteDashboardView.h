#pragma once

#include <QWidget>
#include "Models.h"

class StatCard;

/**
 * Panel principal del Gerente / Subgerente.
 * Muestra métricas centradas en la gestión de usuarios (RF01).
 */
class GerenteDashboardView : public QWidget {
    Q_OBJECT

public:
    explicit GerenteDashboardView(QWidget *parent = nullptr);
    void refreshData();

private:
    StatCard *m_cardUsuarios;
    StatCard *m_cardActivos;
    StatCard *m_cardInactivos;
};
