# Diseños de Figma y su implementación en Qt

Las pantallas de este cliente Qt siguen los diseños exportados de Figma que
están en `../docs/disenos/` (`1.png` … `21.png`). Este documento indica:

- qué vista implementa cada diseño;
- la paleta real del tema;
- lo que se agregó o cambió respecto al diseño, y por qué;
- las pantallas que no tienen diseño.

## 1. Mapa de diseños

| # | Diseño | Vista Qt | Rol(es) |
|---:|---|---|---|
| 1 | Catálogo y Gestión de Autores | `bodega/BodegaAutoresView` | Jefe |
| 2 | Consulta de Existencias y Disponibilidad | `vendedor/VendedorConsultaView` | Vendedor |
| 3 | Punto de Venta — Registro de Venta al Mostrador | `vendedor/VendedorVentasView` | Vendedor |
| 4 | Gestión de Devoluciones a Proveedor | `vendedor/VendedorDevolucionView` | Vendedor |
| 5 | Requisición de Entrada Venta > Bodega | `vendedor/VendedorTrasladoView` (`widgets/RequisicionView`) | Vendedor |
| 6 | Recepción de Mercancía — Compras a Proveedor | `bodega/BodegaComprasView` | Bodega |
| 7 | Historial de Movimientos (bitácora) | `jefe/JefeBitacoraView` | Jefe |
| 8 | Requisición de Salida Bodega > Venta | `bodega/BodegaTrasladoView` (`widgets/RequisicionView`) | Bodega |
| 9 | Alta de Mercancía en Almacén (libro y revista) | `bodega/BodegaRegistroView` | Bodega |
| 10 | Login | `LoginWindow` | todos |
| 11 | Diálogo Registrar Nuevo Autor | `bodega/BodegaAutoresView` | Jefe |
| 12 | Administración y Gestión de Usuarios | `gerente/GerenteUsuariosView`, `jefe/JefeUsuariosView` | Gerente, Jefe |
| 13 | Diálogo Registrar Nuevo Empleado | `gerente/GerenteUsuariosView` | Gerente |
| 14 | Catálogo y Gestión de Géneros | `jefe/JefeGenerosView` | Jefe |
| 15 | Diálogo Registrar Nuevo Género | `jefe/JefeGenerosView` | Jefe |
| 16 | Catálogo y Gestión de Motivos | `jefe/JefeMotivosView` | Jefe |
| 17 | Diálogo Registrar Nuevo Motivo | `jefe/JefeMotivosView` | Jefe |
| 18 | Catálogo y Gestión de Muebles y Estantería | `jefe/JefeMueblesView` | Jefe |
| 19 | Diálogo Registrar Nuevo Mueble | `jefe/JefeMueblesView` | Jefe |
| 20 | Historial y Aprobación de Devoluciones a Proveedor | `jefe/JefeDevolucionesView` | Jefe (el Gerente la ve en solo lectura) |
| 21 | Acta de Devolución a Proveedor (PDF) | la genera la API (`API-Novabook/src/services/pdf.rs`) | Vendedor, Jefe, Gerente |

Todas se revisaron con capturas contra su imagen. En particular:

- **Diseños 1–20:** etiqueta superior (eyebrow) y título de cada página,
  chips completos en las celdas, etiquetas de campo en mayúsculas espaciadas,
  iniciales en los avatares, y flechas de fecha y cantidad.
- **Diseño 21:** autor o edición bajo el título, EAN en Courier y alineación
  a la derecha exacta.

## 2. Paleta (`src/core/Theme.h`)

| Token | Color | Uso |
|---|---|---|
| `BgDark` / `BgInput` | `#0D0D0D` | fondo de pantalla e inputs |
| `BgSidebar` | `#111113` | menú lateral |
| `BgHeader` | `#141416` | cabecera de tabla |
| `BgCard` | `#18181B` | tarjetas y tablas |
| `BgRowAlt` / `BgLabel` | `#1F1F23` | fila alterna y etiqueta lateral |
| `Border` | `#27272A` | bordes |
| `Accent` / `BorderFocus` | `#D4A346` | acento dorado, foco |
| `AccentHover` | `#E0B865` | hover del acento |
| `AccentDim` | `#8A6A24` | acento apagado |
| `TextPrimary` | `#FAFAFA` | texto |
| `TextMuted` / `TextSidebar` | `#A1A1AA` | texto secundario |
| `TextFaint` | `#71717A` | placeholders |
| `Success` / `SuccessBg` | `#00D492` / `#0B2A20` | éxito |
| `Error` / `ErrorBg` | `#FF6467` / `#2A1214` | error |
| `Warning` | `#FFD230` | aviso (fondo del Toast `#2A2510`) |
| `Libro` / `Revista` | `#00D492` / `#00BCFF` | chips de tipo |

