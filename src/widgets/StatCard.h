#pragma once

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include "Theme.h"

class StatCard : public QFrame {
    Q_OBJECT

public:
    explicit StatCard(const QString &title, const QString &value, const QString &subtext = "",
                      const QString &accentColor = Theme::Accent, QWidget *parent = nullptr);

    void setValue(const QString &value);
    void setSubtext(const QString &subtext);

private:
    QLabel *m_lblTitle;
    QLabel *m_lblValue;
    QLabel *m_lblSubtext;
};
