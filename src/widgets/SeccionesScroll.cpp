#include "SeccionesScroll.h"
#include <QTimer>
#include "Ui.h"
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollBar>

SeccionesScroll::SeccionesScroll(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    // Barra de anclas fija (fuera del área con scroll).
    m_barra = new QHBoxLayout();
    m_barra->setSpacing(6);
    m_barra->addStretch();
    layout->addLayout(m_barra);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *contenido = new QWidget(m_scroll);
    m_contenido = new QVBoxLayout(contenido);
    m_contenido->setContentsMargins(0, 0, 8, 0);   // espacio para la barra de scroll
    m_contenido->setSpacing(28);
    m_contenido->addStretch();
    m_scroll->setWidget(contenido);
    layout->addWidget(m_scroll, 1);

    connect(m_scroll->verticalScrollBar(), &QScrollBar::valueChanged, this, &SeccionesScroll::marcarActual);
    // Al cambiar de tamaño cambian las posiciones de las secciones.
    contenido->installEventFilter(this);
}

QWidget *SeccionesScroll::agregarSeccion(const QString &titulo) {
    auto *seccion = new QWidget();
    auto *sl = new QVBoxLayout(seccion);
    sl->setContentsMargins(0, 0, 0, 0);
    sl->setSpacing(12);
    auto *lbl = new QLabel(titulo, seccion);
    lbl->setStyleSheet("font-size: 18px; font-weight: 700;");
    sl->addWidget(lbl);
    auto *cuerpo = new QWidget(seccion);
    sl->addWidget(cuerpo);
    // Antes del stretch final, para que las secciones queden arriba.
    m_contenido->insertWidget(m_contenido->count() - 1, seccion);

    const int idx = static_cast<int>(m_secciones.size());
    m_secciones << seccion;
    auto *ancla = Ui::button(titulo, "page", this);
    ancla->setToolTip(QString("Ir a %1").arg(titulo));
    connect(ancla, &QPushButton::clicked, this, [this, idx]() { irA(idx); });
    m_anclas << ancla;
    m_barra->insertWidget(m_barra->count() - 1, ancla);
    marcarActual();
    return cuerpo;
}

void SeccionesScroll::irA(int i) {
    if (i < 0 || i >= m_secciones.size()) return;
    m_scroll->verticalScrollBar()->setValue(m_secciones.at(i)->y());
    // Si la sección acaba de mostrar contenido (p. ej. el detalle que se abrió),
    // el alto del scroll se recalcula después: se repite al terminar el layout.
    QTimer::singleShot(0, this, [this, i]() {
        m_scroll->verticalScrollBar()->setValue(m_secciones.at(i)->y());
        for (int k = 0; k < m_anclas.size(); ++k) Ui::setCurrent(m_anclas.at(k), k == i);
    });
    // Si la última sección es corta el scroll no llega a su inicio: se marca igual.
    for (int k = 0; k < m_anclas.size(); ++k) Ui::setCurrent(m_anclas.at(k), k == i);
}

void SeccionesScroll::marcarActual() {
    if (m_secciones.isEmpty()) return;
    const int y = m_scroll->verticalScrollBar()->value();
    const bool alFinal = y > 0 && y >= m_scroll->verticalScrollBar()->maximum();
    int actual = 0;
    for (int k = 0; k < m_secciones.size(); ++k)
        if (m_secciones.at(k)->y() <= y + 40) actual = k;
    if (alFinal) actual = static_cast<int>(m_secciones.size()) - 1;
    for (int k = 0; k < m_anclas.size(); ++k) Ui::setCurrent(m_anclas.at(k), k == actual);
}

bool SeccionesScroll::eventFilter(QObject *obj, QEvent *ev) {
    if (ev->type() == QEvent::Resize) marcarActual();
    return QWidget::eventFilter(obj, ev);
}
