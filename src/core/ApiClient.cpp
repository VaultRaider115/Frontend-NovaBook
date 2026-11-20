#include "ApiClient.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QUrl>
#include <QDebug>

using namespace std;

// Busca `key` en las variables de entorno y luego en el archivo .env.
QString ApiClient::setting(const QString &key, const QString &fallback) {
    const QByteArray envValue = qgetenv(key.toUtf8().constData());
    if (!envValue.isEmpty()) {
        return QString::fromUtf8(envValue);
    }

    const QStringList envFileCandidates = {
        QDir::current().absoluteFilePath(".env"),
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../.env")
    };

    for (const QString &envFilePath : envFileCandidates) {
        QFile envFile(envFilePath);
        if (!envFile.open(QIODevice::ReadOnly)) {
            continue;
        }

        QTextStream stream(&envFile);
        while (!stream.atEnd()) {
            QString line = stream.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#')) {
                continue;
            }

            const int separator = line.indexOf('=');
            if (separator <= 0) {
                continue;
            }

            QString value = line.mid(separator + 1).trimmed();
            if (value.startsWith('"') && value.endsWith('"')) {
                value = value.mid(1, value.length() - 2);
            }

            if (line.left(separator).trimmed() == key && !value.isEmpty()) {
                return value;
            }
        }
    }

    return fallback;
}

ApiClient* ApiClient::instance() {
    static ApiClient s_instance;
    return &s_instance;
}

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_baseUrl(setting("API_URL", "http://localhost:8080"))
{
}

void ApiClient::setBaseUrl(const QString &url) {
    m_baseUrl = url.trimmed();
    while (m_baseUrl.endsWith('/')) {
        m_baseUrl.chop(1);
    }
}

QString ApiClient::baseUrl() const {
    return m_baseUrl;
}

void ApiClient::setToken(const QString &token) {
    m_token = token;
}

QString ApiClient::token() const {
    return m_token;
}

bool ApiClient::isAuthenticated() const {
    return !m_token.isEmpty();
}

void ApiClient::logout() {
    m_token.clear();
    m_currentUser = UsuarioDto();
    emit userLoggedOut();
}

void ApiClient::setCurrentUser(const UsuarioDto &user) {
    m_currentUser = user;
    emit userLoggedIn(user);
}

UsuarioDto ApiClient::currentUser() const {
    return m_currentUser;
}

bool ApiClient::handleUnauthorized(int httpStatus, const QString &sentToken, const QByteArray &body) {
    // Sin token (p. ej. login con credenciales incorrectas) el 401 es un error normal.
    if (httpStatus != 401 || sentToken.isEmpty()) {
        return false;
    }
    // Respuesta de una sesión anterior: ya se cerró o se inició otra; se descarta.
    if (sentToken != m_token) {
        return true;
    }

    QString reason = "Tu sesión expiró. Vuelve a iniciar sesión.";
    const QJsonObject err = QJsonDocument::fromJson(body).object().value("error").toObject();
    if (err.value("code").toString() != "INVALID_TOKEN") {
        const QString serverMsg = err.value("message").toString();
        if (!serverMsg.isEmpty()) reason = serverMsg;
    }

    expireSession(reason);
    return true;
}

void ApiClient::expireSession(const QString &reason) {
    if (m_token.isEmpty()) return;   // ya no hay sesión: no notificar dos veces
    m_token.clear();
    m_currentUser = UsuarioDto();
    emit sessionExpired(reason);
}

