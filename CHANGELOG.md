# CHANGELOG — Frontend NovaBook

Todos los cambios relevantes del frontend se registran aquí siguiendo el formato Keep a Changelog.

## [Unreleased]

### Changed
- Actualizada la documentación para reflejar el estado real del proyecto: frontend Qt 6/C++ y no una aplicación web.
- Sincronizada la estructura de archivos con `src/core`, `src/widgets`, `src/views` y `src/resources` reales presentes en el repositorio.
- Eliminadas referencias obsoletas a una SPA web o a rutas de archivos inexistentes en este proyecto.

### Added
- Documentación de la arquitectura por roles y módulos de interfaz.
- Descripción actualizada de `ApiClient`, `Theme`, `EanClassifier`, `CatalogView` y `RequisicionView`.

## [0.1.0] - 2026-09-18

### Added
- Primera versión del frontend de escritorio Qt 6 para NovaBook.
- Implementación de dashboard, login, vistas por rol y cliente HTTP con `QNetworkAccessManager`.
- Componentes visuales reutilizables (`StatCard`, `Toast`, `Ui`, catálogo y requisiciones).
- Integración con la API REST `api_migrado`.

### Changed
- Ajustes de tema visual y navegación por roles.
- Estructuración modular del frontend por dominio de negocio.
