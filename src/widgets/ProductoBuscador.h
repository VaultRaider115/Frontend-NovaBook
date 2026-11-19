#pragma once

#include <QJsonObject>
#include <QList>
#include <QLineEdit>
#include <QObject>
#include <QPointer>
#include <functional>

class QCompleter;
class QStandardItemModel;
class QTimer;

// Convierte un campo de EAN en un buscador de productos: acepta el EAN-13
// (escaneado o tecleado) o parte del título. Con el título sugiere los productos
// que coinciden (título, EAN, editorial y stock) y al elegir uno deja su EAN en
// el campo, así la vista sigue trabajando con el EAN como antes.
//
// Enter con un título: si solo hay un resultado lo elige y dispara
// returnPressed (la vista lo agrega); si hay varios, abre la lista.
class ProductoBuscador : public QObject {
    Q_OBJECT
public:
    using Filtro = std::function<bool(const QJsonObject &producto)>;

    // `filtro` (opcional) descarta productos de la lista, p. ej. los de otra editorial.
    explicit ProductoBuscador(QLineEdit *edit, Filtro filtro = {});

    static bool esEanCompleto(const QString &texto);   // 13 dígitos
    static bool esBusquedaPorTitulo(const QString &texto);   // tiene algo que no es dígito

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    void buscar(bool alPresionarEnter);
    void mostrar(const QList<QJsonObject> &productos);

    QPointer<QLineEdit> m_edit;
    Filtro m_filtro;
    QCompleter *m_completer;
    QStandardItemModel *m_modelo;
    QTimer *m_timer;
    QString m_ultimaBusqueda;
    QList<QJsonObject> m_resultados;   // de m_ultimaBusqueda
    int m_peticion = 0;               // descarta respuestas viejas
};