void ApiClient::sendRequest(const QString &verb, const QString &endpoint, const QUrlQuery &query,
                            const QByteArray &body, ApiCallback cb) {
    QUrl url(m_baseUrl + endpoint);
    if (!query.isEmpty()) {
        url.setQuery(query);
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    }

    QNetworkReply *reply = nullptr;
    if (verb == "GET") {
        reply = m_nam->get(request);
    } else if (verb == "POST") {
        reply = m_nam->post(request, body);
    } else if (verb == "PUT") {
        reply = m_nam->put(request, body);
    } else if (verb == "DELETE") {
        reply = m_nam->deleteResource(request);
    } else {
        if (cb) cb(false, QJsonValue(), "Verbo HTTP no soportado");
        return;
    }

    connect(reply, &QNetworkReply::finished, this, [this, reply, cb, sentToken = m_token]() {
        reply->deleteLater();
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray respData = reply->readAll();

        if (handleUnauthorized(httpStatus, sentToken, respData)) {
            return;
        }

        if (reply->error() != QNetworkReply::NoError && respData.isEmpty()) {
            if (cb) cb(false, QJsonValue(), QString("Error de red (%1): %2").arg(httpStatus).arg(reply->errorString()));
            return;
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(respData, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            if (cb) cb(false, QJsonValue(), QString("Respuesta no válida del servidor (%1)").arg(httpStatus));
            return;
        }

        QJsonObject root = doc.object();
        bool success = root.value("success").toBool(false);
        QString message = root.value("message").toString();

        if (!success) {
            if (root.contains("error") && root.value("error").isObject()) {
                QJsonObject errObj = root.value("error").toObject();
                message = errObj.value("message").toString(message);
            }
            if (message.isEmpty()) {
                message = QString("Error en la solicitud (Código HTTP %1)").arg(httpStatus);
            }
            if (cb) cb(false, root.value("data"), message);
            return;
        }

        if (cb) cb(true, root.value("data"), message);
    });
}

// --- Autenticación ---

void ApiClient::login(qint64 id_usuario, const QString &contrasena, ApiCallback cb) {
    QJsonObject req;
    req["id_usuario"] = id_usuario;
    req["contrasena"] = contrasena;

    sendRequest("POST", "/api/auth/login", QUrlQuery(), QJsonDocument(req).toJson(),
        [this, cb](bool success, const QJsonValue &data, const QString &message) {
            if (success && data.isObject()) {
                LoginResponse resp = LoginResponse::fromJson(data.toObject());
                this->setToken(resp.token);
                this->setCurrentUser(resp.usuario);
            }
            if (cb) cb(success, data, message);
        }
    );
}

void ApiClient::getPerfil(ApiCallback cb) {
    sendRequest("GET", "/api/auth/perfil", QUrlQuery(), QByteArray(), cb);
}

// --- CRUD Usuarios ---

void ApiClient::listarUsuarios(ApiCallback cb) {
    sendRequest("GET", "/api/usuarios", QUrlQuery(), QByteArray(), cb);
}

void ApiClient::obtenerUsuario(qint64 id, ApiCallback cb) {
    sendRequest("GET", QString("/api/usuarios/%1").arg(id), QUrlQuery(), QByteArray(), cb);
}

void ApiClient::crearUsuario(const QJsonObject &userObj, ApiCallback cb) {
    sendRequest("POST", "/api/usuarios", QUrlQuery(), QJsonDocument(userObj).toJson(), cb);
}

void ApiClient::actualizarUsuario(qint64 id, const QJsonObject &userObj, ApiCallback cb) {
    sendRequest("PUT", QString("/api/usuarios/%1").arg(id), QUrlQuery(), QJsonDocument(userObj).toJson(), cb);
}

void ApiClient::eliminarUsuario(qint64 id, ApiCallback cb) {
    sendRequest("DELETE", QString("/api/usuarios/%1").arg(id), QUrlQuery(), QByteArray(), cb);
}

// --- Catálogo de Géneros ---

void ApiClient::listarGeneros(ApiCallback cb) {
    sendRequest("GET", "/api/generos", QUrlQuery(), QByteArray(), cb);
}

void ApiClient::crearGenero(const QJsonObject &obj, ApiCallback cb) {
    sendRequest("POST", "/api/generos", QUrlQuery(), QJsonDocument(obj).toJson(), cb);
}

void ApiClient::actualizarGenero(int id, const QJsonObject &obj, ApiCallback cb) {
    sendRequest("PUT", QString("/api/generos/%1").arg(id), QUrlQuery(), QJsonDocument(obj).toJson(), cb);
}

void ApiClient::eliminarGenero(int id, ApiCallback cb) {
    sendRequest("DELETE", QString("/api/generos/%1").arg(id), QUrlQuery(), QByteArray(), cb);
}

// --- Catálogo de Motivos ---

void ApiClient::listarMotivos(ApiCallback cb) {
    sendRequest("GET", "/api/motivos", QUrlQuery(), QByteArray(), cb);
}

void ApiClient::crearMotivo(const QJsonObject &obj, ApiCallback cb) {
    sendRequest("POST", "/api/motivos", QUrlQuery(), QJsonDocument(obj).toJson(), cb);
}

void ApiClient::actualizarMotivo(int id, const QJsonObject &obj, ApiCallback cb) {
    sendRequest("PUT", QString("/api/motivos/%1").arg(id), QUrlQuery(), QJsonDocument(obj).toJson(), cb);
}

void ApiClient::eliminarMotivo(int id, ApiCallback cb) {
    sendRequest("DELETE", QString("/api/motivos/%1").arg(id), QUrlQuery(), QByteArray(), cb);
}

// --- Catálogo de Muebles ---

void ApiClient::listarMuebles(ApiCallback cb) {
    sendRequest("GET", "/api/muebles", QUrlQuery(), QByteArray(), cb);
}

void ApiClient::crearMueble(const QJsonObject &obj, ApiCallback cb) {
    sendRequest("POST", "/api/muebles", QUrlQuery(), QJsonDocument(obj).toJson(), cb);
}

void ApiClient::actualizarMueble(int id, const QJsonObject &obj, ApiCallback cb) {
    sendRequest("PUT", QString("/api/muebles/%1").arg(id), QUrlQuery(), QJsonDocument(obj).toJson(), cb);
}

void ApiClient::eliminarMueble(int id, ApiCallback cb) {
    sendRequest("DELETE", QString("/api/muebles/%1").arg(id), QUrlQuery(), QByteArray(), cb);
}

// --- CRUD Autores ---

void ApiClient::listarAutores(ApiCallback cb) {
    sendRequest("GET", "/api/autores", QUrlQuery(), QByteArray(), cb);
}

void ApiClient::obtenerAutor(int id, ApiCallback cb) {
    sendRequest("GET", QString("/api/autores/%1").arg(id), QUrlQuery(), QByteArray(), cb);
}

void ApiClient::crearAutor(const QJsonObject &autorObj, ApiCallback cb) {
    sendRequest("POST", "/api/autores", QUrlQuery(), QJsonDocument(autorObj).toJson(), cb);
}

void ApiClient::actualizarAutor(int id, const QJsonObject &autorObj, ApiCallback cb) {
    sendRequest("PUT", QString("/api/autores/%1").arg(id), QUrlQuery(), QJsonDocument(autorObj).toJson(), cb);
}

void ApiClient::eliminarAutor(int id, ApiCallback cb) {
    sendRequest("DELETE", QString("/api/autores/%1").arg(id), QUrlQuery(), QByteArray(), cb);
}

// --- CRUD Proveedores ---

void ApiClient::listarEditoriales(ApiCallback cb) {
    sendRequest("GET", "/api/editoriales", QUrlQuery(), QByteArray(), cb);
}

void ApiClient::listarProveedores(ApiCallback cb) {
    sendRequest("GET", "/api/proveedores", QUrlQuery(), QByteArray(), cb);
}

void ApiClient::obtenerProveedor(int id, ApiCallback cb) {
    sendRequest("GET", QString("/api/proveedores/%1").arg(id), QUrlQuery(), QByteArray(), cb);
}

void ApiClient::crearProveedor(const QJsonObject &provObj, ApiCallback cb) {
    sendRequest("POST", "/api/proveedores", QUrlQuery(), QJsonDocument(provObj).toJson(), cb);
}

void ApiClient::actualizarProveedor(int id, const QJsonObject &provObj, ApiCallback cb) {
    sendRequest("PUT", QString("/api/proveedores/%1").arg(id), QUrlQuery(), QJsonDocument(provObj).toJson(), cb);
}

void ApiClient::eliminarProveedor(int id, ApiCallback cb) {
    sendRequest("DELETE", QString("/api/proveedores/%1").arg(id), QUrlQuery(), QByteArray(), cb);
}

// --- Personal de Bodega ---

void ApiClient::registrarLibroBodega(const QJsonObject &libroObj, ApiCallback cb) {
    sendRequest("POST", "/api/bodega/libros", QUrlQuery(), QJsonDocument(libroObj).toJson(), cb);
}

void ApiClient::registrarRevistaBodega(const QJsonObject &revistaObj, ApiCallback cb) {
    sendRequest("POST", "/api/bodega/revistas", QUrlQuery(), QJsonDocument(revistaObj).toJson(), cb);
}

void ApiClient::trasladoBodegaLibros(qint64 ean, int cant, const QString &obs, ApiCallback cb) {
    QJsonObject req;
    req["codigo_ean"] = ean;
    req["cantidad"] = cant;
    if (!obs.isEmpty()) req["observaciones"] = obs;
    sendRequest("POST", "/api/bodega/movimientos/libros", QUrlQuery(), QJsonDocument(req).toJson(), cb);
}

void ApiClient::trasladoBodegaRevistas(qint64 ean, int cant, const QString &obs, ApiCallback cb) {
    QJsonObject req;
    req["codigo_ean"] = ean;
    req["cantidad"] = cant;
    if (!obs.isEmpty()) req["observaciones"] = obs;
    sendRequest("POST", "/api/bodega/movimientos/revistas", QUrlQuery(), QJsonDocument(req).toJson(), cb);
}

void ApiClient::registrarCompra(const QJsonObject &compraObj, ApiCallback cb) {
    sendRequest("POST", "/api/bodega/compras", QUrlQuery(), QJsonDocument(compraObj).toJson(), cb);
}

void ApiClient::listarCompras(ApiCallback cb) {
    sendRequest("GET", "/api/bodega/compras", QUrlQuery(), QByteArray(), cb);
}

// --- Vendedor ---

void ApiClient::consultarExistencias(const QString &q, const QString &tipo, int ubicacion, ApiCallback cb) {
    QUrlQuery query;
    if (!q.isEmpty()) query.addQueryItem("q", q);
    if (!tipo.isEmpty()) query.addQueryItem("tipo", tipo);
    if (ubicacion > 0) query.addQueryItem("ubicacion", QString::number(ubicacion));

    // Bodega la usa para armar la requisición de salida (título y editorial) y el
    // Jefe para ver los títulos de una solicitud de editorial.
    const int rol = currentUser().id_roles;
    const QString ruta = rol == 2 ? "/api/bodega/existencias" : (rol == 1 ? "/api/jefe/existencias" : "/api/vendedor/existencias");
    sendRequest("GET", ruta, query, QByteArray(), cb);
}

void ApiClient::registrarVenta(const QString &cliente, const QJsonArray &items, ApiCallback cb) {
    registrarVenta(cliente, "EFECTIVO", items, cb);
}

void ApiClient::registrarVenta(const QString &cliente, const QString &metodoPago, const QJsonArray &items, ApiCallback cb) {
    QJsonObject req;
    req["cliente"] = cliente;
    req["metodo_pago"] = metodoPago;
    req["items"] = items;
    sendRequest("POST", "/api/vendedor/ventas", QUrlQuery(), QJsonDocument(req).toJson(), cb);
}

void ApiClient::registrarVenta(const QJsonArray &items, ApiCallback cb) {
    registrarVenta("Público en General", items, cb);
}

void ApiClient::trasladoTiendaLibros(qint64 ean, int cant, const QString &obs, ApiCallback cb) {
    QJsonObject req;
    req["codigo_ean"] = ean;
    req["cantidad"] = cant;
    if (!obs.isEmpty()) req["observaciones"] = obs;
    sendRequest("POST", "/api/vendedor/movimientos/libros", QUrlQuery(), QJsonDocument(req).toJson(), cb);
}

void ApiClient::trasladoTiendaRevistas(qint64 ean, int cant, const QString &obs, ApiCallback cb) {
    QJsonObject req;
    req["codigo_ean"] = ean;
    req["cantidad"] = cant;
    if (!obs.isEmpty()) req["observaciones"] = obs;
    sendRequest("POST", "/api/vendedor/movimientos/revistas", QUrlQuery(), QJsonDocument(req).toJson(), cb);
}

void ApiClient::crearDevolucion(int idProveedor, const QString &tipo, const QJsonArray &items, ApiCallback cb) {
    QJsonObject req;
    req["id_proveedor"] = idProveedor;
    req["tipo_producto"] = tipo;
    req["items"] = items;
    sendRequest("POST", "/api/vendedor/devoluciones", QUrlQuery(), QJsonDocument(req).toJson(), cb);
}

void ApiClient::registrarRequisicion(bool salida, const QJsonArray &items, ApiCallback cb) {
    QJsonObject req;
    req["items"] = items;
    sendRequest("POST", salida ? "/api/bodega/requisiciones" : "/api/vendedor/requisiciones",
                QUrlQuery(), QJsonDocument(req).toJson(), cb);
}

namespace {
// Bodega (rol 2) y Vendedor (rol 3) consultan requisiciones bajo su propio prefijo.
QString prefijoRequisiciones(int rol) {
    return rol == 2 ? QStringLiteral("/api/bodega/requisiciones") : QStringLiteral("/api/vendedor/requisiciones");
}
} // namespace

void ApiClient::listarRequisiciones(const QString &tipo, const QString &estado, ApiCallback cb) {
    QUrlQuery query;
    if (!tipo.isEmpty()) query.addQueryItem("tipo", tipo);
    if (!estado.isEmpty()) query.addQueryItem("estado", estado);
    sendRequest("GET", prefijoRequisiciones(m_currentUser.id_roles), query, QByteArray(), cb);
}

void ApiClient::obtenerRequisicion(const QString &folio, ApiCallback cb) {
    sendRequest("GET", prefijoRequisiciones(m_currentUser.id_roles) + "/" + QUrl::toPercentEncoding(folio),
                QUrlQuery(), QByteArray(), cb);
}

void ApiClient::recibirRequisicion(const QString &folio, const QJsonArray &items, const QString &observaciones, ApiCallback cb) {
    QJsonObject req;
    req["items"] = items;
    if (!observaciones.trimmed().isEmpty()) req["observaciones"] = observaciones.trimmed();
    sendRequest("POST", "/api/vendedor/requisiciones/" + QUrl::toPercentEncoding(folio) + "/recepcion",
                QUrlQuery(), QJsonDocument(req).toJson(), cb);
}

void ApiClient::listarIncidencias(const QString &estado, ApiCallback cb) {
    QUrlQuery query;
    if (!estado.isEmpty()) query.addQueryItem("estado", estado);
    sendRequest("GET", "/api/bodega/incidencias", query, QByteArray(), cb);
}

void ApiClient::reponerIncidencia(qint64 idIncidencia, const QJsonArray &items, ApiCallback cb) {
    sendRequest("POST", QString("/api/bodega/incidencias/%1/reposicion").arg(idIncidencia), QUrlQuery(),
                QJsonDocument(QJsonObject{{"items", items}}).toJson(), cb);
}

void ApiClient::consultarMisDevoluciones(const QString &estado, ApiCallback cb) {
    QUrlQuery query;
    if (!estado.isEmpty()) query.addQueryItem("estado", estado);
    sendRequest("GET", "/api/vendedor/devoluciones", query, QByteArray(), cb);
}

void ApiClient::consultarTicket(qint64 idVenta, ApiCallback cb) {
    sendRequest("GET", QString("/api/vendedor/tickets/%1").arg(idVenta), QUrlQuery(), QByteArray(), cb);
}

void ApiClient::registrarDevolucionCliente(const QJsonObject &req, ApiCallback cb) {
    sendRequest("POST", "/api/vendedor/devoluciones-cliente", QUrlQuery(), QJsonDocument(req).toJson(), cb);
}

// --- Inventario mensual ---

void ApiClient::inventarioActual(ApiCallback cb) {
    sendRequest("GET", "/api/vendedor/inventarios/actual", QUrlQuery(), QByteArray(), cb);
}

void ApiClient::abrirInventario(ApiCallback cb) {
    sendRequest("POST", "/api/vendedor/inventarios", QUrlQuery(), QByteArray("{}"), cb);
}

void ApiClient::registrarConteo(qint64 idInventario, qint64 ean, int cantidad, ApiCallback cb) {
    QJsonObject req{{"codigo_ean", ean}, {"cantidad", cantidad}};
    sendRequest("PUT", QString("/api/vendedor/inventarios/%1/conteos").arg(idInventario), QUrlQuery(),
                QJsonDocument(req).toJson(), cb);
}

void ApiClient::pendientesDevolucion(qint64 idInventario, ApiCallback cb) {
    sendRequest("GET", QString("/api/vendedor/inventarios/%1/devolucion").arg(idInventario), QUrlQuery(), QByteArray(), cb);
}

void ApiClient::validarPartidaDevolucion(int idProveedor, qint64 ean, int cantidad, ApiCallback cb) {
    QUrlQuery q;
    q.addQueryItem("id_proveedor", QString::number(idProveedor));
    q.addQueryItem("codigo_ean", QString::number(ean));
    q.addQueryItem("cantidad", QString::number(cantidad));
    sendRequest("GET", "/api/vendedor/devoluciones/validar", q, QByteArray(), cb);
}

void ApiClient::cerrarInventario(qint64 idInventario, ApiCallback cb) {
    sendRequest("POST", QString("/api/vendedor/inventarios/%1/cerrar").arg(idInventario), QUrlQuery(),
                QByteArray("{}"), cb);
}

void ApiClient::listarInventarios(ApiCallback cb) {
    sendRequest("GET", "/api/jefe/inventarios", QUrlQuery(), QByteArray(), cb);
}

void ApiClient::obtenerInventario(qint64 idInventario, ApiCallback cb) {
    sendRequest("GET", QString("/api/jefe/inventarios/%1").arg(idInventario), QUrlQuery(), QByteArray(), cb);
}

void ApiClient::revisarInventario(qint64 idInventario, bool aprobar, const QString &observaciones, ApiCallback cb) {
    QJsonObject req{{"aprobar", aprobar}};
    if (!observaciones.trimmed().isEmpty()) req["observaciones"] = observaciones.trimmed();
    sendRequest("PUT", QString("/api/jefe/inventarios/%1/revisar").arg(idInventario), QUrlQuery(),
                QJsonDocument(req).toJson(), cb);
}

void ApiClient::listarSolicitudesEditorial(ApiCallback cb) {
    sendRequest("GET", "/api/jefe/solicitudes-editorial", QUrlQuery(), QByteArray(), cb);
}

void ApiClient::crearSolicitudEditorial(int idProveedor, const QString &referencia, const QList<qint64> &eans, ApiCallback cb) {
    QJsonArray arr;
    for (qint64 e : eans) arr.append(e);
    QJsonObject req{{"id_proveedor", idProveedor}, {"referencia", referencia}, {"eans", arr}};
    sendRequest("POST", "/api/jefe/solicitudes-editorial", QUrlQuery(), QJsonDocument(req).toJson(), cb);
}

void ApiClient::atenderSolicitudEditorial(qint64 idSolicitud, ApiCallback cb) {
    sendRequest("PUT", QString("/api/jefe/solicitudes-editorial/%1/atender").arg(idSolicitud), QUrlQuery(),
                QByteArray("{}"), cb);
}

void ApiClient::descargarPdfDevolucion(qint64 idDevolucion, const QString &tipo, PdfCallback cb) {
    QString subpath = (tipo.toLower() == "revista") ? "revistas" : "libros";
    // El acta la descargan su solicitante (ruta de vendedor), el Jefe o el Gerente.
    const QString rolPath = m_currentUser.id_roles == 1 ? "jefe"
                          : m_currentUser.id_roles == 4 ? "gerente" : "vendedor";
    QUrl url(QString("%1/api/%2/devoluciones/%3/pdf/%4")
                 .arg(m_baseUrl, rolPath)
                 .arg(idDevolucion)
                 .arg(subpath));

    QNetworkRequest request(url);
    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    }

    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, cb, sentToken = m_token]() {
        reply->deleteLater();
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();
        if (httpStatus == 401 && handleUnauthorized(httpStatus, sentToken, body)) {
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            // La API explica el motivo en JSON (p. ej. "solo existe si está aprobada").
            const QString apiMsg = QJsonDocument::fromJson(body).object()
                                       .value("error").toObject().value("message").toString();
            if (cb) cb(false, QByteArray(), apiMsg.isEmpty() ? reply->errorString() : apiMsg);
            return;
        }

        QString disposition = reply->rawHeader("Content-Disposition");
        QString filename = "devolucion.pdf";
        if (disposition.contains("filename=")) {
            filename = disposition.section("filename=", 1, 1).replace("\"", "").trimmed();
        }

        if (cb) cb(true, body, filename);
    });
}

