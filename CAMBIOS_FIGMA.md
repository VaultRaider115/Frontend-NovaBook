# Cambios visuales y evolución del frontend

Este documento describe la evolución actual del frontend de escritorio de NovaBook y deja claro que la implementación real del proyecto no corresponde a una web SPA, sino a una aplicación Qt 6 con vistas nativas.

## 1. Estado real del proyecto

La interfaz actual se construye sobre widgets y vistas nativas del framework Qt. La estructura principal es:

- `src/core/Theme.*`: identidad visual y estilos
- `src/widgets/Ui.*`: kit reutilizable de componentes UI
- `src/widgets/CatalogView.*`: layouts estándares para listados
- `src/widgets/RequisicionView.*`: pantallas de traspasos y requisiciones
- `src/views/...`: vistas por rol y flujo operativo

La referencia visual se ha mantenido en un estilo corporativo oscuro, con acentos dorados y diferenciación por rol.

## 2. Paleta visual actual

- Fondo base: `#0D0D0D`
- Tarjetas: `#161616`
- Inputs: `#1F1F1F`
- Acento principal: `#C8973A`
- Acento hover: `#E0AE52`
- Error: `#E05C5C`
- Éxito: `#5CBF8A`
- Advertencia: `#E0B84A`

Los roles se diferencian con colores:

- Jefe / Gerente: dorado
- Bodega: azul
- Vendedor: verde

## 3. Componentes clave agregados o reutilizados

### `src/widgets/Ui.*`

Kit de componentes visuales reutilizables, útil para mantener consistencia entre formularios, tablas y paneles.

### `src/widgets/CatalogView.*`

Componente para listar catálogos con acciones frecuentes como búsqueda, filtros, estructura de tabla y acciones de edición.

### `src/widgets/RequisicionView.*`

Pantallas para movimientos internos, traslado entre ubicaciones y flujos de requisición.

### Vistas por módulo

- `views/jefe/`: catálogo, bitácora, devoluciones y dashboard
- `views/bodega/`: registro, compras, proveedores, inventario y traslado
- `views/vendedor/`: ventas, consulta, devoluciones y traslados
- `views/gerente/`: dashboard y administración de usuarios

## 4. Diferencia con una SPA web

No existe una carpeta `web/` ni una aplicación web embebida dentro del proyecto. La referencia de implementación persistente es la aplicación de escritorio Qt. Por tanto, cualquier documentación que sugiera una SPA, un front web o una app en HTML/JS debe considerarse histórica o incompleta.

## 5. Recomendación para mantenimiento

Cuando se agreguen nuevas pantallas o mejoras visuales:

- mantener la paleta en `Theme`;
- preferir componentes reutilizables en `src/widgets/`;
- no duplicar estilos inline;
- registrar cambios en `frontend_migrado/CHANGELOG.md`.
