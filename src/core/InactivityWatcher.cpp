#include "InactivityWatcher.h"
#include "ApiClient.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QEvent>

namespace {
constexpr int kMinutosPorDefecto = 15;
constexpr int kIntervaloRevisionMs = 15 * 1000;

int minutosConfigurados() {
    bool ok = false;
    const int min = ApiClient::setting("INACTIVIDAD_MINUTOS").toInt(&ok);
    return (ok && min > 0) ? min : kMinutosPorDefecto;
}
}

InactivityWatcher::InactivityWatcher(QObject *parent)
    : QObject(parent)
    , m_timeoutMs(qint64(minutosConfigurados()) * 60 * 1000)
{
    m_checkTimer.setInterval(kIntervaloRevisionMs);
    connect(&m_checkTimer, &QTimer::timeout, this, &InactivityWatcher::check);

    auto *api = ApiClient::instance();
    connect(api, &ApiClient::userLoggedIn, this, &InactivityWatcher::start);
    connect(api, &ApiClient::userLoggedOut, this, &InactivityWatcher::stop);
    connect(api, &ApiClient::sessionExpired, this, &InactivityWatcher::stop);
}

void InactivityWatcher::start() {
    m_lastActivityMs = QDateTime::currentMSecsSinceEpoch();
    qApp->installEventFilter(this);   // instalarlo dos veces no lo duplica
    m_checkTimer.start();
}

void InactivityWatcher::stop() {
    m_checkTimer.stop();
    qApp->removeEventFilter(this);
}

bool InactivityWatcher::eventFilter(QObject *watched, QEvent *event) {
    switch (event->type()) {
        case QEvent::KeyPress:
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
        case QEvent::Wheel:
        case QEvent::TouchBegin:
            m_lastActivityMs = QDateTime::currentMSecsSinceEpoch();
            break;
        default:
            break;
    }
    return QObject::eventFilter(watched, event);   // solo observa, nunca consume
}

void InactivityWatcher::check() {
    if (QDateTime::currentMSecsSinceEpoch() - m_lastActivityMs < m_timeoutMs) return;
    // expireSession emite sessionExpired, que detiene este vigilante.
    ApiClient::instance()->expireSession("Tu sesión se cerró por inactividad.");
}
