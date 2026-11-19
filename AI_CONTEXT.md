# Contexto técnico del frontend NovaBook

Este documento describe cómo está hecho por dentro el cliente de escritorio
Qt 6 del ERP de NovaBook, para quien vaya a modificarlo. Si algo de aquí no
coincide con el código, manda el código.

- **Uso y compilación:** `README.md`.
- **Diseños y paleta:** `CAMBIOS_FIGMA.md`.
- **Backend:** `../API-Novabook/AI_CONTEXT.md`.

## 1. Arranque (`src/main.cpp`)

1. Estilo Fusion con una paleta oscura de `Theme`, para que los diálogos y
   el calendario sin estilo propio hereden el tema.
2. Hoja de estilos global `Theme::globalStyleSheet()`.
3. Se crea un `InactivityWatcher` para toda la aplicación.
4. `LoginWindow`. Al iniciar sesión crea `MainWindow`, que arma el menú
   según `id_roles`. Al expirar la sesión, `LoginWindow::onSessionExpired`
   cierra los diálogos abiertos, oculta `MainWindow` y muestra el login con
   el motivo.

## 2. `ApiClient` (`src/core/ApiClient.*`)

Singleton (`ApiClient::instance()`) y único punto de acceso HTTP.

- **Patrón de llamada:** cada método recibe
  `ApiCallback(bool ok, QJsonValue data, QString msg)`.
  - Si `ok` es true, `data` trae el contenido de `"data"` de la respuesta.
  - Si es false, `msg` trae `error.message` de la API, listo para mostrarse
    en un `Toast`.
- **Sesión:**
  - `login()` guarda el token y el `UsuarioDto` (`currentUser()`) y emite
    `userLoggedIn`.
  - Cada petición manda `Authorization: Bearer`.
- **401 con token:**
  - `handleUnauthorized` → `expireSession(reason)` emite `sessionExpired`.
  - `LoginWindow::onSessionExpired` cierra todo y avisa.
  - Un 401 sin token (credenciales incorrectas) es un error normal.
- **Ruta según el rol:**
  - `consultarExistencias` usa `/api/bodega/...` para Bodega y
    `/api/vendedor/...` para los demás.
  - El historial y las actas de devoluciones usan `/api/gerente/...` para el
    Gerente y `/api/jefe/...` para el Jefe.
- **Configuración:** `ApiClient::setting(key, fallback)` lee la variable de
  entorno y luego `.env`, ya sea en el directorio actual o en `../.env` junto
  al ejecutable. Claves: `API_URL` e `INACTIVIDAD_MINUTOS`.
- **PDF:** `descargarPdfDevolucion` usa `PdfCallback` con los bytes del acta.

Cuando la API agrega o cambia una ruta:

1. Agrega el método aquí.
2. Agrega o ajusta el DTO en `Models.h`, si hace falta.
3. Úsalo desde la vista.

## 3. Otros módulos de `core/`

- **`Models.h`:** DTOs con `fromJson`. Los identificadores grandes (EAN,
  usuarios, folios) son `qint64`, y se leen con
  `toVariant().toLongLong()`, no con `toInt()`.
- **`EanClassifier`:**
  - `clasificar(texto)` devuelve `{esValido, tipo, mensaje}`. Revisa el
    dígito verificador; 978/979 es libro y 977 revista.
  - `aplicarAControles(texto, label)` pinta el estado bajo el campo.
- **`Theme`:** tokens de color (`Theme::Accent`, `Theme::Role1`…) y el QSS
  global.
  - Los botones toman estilo de la propiedad `variant`: `primary`, `outline`,
    `ghost`, `danger`, `approve`, `reject`, `pay`, `tint`, `small`, `icon`,
    `page`.
  - Los chips toman estilo de la propiedad `chip`: `libro`, `revista`, `ok`,
    `pending`, `danger`, `neutral`, `mint`, `blue`, `purple`, `amber`,
    `rose`, `gold`, `indigo`.
