#pragma once

#include <QObject>
#include <QTimer>

// Cierra la sesión cuando no hay actividad del usuario (teclado, mouse, toques)
// durante el tiempo configurado en INACTIVIDAD_MINUTOS (por defecto 15).
//
// Usa la hora del reloj y no la cuenta regresiva de un QTimer, para que el
// tiempo con el equipo suspendido también cuente.
class InactivityWatcher : public QObject {
    Q_OBJECT

public:
    explicit InactivityWatcher(QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void start();
    void stop();
    void check();

    QTimer m_checkTimer;
    qint64 m_lastActivityMs = 0;
    qint64 m_timeoutMs = 0;
};
