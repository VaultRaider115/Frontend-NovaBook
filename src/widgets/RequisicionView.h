#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <functional>
#include "ApiClient.h"

// Pantalla de requisición por lista (Bodega ➔ Piso y Piso ➔ Bodega). Las subclases definen
// los textos y la llamada a la API para trasladar una partida.
class RequisicionView : public QWidget {
    Q_OBJECT
public:
    RequisicionView(const QString &eyebrow, const QString &title, const QString &sendText,
                    const QString &addedMessage, QWidget *parent = nullptr);

protected:
    virtual void trasladar(qint64 ean, const QString &tipo, int cantidad, ApiClient::ApiCallback cb) = 0;

private:
    struct Partida { qint64 ean; QString tipo; QString nombre; int cantidad; };
    void agregar();
    void render();
    void enviarTodo();

    QLineEdit *m_txtEan;
    QLabel *m_lblStatus;
    QSpinBox *m_spnCantidad;
    QTableWidget *m_table;
    QLabel *m_lblTotal;
    QPushButton *m_btnEnviar;
    QList<Partida> m_items;
    QString m_okMessage;
};