- **`Documentos`:**
  - `guardarYAbrir(parent, bytes, nombre)` pide dónde guardar, propone
    `Documentos/NovaBook`, escribe el archivo y lo abre en el visor del
    sistema.
  - `pdfRequisicion(Requisicion)` genera la requisición con `QPdfWriter`:
    folio, editorial, origen y destino, partidas y firmas. Si las partidas
    traen `recibida >= 0`, agrega la columna MUEBLE, tacha la cantidad
    original donde difiere y, con `estado == "CORREGIDA"`, dibuja el sello.
  - `Requisicion::desdeJson(obj)` arma el documento desde
    `GET /requisiciones/{folio}`.
  - `abrirCorreo(parent, {para, asunto, cuerpo})` abre un `mailto:` en el
    programa de correo predeterminado; sin destinatario o sin programa,
    copia el texto al portapapeles.
- **`InactivityWatcher`:** filtro de eventos global. Cuenta como actividad
  las teclas, clics, movimiento del ratón, rueda y toques. Tras
  `INACTIVIDAD_MINUTOS` (15 por defecto) sin actividad, llama a
  `expireSession`.

## 4. Kit visual (`src/widgets/`)

`Ui.h` concentra los constructores de widgets; úsalos en lugar de armar
estilos a mano.

| Función | Uso |
|---|---|
| `pageHeader(eyebrow, title, subtitle)` | encabezado de página (eyebrow dorado en mayúsculas) |
| `card()`, `divider()` | contenedores |
| `button(text, variant, parent, icon)`, `iconButton(icon, tooltip)` | botones con ícono de `:/icons/*.svg` |
| `chip(text, kind)`, `tipoChip(tipo)`, `avatar(nombre, tono)` | etiquetas de estado e iniciales |
| `field(label, input)`, `rowField(...)` | campo con etiqueta en mayúsculas espaciadas |
| `searchBox()`, `decorateDate()`, `stepper(spin)` | inputs |
| `eanInput(lineEdit)` | campo de EAN solo numérico (máximo 13): lo usa el Alta |
| `new ProductoBuscador(lineEdit, filtro)` | campo de EAN **o título**: con letras sugiere productos (`consultarExistencias`), al elegir deja el EAN; Enter con un solo resultado dispara `returnPressed`. `filtro` opcional (p. ej. solo el proveedor elegido) |
| `table(headers, stretchColumn, parent, rowHeight)` | tabla con el estilo del diseño |
| `cellText(t, r, c, text, mono, bold, color)`, `cellWidget(...)`, `cellWidgets(...)` | celdas |
| `fitColumn(t, col, widget)` | ensancha la columna para que un chip o botón no se recorte |
| `money()`, `phone()`, `initials()`, `spaced()` | formato |
| `FormDialog(title, okText)` + `addField(...)` | diálogo de alta y edición (rejilla y Cancelar/Aceptar) |

Otros widgets:

- **`CatalogView`:** base de las pantallas de catálogo (búsqueda, tabla,
  alta, edición y baja).
- **`RequisicionView(eyebrow, title, sendText, okMsg, salida)`:**
  requisición por lista. La usan `BodegaTrasladoView` (salida) y
  `VendedorTrasladoView` (entrada). Bloquea mezclar editoriales y al
  registrar imprime el PDF.
- **`StatCard`:** tarjeta de indicador de los paneles; el acento por defecto
  es `Theme::Accent`.
- **`Toast::showToast(parent, msg, tipo, ms)`:** aviso arriba al centro, de
  hasta 560 px de ancho.

## 5. Vistas (`src/views/`)

`MainWindow::addPage(textoMenu, vista, onShow)` agrega una entrada al menú;
`onShow` recarga los datos cada vez que se abre la página. El menú de cada
rol está en el README y el mapa de diseño → vista, en `CAMBIOS_FIGMA.md`.

Patrón habitual de una vista:

```cpp
layout->addWidget(Ui::pageHeader("Jefatura · Algo", "Título", "Subtítulo", this));
m_tabla = Ui::table({"COL1", "COL2", ""}, 1, this, 52);
// cargar():
ApiClient::instance()->listarAlgo([this](bool ok, const QJsonValue &data, const QString &msg) {
    if (!ok) { Toast::showToast(this, "No se pudo cargar: " + msg, Toast::Error); return; }
    // pintar filas con Ui::cellText / Ui::cellWidget
});
```