**Colores por rol** (insignia del menú):

| Rol | Nombre del color | Hex |
|---|---|---|
| Jefe | índigo | `#A3B3FF` |
| Bodega | ámbar | `#FFD230` |
| Vendedor | menta | `#5EE9B5` |
| Gerente | dorado | `#D4A346` |

Regla: las vistas toman los colores de `Theme` o de los componentes de
`widgets/Ui`; no escriben colores sueltos.

## 3. Diferencias con el diseño (a propósito)

Se agregaron cosas que piden las reglas de negocio o el diagrama de procesos
(`../docs/NovaBook-Diagrama-de-procesos-del-sistema.png`) y que el diseño no
muestra.

- **Bitácora (7)**
  - Categorías extra con su color: DEVOLUCIÓN ámbar, INVENTARIO rosa,
    USUARIOS dorado, CATÁLOGO índigo, SISTEMA gris. El diseño solo tiene
    VENTA verde, TRASLADO azul, COMPRA morado y ACCESO gris.
  - Los accesos fallidos van en rojo.
  - La acción técnica aparece como tooltip del chip.
- **Requisiciones (5 y 8)**
  - El pie muestra la editorial del documento (una sola editorial por
    requisición).
  - Al registrar se imprime el PDF con folio `REQ-AAAA-n`.
- **Ventas (3):** método de pago.
- **Compras (6)**
  - Número de factura.
  - Campos "Facturadas" y "Llegaron" (diagrama, paso 1); la columna
    "Llegaron / Fact." marca en ámbar lo que faltó.
- **Alta (9)**
  - Sin campo Cantidad: la cantidad entra con la compra.
  - Mueble de bodega y editorial reales.
  - EAN-13 y SKU de 6 a 7 dígitos validados al capturar.
- **Usuarios (12 y 13)**
  - El rol se bloquea al editarse a uno mismo.
  - El Jefe no registra usuarios ni modifica cuentas de Gerente (aparecen
    como "Solo Gerente").
  - Teléfono solo con dígitos.
- **Catálogos (1, 14, 16, 18):** el ícono de borrar se deshabilita cuando el
  registro no se puede eliminar.
- **Devoluciones del Jefe (20)**
  - Calendario con rango legible.
  - Nota obligatoria al rechazar.
  - Botón para el acta aprobada.
- **Acta PDF (21)**
  - Columnas COSTO y P. VENTA con sus importes (diagrama, paso 8).
  - Pie con quién solicitó y quién autorizó.
  - Un acta por tipo de producto (libros o revistas).
- **Avisos (`Toast`):** fondo y borde del color del aviso, arriba al centro,
  hasta 560 px de ancho.

## 4. Pantallas sin diseño

Se construyeron con el mismo kit (`widgets/Ui`, `CatalogView`, `StatCard`) para
que se vean como las demás:

- Panel Principal de cada rol (`*DashboardView`).
- Proveedores (`bodega/BodegaProveedoresView`): Jefe (CRUD) y Bodega (solo
  alta).
- Consultar Inventario de Bodega (`bodega/BodegaInventarioView`).
- Devolución de Cliente (`vendedor/VendedorDevolucionClienteView`).
- Mis Devoluciones (`vendedor/VendedorMisDevolucionesView`).
- Recepción de Salida del Vendedor y Seguimiento de Salidas de Bodega
  (`vendedor/VendedorRecepcionView`).
- Incidencias de Compra (`bodega/BodegaIncidenciasView`).
- Inventario Mensual:
  - conteo del Vendedor (`vendedor/VendedorInventarioView`);
  - revisión del Jefe y solicitudes de editorial (`jefe/JefeInventarioView`).
- Devoluciones a Proveedor del Gerente: reutiliza `JefeDevolucionesView`
  con `puedeAprobar = false`.

## 5. Al agregar o cambiar una pantalla

1. Si tiene diseño, compárala con su imagen en `../docs/disenos/` (tabla de
   arriba).
2. Usa `Theme` y los componentes de `widgets/Ui` (`pageHeader`, `table`,
   `chip`, `field`, `FormDialog`…).
3. Si te apartas del diseño, anótalo en la sección 3 con el motivo.
4. Registra el cambio en `CHANGELOG.md`.
