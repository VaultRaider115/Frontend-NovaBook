# NovaBook — Frontend de escritorio (C++ / Qt 6)

Cliente de escritorio del ERP de la librería Sanborns Saltillo. Consume la API
REST de `../API-Novabook` y muestra un menú distinto para cada rol: Gerente,
Jefe de Departamento, Personal de Bodega y Vendedor. Las pantallas siguen los
diseños de `../docs/disenos/` (1–21): tema oscuro con acento dorado `#D4A346`.

## 1. Stack

- C++23, CMake ≥ 3.16
- Qt 6: Core, Gui (incluye `QPdfWriter`), Widgets y Network
- `QNetworkAccessManager` en un cliente HTTP único (`ApiClient`)

## 2. Estructura

```text
Frontend-NovaBook/
├── CMakeLists.txt
├── README.md · CHANGELOG.md · CAMBIOS_FIGMA.md
└── src/
    ├── main.cpp
    ├── core/
    │   ├── ApiClient        cliente HTTP (singleton), JWT, sesión y configuración
    │   ├── Documentos       PDF de la requisición; guardar y abrir PDFs
    │   ├── EanClassifier    EAN-13: dígito verificador y tipo (libro/revista)
    │   ├── InactivityWatcher cierre de sesión por inactividad
    │   ├── Models           DTOs del dominio
    │   └── Theme            paleta, QSS global y colores por rol/categoría
    ├── widgets/
    │   ├── Ui               kit visual: table, cellText, cellWidget, fitColumn,
    │   │                    chip, button, field, FormDialog, pageHeader…
    │   ├── CatalogView      tablas de catálogo con alta/edición/baja
    │   ├── RequisicionView  requisición por lista (salida o entrada)
    │   ├── StatCard         tarjetas de indicadores
    │   └── Toast            avisos de éxito, aviso o error
    ├── views/
    │   ├── LoginWindow, MainWindow
    │   ├── gerente/  jefe/  bodega/  vendedor/
    └── resources/            resources.qrc e íconos SVG
```

## 3. Menús por rol

El menú lateral se arma en `MainWindow` según `id_roles`. La API vuelve a
validar cada permiso.

| Rol (id) | Pantallas |
|---|---|
| Gerente (4) | Panel Principal · Gestión de Usuarios (crea usuarios y asigna roles) · Devoluciones a Proveedor (solo lectura y actas) |
| Jefe de Departamento (1) | Panel Principal · Historial de Movimientos (bitácora) · Aprobación de Devoluciones · Gestión de Usuarios (sin tocar roles ni Gerentes) · Catálogos de Autores, Géneros, Motivos y Muebles · Proveedores · Inventario Mensual (revisión y solicitudes de editorial) |
| Personal de Bodega (2) | Panel Principal · Alta de Mercancía (EAN) · Compras a Proveedor · Incidencias de Compra · Proveedores (solo alta) · Requisición de Salida · Seguimiento de Salidas · Consultar Inventario |
| Vendedor (3) | Panel Principal · Consulta de Existencias · Punto de Venta · Devolución de Cliente · Recepción de Salida · Requisición de Entrada · Devoluciones a Proveedor · Mis Devoluciones · Inventario Mensual (conteo) |

## 4. Configuración

`ApiClient::setting()` busca primero la variable de entorno y luego un archivo
`.env`, ya sea en el directorio actual o junto a la carpeta del ejecutable
(`../.env`).

| Variable | Default | Uso |
|---|---|---|
| `API_URL` | `http://localhost:8080` | URL base de la API |
| `INACTIVIDAD_MINUTOS` | 15 | minutos sin actividad antes de cerrar la sesión |

## 5. Sesión

- **Login:** `POST /api/auth/login`. El token JWT va en cada petición.
- **401:** si la API responde 401 (token vencido o usuario dado de baja), la
  app cierra la sesión y vuelve al login.
- **Inactividad:** `InactivityWatcher` cierra la sesión tras
  `INACTIVIDAD_MINUTOS` sin teclas, clics, movimiento del ratón, rueda ni
  toques.

## 6. Integración con la API

Todas las llamadas pasan por `ApiClient` con el patrón
`ApiCallback(bool ok, QJsonValue data, QString msg)`. La API responde:

```json
{ "success": true, "data": { } }
{ "success": false, "error": { "code": "BAD_REQUEST", "message": "…" } }
```

En caso de error, `msg` trae el mensaje de la API y la vista lo muestra con
`Toast`. Las reglas de negocio (EAN y SKU, una editorial por documento, retorno
de máximo la mitad a bodega, límites de devoluciones, etc.) las valida la API;
la interfaz repite las que ayudan a capturar mejor (EAN, SKU, teléfono, mezcla
de editoriales).

## 7. Documentos PDF

La app pregunta dónde guardarlos (propone `Documentos/NovaBook`) y los abre
en el visor del sistema para imprimir.

- **Requisición:** la genera Qt (`Documentos::pdfRequisicion`). Lleva folio
  `REQ-AAAA-n`, editorial, origen y destino, partidas y firmas. Ya recibida,
  agrega el mueble de piso y quién recibió; si se corrigió, el sello
  CORREGIDA y la cantidad original tachada.
- **Aviso de compra incompleta:** no es PDF. La app abre el programa de
  correo del equipo con el texto que arma la API.
- **Acta de devolución a proveedor:** la genera la API (diseño 21) y solo
  existe si la devolución está aprobada.

## 8. Compilación y ejecución

### Windows (MSVC + Ninja, como el equipo)

Desde una consola con `vcvars64.bat` cargado y Qt 6 MSVC en el `PATH`:

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\msvc2022_64
cmake --build build
build\novabook_frontend.exe
```

### Linux

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/ruta/a/Qt/6.x/gcc_64
cmake --build build -j"$(nproc)"
./build/novabook_frontend
```

La API debe estar corriendo (ver `../API-Novabook`); si no está en
`http://localhost:8080`, define `API_URL`.

## 9. Usuarios de prueba (semilla de la base)

| Usuario | Contraseña | Rol |
|---|---|---|
| 1001 | admin123 | Gerente |
| 350976899 | 2501 | Jefe de Departamento |
| 628777130 | 7777 | Personal de Bodega |
| 628777129 | 8888 | Vendedor |

## 10. Convenciones

- Toda comunicación HTTP pasa por `ApiClient`.
- **Colores y estilos:** se toman de `Theme` y de los componentes de `Ui`;
  no se escriben colores sueltos en las vistas.
- **Identificadores grandes:** EAN, usuarios y folios usan `qint64`.
- **Clases nuevas:** toda clase o vista nueva se registra en `CMakeLists.txt`.
- **Cambios en la API:** si cambian rutas, permisos o payloads del backend,
  también se ajustan aquí.
- **Al terminar una pantalla:** se compara con su imagen en
  `../docs/disenos/`.