Al guardar, deshabilita el botón mientras responde la API y vuelve a
habilitarlo en el callback.

`BodegaComprasView` tiene dos secciones con botones tipo pestaña ("Registrar
factura" y "Facturas registradas", un `QStackedWidget` como en
`JefeInventarioView`). La segunda usa `listarCompras` (`GET
/api/bodega/compras`), filtra en el cliente por factura, proveedor, título o
EAN y el ojo abre el detalle de lo que se compró. Se recarga al abrir la
página (`cargar()`) y después de registrar una compra.

`VendedorRecepcionView(puedeRecibir)` sirve a dos roles: el Vendedor recibe
(`true`) y Bodega solo da seguimiento y reimprime (`false`). Las rutas de
`listarRequisiciones` / `obtenerRequisicion` se eligen por el rol en sesión.

## 6. Reglas que la interfaz repite

La API valida todo; la interfaz repite lo que ayuda a capturar mejor:

- EAN-13 con dígito verificador (`EanClassifier`); SKU de 6 a 7 dígitos.
- Teléfono solo con dígitos (`QRegularExpressionValidator`); trim en los
  nombres.
- Usuarios:
  - Nadie edita su propio rol.
  - El Jefe no registra usuarios ni modifica cuentas de Gerente (aparecen
    como "Solo Gerente").
- Requisición: una sola editorial por documento.
- Compras: "Llegaron" no puede pasar de "Facturadas"; sigue su valor hasta
  que se captura algo distinto. El número de factura lleva el prefijo fijo
  `F-` (se escribe solo el número; si escriben "F-12" no se repite).
- Los campos de EAN usan `ProductoBuscador` (EAN o título); el Alta usa
  `Ui::eanInput` (producto nuevo: solo el EAN).
- Requisiciones: al añadir se valida que el producto exista y que la cantidad
  (sumando la lista) no pase del stock del origen (bodega en la salida, piso
  en la entrada); la cantidad de la tabla tiene ese máximo. La regla de la
  mitad la valida la API al registrar.
- Proveedores: Bodega solo consulta (`BodegaProveedoresView(..., false)` oculta
  el alta).
- Recepción de la salida: pide confirmación antes de corregir y reimprime
  sola si quedó CORREGIDA.
- Catálogos: el botón de borrar se deshabilita si el registro está en uso.
- El rechazo de una devolución pide observaciones obligatorias.

## 7. Errores conocidos de Qt y cómo se resolvieron

- **Fondo de un `QWidget` simple:** no pinta el `background` de su hoja de
  estilo. Hay que activar `setAttribute(Qt::WA_StyledBackground)`; así se
  corrigió el `Toast`.
- **`QSpinBox` dentro de una celda:** se recorta. Usa
  `setFixedSize(90, 34)` y `QSpinBox { padding: 0px 10px; min-height: 0px; }`.
- **Chips en celdas:** el ítem suma su padding al del widget y lo recorta.
  Usa `Ui::cellWidget` y `Ui::fitColumn`.
- **Letras espaciadas:** QSS no soporta `letter-spacing`; usa
  `Ui::spaced(widget, px)`.
- **`qMin`/`qMax` con listas de inicialización:** no compilan con MSVC. Usa
  `std::clamp` o `std::min`.

## 8. Compilación y verificación

- **Windows (el equipo):** MSVC 2022 + Ninja + Qt 6.8.3 `msvc2022_64`; los
  comandos están en el README.
- **Clases nuevas:** toda clase nueva se registra en `CMakeLists.txt`
  (`SOURCES`/`HEADERS`).
- **Capturas sin pantalla:** se pueden tomar con `QT_QPA_PLATFORM=offscreen`
  y `QT_QPA_FONTDIR=C:\Windows\Fonts`, renderizando cada vista con
  `widget->grab()` contra una API de prueba. Así se hizo la revisión visual
  contra `../docs/disenos/`.
- **Antes de entregar:** compila sin avisos y compara la pantalla con su
  diseño.