// --- Jefe de Departamento ---

void ApiClient::consultarHistorialDevoluciones(const QString &fecha, const QString &estado, int idProveedor, ApiCallback cb) {
    QUrlQuery query;
    if (!fecha.isEmpty()) query.addQueryItem("fecha", fecha);
    if (!estado.isEmpty() && estado != "Todos") query.addQueryItem("estado", estado);
    if (idProveedor > 0) query.addQueryItem("id_proveedor", QString::number(idProveedor));

    // El Gerente consulta el mismo historial por su propia ruta (solo lectura).
    const QString base = m_currentUser.id_roles == 4 ? "/api/gerente" : "/api/jefe";
    sendRequest("GET", base + "/devoluciones/historial", query, QByteArray(), cb);
}

void ApiClient::consultarMovimientoDiario(const QString &fecha, ApiCallback cb) {
    QUrlQuery query;
    if (!fecha.isEmpty()) query.addQueryItem("fecha", fecha);

    sendRequest("GET", "/api/jefe/movimientos/diarios", query, QByteArray(), cb);
}

void ApiClient::consultarBitacora(const QString &desde, const QString &hasta, const QString &categoria,
                                  int page, int perPage, ApiCallback cb) {
    QUrlQuery query;
    query.addQueryItem("desde", desde);
    query.addQueryItem("hasta", hasta);
    if (!categoria.isEmpty()) query.addQueryItem("categoria", categoria);
    query.addQueryItem("page", QString::number(page));
    query.addQueryItem("per_page", QString::number(perPage));
    sendRequest("GET", "/api/jefe/bitacora", query, QByteArray(), cb);
}

void ApiClient::aprobarDevolucion(qint64 idDevolucion, bool aprobar, const QString &notas, ApiCallback cb) {
    QJsonObject req;
    req["aprobar"] = aprobar;
    if (!notas.isEmpty()) req["notas"] = notas;

    sendRequest("PUT", QString("/api/jefe/devoluciones/%1/aprobar").arg(idDevolucion),
                QUrlQuery(), QJsonDocument(req).toJson(), cb);
}
