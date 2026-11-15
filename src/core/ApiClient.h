#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <functional>
#include "Models.h"

class ApiClient : public QObject {
    Q_OBJECT

public:
    using ApiCallback = std::function<void(bool success, const QJsonValue &data, const QString &message)>;
    using PdfCallback = std::function<void(bool success, const QByteArray &pdfBytes, const QString &filename)>;

    static ApiClient* instance();

    void setBaseUrl(const QString &url);
    QString baseUrl() const;

    void setToken(const QString &token);
    QString token() const;
    bool isAuthenticated() const;
    void logout();
    // Cierra la sesión sin intervención del usuario (401, inactividad) y emite sessionExpired.
    void expireSession(const QString &reason);

    // Valor de configuración: variable de entorno o, si no existe, línea KEY=valor del .env.
    static QString setting(const QString &key, const QString &fallback = QString());

    void setCurrentUser(const UsuarioDto &user);
    UsuarioDto currentUser() const;

    // --- Autenticación ---
    void login(qint64 id_usuario, const QString &contrasena, ApiCallback cb);
    void getPerfil(ApiCallback cb);

    // --- CRUD Usuarios (Jefe / Gerente) ---
    void listarUsuarios(ApiCallback cb);
    void obtenerUsuario(qint64 id, ApiCallback cb);
    void crearUsuario(const QJsonObject &userObj, ApiCallback cb);
    void actualizarUsuario(qint64 id, const QJsonObject &userObj, ApiCallback cb);
    void eliminarUsuario(qint64 id, ApiCallback cb);

    // --- Catálogo de Géneros ---
    void listarGeneros(ApiCallback cb);
    void crearGenero(const QJsonObject &obj, ApiCallback cb);
    void actualizarGenero(int id, const QJsonObject &obj, ApiCallback cb);
    void eliminarGenero(int id, ApiCallback cb);

    // --- Catálogo de Motivos de devolución (Jefe / Gerente) ---
    void listarMotivos(ApiCallback cb);
    void crearMotivo(const QJsonObject &obj, ApiCallback cb);
    void actualizarMotivo(int id, const QJsonObject &obj, ApiCallback cb);
    void eliminarMotivo(int id, ApiCallback cb);

    // --- Catálogo de Muebles y estantería (Jefe / Gerente) ---
    void listarMuebles(ApiCallback cb);
    void crearMueble(const QJsonObject &obj, ApiCallback cb);
    void actualizarMueble(int id, const QJsonObject &obj, ApiCallback cb);
    void eliminarMueble(int id, ApiCallback cb);

    // --- CRUD Autores (Bodega / Gerente) ---
    void listarAutores(ApiCallback cb);
    void obtenerAutor(int id, ApiCallback cb);
    void crearAutor(const QJsonObject &autorObj, ApiCallback cb);
    void actualizarAutor(int id, const QJsonObject &autorObj, ApiCallback cb);
    void eliminarAutor(int id, ApiCallback cb);

    // --- CRUD Proveedores (Bodega / Gerente) ---
    void listarProveedores(ApiCallback cb);
    void listarEditoriales(ApiCallback cb);
    void obtenerProveedor(int id, ApiCallback cb);
    void crearProveedor(const QJsonObject &provObj, ApiCallback cb);
    void actualizarProveedor(int id, const QJsonObject &provObj, ApiCallback cb);
    void eliminarProveedor(int id, ApiCallback cb);

    // --- Personal de Bodega: Mercancía, Traslados y Compras ---
    void registrarLibroBodega(const QJsonObject &libroObj, ApiCallback cb);
    void registrarRevistaBodega(const QJsonObject &revistaObj, ApiCallback cb);
    void trasladoBodegaLibros(qint64 ean, int cant, const QString &obs, ApiCallback cb);
    void trasladoBodegaRevistas(qint64 ean, int cant, const QString &obs, ApiCallback cb);
    void registrarCompra(const QJsonObject &compraObj, ApiCallback cb);
    void listarCompras(ApiCallback cb);

    // --- Vendedor ---
    void consultarExistencias(const QString &q, const QString &tipo, int ubicacion, ApiCallback cb);
    void registrarVenta(const QString &cliente, const QJsonArray &items, ApiCallback cb);
    void registrarVenta(const QString &cliente, const QString &metodoPago, const QJsonArray &items, ApiCallback cb);
    void registrarVenta(const QJsonArray &items, ApiCallback cb);
    void trasladoTiendaLibros(qint64 ean, int cant, const QString &obs, ApiCallback cb);
    void trasladoTiendaRevistas(qint64 ean, int cant, const QString &obs, ApiCallback cb);
    void crearDevolucion(int idProveedor, const QString &tipo, const QJsonArray &items, ApiCallback cb);
    void descargarPdfDevolucion(qint64 idDevolucion, const QString &tipo, PdfCallback cb);

    // --- Jefe de Departamento ---
    void consultarHistorialDevoluciones(const QString &fecha, const QString &estado, int idProveedor, ApiCallback cb);
    void consultarMovimientoDiario(const QString &fecha, ApiCallback cb);
    void consultarBitacora(const QString &desde, const QString &hasta, const QString &categoria, int page, int perPage, ApiCallback cb);
    void aprobarDevolucion(qint64 idDevolucion, bool aprobar, const QString &notas, ApiCallback cb);

signals:
    // Se emite una sola vez cuando el servidor rechaza (401) el token vigente.
    void sessionExpired(const QString &reason);
    void userLoggedIn(const UsuarioDto &user);
    void userLoggedOut();

private:
    explicit ApiClient(QObject *parent = nullptr);
    ~ApiClient() override = default;

    // Devuelve true si el 401 invalidó la sesión actual (y ya se notificó);
    // en ese caso la respuesta no debe llegar a la vista.
    bool handleUnauthorized(int httpStatus, const QString &sentToken, const QByteArray &body);

    void sendRequest(const QString &verb, const QString &endpoint, const QUrlQuery &query,
                     const QByteArray &body, ApiCallback cb);

    QNetworkAccessManager *m_nam;
    QString m_baseUrl;
    QString m_token;
    UsuarioDto m_currentUser;
};
