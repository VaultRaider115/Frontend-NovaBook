#pragma once

#include <QList>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

// Vista de una sola página con scroll vertical: las secciones van una debajo de
// otra, cada una con su título, y arriba queda fija una barra de anclas para
// saltar a cada sección sin perder el scroll. La barra marca la sección que se
// está viendo. Sustituye a las "pestañas" (botones que cambiaban de página).
//
// Uso:
//   auto *secc = new SeccionesScroll(this);
//   layout->addWidget(secc, 1);
//   QWidget *a = secc->agregarSeccion("Inventarios");   // se llena con su propio layout
class SeccionesScroll : public QWidget {
    Q_OBJECT
public:
    explicit SeccionesScroll(QWidget *parent = nullptr);

    // Agrega una sección con su título y devuelve el contenedor vacío para llenarla.
    QWidget *agregarSeccion(const QString &titulo);
    // Lleva el scroll al inicio de la sección `i` (0 = la primera).
    void irA(int i);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    void marcarActual();

    QHBoxLayout *m_barra;
    QScrollArea *m_scroll;
    QVBoxLayout *m_contenido;
    QList<QWidget *> m_secciones;
    QList<QPushButton *> m_anclas;
};
