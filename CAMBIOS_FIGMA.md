# Cambios de UI según Figma "Proyecto S"

## Nuevos archivos
- `src/widgets/Ui.{h,cpp}` — kit de componentes (tarjeta, encabezado de página, chips, avatar, stepper, tabla, `FormDialog`).
- `src/widgets/CatalogView.{h,cpp}` — plantilla de catálogos (buscador, total, botón nuevo, tabla, paginador `PagerBar`).
- `src/widgets/RequisicionView.{h,cpp}` — pantalla de requisición por lista (base de Bodega→Piso y Piso→Bodega).
- `src/views/jefe/Jefe{Generos,Motivos,Muebles}View.*` — catálogos nuevos con su formulario (CRUD).
- `src/resources/resources.qrc` + `icons/*.svg` — iconos vectoriales.

## Rutas nuevas
- Menú de Jefe/Gerente: Autores, Géneros, Motivos, Muebles y Estantería (además de lo existente).
- `ApiClient`: `/api/generos` (POST/PUT/DELETE), `/api/motivos` (GET/POST/PUT/DELETE), `/api/muebles` (GET/POST/PUT/DELETE).
  El backend `api_migrado` no venía en el zip: rutas y nombres de campos son supuestos y hay que confirmarlos.
