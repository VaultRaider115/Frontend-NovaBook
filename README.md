# NovaBook — Frontend de escritorio en C++ / Qt 6

Este frontend consume la API REST del backend `api_migrado` y presenta una interfaz de escritorio moderna, con navegación por roles y acceso a inventario, ventas, bitácora y gestión de usuarios.

## 1. Objetivo

La aplicación tiene como objetivo operar una librería con cuatro perfiles funcionales:

- Jefe de Departamento
- Gerente
- Personal de Bodega
- Vendedor

Cada rol tiene rutas, accesos y vistas distintas, con una capa de permisos aplicada tanto en UI como en la API del backend.

## 2. Stack técnico

- C++ 23
- Qt 6 Core / Gui / Widgets / Network
- CMake
- HTTP client centralizado con `QNetworkAccessManager`
- Integración REST con `api_migrado`

## 3. Estructura actual del proyecto

```text
frontend_migrado/
├── CMakeLists.txt
├── CHANGELOG.md
├── README.md
├── CAMBIOS_FIGMA.md
├── build/
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── ApiClient.h
│   │   ├── ApiClient.cpp
│   │   ├── EanClassifier.h
│   │   ├── Models.h
│   │   ├── Theme.h
│   │   └── Theme.cpp
│   ├── widgets/
│   │   ├── CatalogView.h
│   │   ├── CatalogView.cpp
│   │   ├── RequisicionView.h
│   │   ├── RequisicionView.cpp
│   │   ├── StatCard.h
│   │   ├── StatCard.cpp
│   │   ├── Toast.h
│   │   ├── Toast.cpp
│   │   ├── Ui.h
│   │   └── Ui.cpp
│   ├── views/
│   │   ├── LoginWindow.h
│   │   ├── LoginWindow.cpp
│   │   ├── MainWindow.h
│   │   ├── MainWindow.cpp
│   │   ├── gerente/
│   │   ├── jefe/
│   │   ├── bodega/
│   │   └── vendedor/
│   └── resources/
│       ├── resources.qrc
│       └── icons/
└── ...
```

## 4. Módulos principales

### `src/core/`

- `Theme`: paleta corporativa, estilos globales y colores por rol.
- `ApiClient`: cliente HTTP centralizado con manejo de JWT y sesión.
- `EanClassifier`: validación y clasificación de códigos EAN.
- `Models`: DTOs del dominio.

### `src/widgets/`

- `Ui`: kit visual reutilizable.
- `CatalogView`: tablas y flujos de catálogo.
- `RequisicionView`: pantallas de requisición para traslados y movimientos.
- `StatCard`: tarjetas de KPI.
- `Toast`: notificaciones de éxito/error.

### `src/views/`

Cada vista corresponde a un flujo operativo del negocio:

- `jefe/`: devoluciones, movimientos, usuarios y catálogos de dominio.
- `bodega/`: registro, proveedores, compras, inventario y traslado.
- `vendedor/`: ventas, consulta, devoluciones y traslado.
- `gerente/`: dashboard y administración.

## 5. Roles y permisos

| Rol | Código | Acceso principal |
|---|---:|---|
| Jefe de Departamento | 1 | dashboard, devoluciones, bitácora, gestión de catálogo |
| Personal de Bodega | 2 | inventario, registrar productos, proveedores, compras y traslados |
| Vendedor | 3 | ventas, consulta de existencias, devoluciones y traslado |
| Gerente | 4 | dashboard ejecutivo y administración de usuarios |

La navegación debe filtrarse por rol para no mostrar vistas no autorizadas.

## 6. Integración con la API

La comunicación con el backend pasa por `ApiClient` y se espera un esquema de respuestas tipo:

```json
{
  "success": true,
  "data": { "...": "..." },
  "message": "OK"
}
```

En caso de error, la API puede responder con payload estructurado de error y la aplicación debe reaccionar según el código HTTP.

## 7. Compilación y ejecución

### Requisitos

- CMake >= 3.16
- Qt 6 instalado con los módulos Core, Gui, Widgets y Network
- compilador compatible con C++17 o superior

### Comandos

```bash
cd /home/iron/Proyectos/Ingenieria de Software/frontend_migrado
cmake -B build -S .
cmake --build build -j$(nproc)
./build/novabook_frontend
```

## 8. Credenciales de prueba

Las credenciales de ejemplo deben validar según la base de datos real del backend. En la práctica, los usuarios de referencia son los que aparecen en los scripts y en la base de datos del proyecto, con los siguientes perfiles:

- Gerente
- Jefe de Departamento
- Personal de Bodega
- Vendedor

## 9. Convenciones de desarrollo

- Toda comunicación HTTP debe pasarse por `ApiClient`.
- No se deben escribir colores hexadecimales en bruto dentro de vistas, sino reutilizar `Theme`.
- Los identificadores grandes deben manejarse como `qint64` cuando corresponda.
- Si se agrega una nueva clase o vista, debe registrarse en `CMakeLists.txt`.
- Si se cambian rutas, permisos o payloads del backend, también debe reflejarse en el cliente.

## 10. Nota de estado

La documentación del frontend se ha actualizado para reflejar el estado real del repositorio: aplicación desktop Qt con arquitectura modular, no una SPA web ni un cliente móvil.
