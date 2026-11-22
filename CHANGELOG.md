# CHANGELOG — Frontend NovaBook

Todos los cambios relevantes del frontend se registran aquí siguiendo el formato Keep a Changelog.

## [Unreleased]

Rama `integracion-validaciones`: pantallas iguales a los diseños de
`docs/disenos/` y conectadas a la API del esquema unificado.

### Added
- Cambios del hito 2 (21-nov, rama `hito2-cambios`):
  - **Compras a Proveedor**: al añadir, el costo unitario no puede pasar
    del precio de venta del producto (aviso con ambos importes); el alta
    hecha desde Compras entra por el mismo camino y también se valida.
  - **Modificar partida** (lápiz junto al bote, antes de registrar):
    cambia facturadas, llegaron y costo sin quitar y volver a añadir; aplica
    la misma validación del precio.
  - **Requisición Bodega → Piso (y Piso → Bodega)**: las cantidades no
    dejan teclear 0 (`Ui::SpinSinCero`); al borrar todo regresa al valor
    anterior.
  - **Subtotal de la compra = lo que llegó** (como la API): antes la tabla
    multiplicaba por las facturadas (3 de 5 a $130 mostraba $650 y se
    registraban $390). Si faltan piezas, el total dice "Total a registrar"
    con la factura completa entre paréntesis.
  - **Tickets de Venta** (Jefe, nueva opción del menú): lista por rango de
    fechas y búsqueda por ticket, cliente o vendedor, con total del rango;
    el ojo o doble clic abre el ticket con lo vendido y lo ya devuelto.
- Revisión del 19-nov (3a):
  - **Historial de Movimientos sin páginas**: la tabla se recorre con scroll
    y, al llegar al final, pide al servidor los siguientes 50 eventos
    ("Mostrando N de M eventos · baja para ver más").
  - **Campos de EAN o título** (`Ui::busquedaProducto`, también en la
    consulta de inventario): título de hasta 30 caracteres; con prefijo de
    EAN o solo dígitos, máximo 13 (el contador cambia de "/30" a "/13").
  - **Lista + detalle en una sola vista con scroll** (`SeccionesScroll`):
    Incidencias de Compra, Seguimiento de Salidas, Recepción de Salida,
    Devolución de Cliente e Inventario Mensual del Vendedor; al abrir un
    registro la vista baja sola a su detalle.
  - **PDFs**: todos se guardan solos en Descargas\NovaBook (sin preguntar;
    si el nombre existe se numera) y se abren (`Documentos::guardarYAbrir`).
  - **Límites de caracteres** en todos los campos de texto, con contador
    (`Ui::conLimite`, también para texto de varias líneas) y constantes en
    `core/Limites.h`; los rechazos de devolución e inventario usan
    `Ui::pedirTexto` (tope 200 con contador).
  - **Recepción de mercancía**: un EAN fuera del catálogo no entra a la
    compra; aparece "Agregar nuevo libro", que abre el alta en una ventana
    con el EAN puesto y, al guardar, lo añade con la cantidad y el costo
    capturados. El costo debe ser mayor a $0 y el título del proveedor de la
    factura.
  - **Devoluciones a Proveedor**: la editorial se valida al pulsar "Añadir"
    (columna "Editorial" Validada / Pendiente / No es de esta editorial, y
    estado de la lista); al cambiar de proveedor se vuelve a validar y no se
    registra si algo no pasó.
  - **Inventario (Bodega y Vendedor)**: los géneros van en una fila con
    scroll horizontal; no se corta ninguno.
  - **Historial de Movimientos**: el rango y el tipo de evento se conservan
    aunque se cierre sesión y se vuelva a entrar (solo "Hoy" los reinicia).
- Revisión del 19-nov:
  - **Inventario Mensual (Vendedor)**: apartado "Para devolución" con el
    contador general y "X de Y encontrados" por título, en tiempo real; los
    títulos con faltantes se resaltan en rojo.
  - **Punto de Venta**: límite de caracteres con contador (cliente 60,
    búsqueda 100; `core/Limites.h`); recuadro con el libro (título, autor,
    editorial, ISBN, precio y stock); autor e ISBN en cada línea del carrito
    y en el ticket; al añadir valida que exista y que alcance el stock de piso.
  - **Una sola vista con scroll** en lugar de pestañas en Compras a Proveedor
    e Inventario Mensual del Jefe (`widgets/SeccionesScroll`), con anclas fijas.
  - **Historial de Movimientos**: botón "Hoy"; el rango elegido se conserva
    al cambiar el tipo de evento o de pantalla.
- Validaciones de Ian (17-nov, PR #5) y ajustes de Federico (18-nov):
  - Solo dígitos en el número de empleado, factura, teléfono de proveedor y
    código de mueble; solo letras en nombres de usuarios, autores, géneros,
    motivos y cliente de la venta.
  - Proveedores: dirección en calle, número, colonia y ciudad. **Bodega solo
    consulta** (se ocultó "Registrar Proveedor"; el equipo decidió mantenerlo
    así el 19-nov).
  - Bodega: "Inventario General" usa la vista del Vendedor; el Alta avisa si
    el libro ya está con ese proveedor; la requisición deja ajustar la
    cantidad y el error muestra el título.
  - Punto de Venta sin "Transferencia" y con desplazamiento; títulos de
    pantallas y menús ("Piso de Venta", "Inventario General", "Control de
    Devoluciones a Proveedor"); "Otra" al final de las nacionalidades.
  - Federico: dependencia `Qt6::Svg` y el número visible en los botones +/−.
- Cambios pedidos por el profesor (18-nov):
  - **Búsqueda por título** en los campos de EAN (`ProductoBuscador`): Compras,
    Punto de Venta, Devoluciones a Proveedor y de Cliente, Requisiciones e
    Inventario Mensual. Con letras sugiere los productos que coinciden (título,
    EAN, editorial y stock en piso y bodega); al elegir uno deja su EAN. Enter
    con un solo resultado lo agrega. En Compras y Devoluciones solo sugiere
    títulos del proveedor elegido. El Alta sigue pidiendo el EAN (es un
    producto nuevo).
  - **Requisiciones: validación al añadir a la lista**, no al registrar: el
    producto debe existir y la cantidad (sumando lo que ya está en la lista)
    no puede pasar de lo que hay en bodega (salida) o en piso (entrada). La
    cantidad editable en la tabla tampoco pasa de ese máximo.
- Los campos de EAN aceptan solo dígitos y máximo 13 (`Ui::eanInput`) en
  Compras, Alta, Ventas, Devoluciones (proveedor y cliente), Requisiciones e
  Inventario Mensual.
- Compras a Proveedor: el número de factura lleva el prefijo fijo `F-` y solo
  se escribe el número (si escriben "F-12" no se repite).
- **Facturas registradas** (Compras a Proveedor): segunda sección con las
  facturas ya capturadas (factura, proveedor, fecha, quién la registró,
  títulos, piezas y total), buscador por factura, proveedor, título o EAN, y
  el icono del ojo (o doble clic) con lo que se compró en cada una. Usa
  `GET /api/bodega/compras`; `CompraResponse` ahora lee `factura`.
- Revisión del equipo (16-nov):
  - **Aprobación de Devoluciones**: icono del ojo con la lista de lo que se
    devuelve (EAN, título, piezas, motivo); aprobar o rechazar se confirma
    viendo esa lista.
  - **Muebles**: el código lleva el prefijo fijo del área (`P-` piso, `B-`
    bodega) y solo se escribe el número; la capacidad deja escribir menos de
    lo ocupado y, al guardar, explica el error con las piezas que tiene.
  - **Inventario Mensual (Jefe)**: la solicitud de editorial muestra, al
    escribir los EAN, cada título con su stock, "N títulos · M piezas por
    regresar" y avisa de EAN inválidos, inexistentes o de otra editorial (no
    deja registrar así). La tabla muestra títulos y piezas, y el ojo los lista.
  - **Inventario Mensual (Vendedor)**: lo que va a devolución sale en rojo
    con "Separar · pidió la editorial" o "Separar · añejado" y el pie cuenta
    los títulos por separar.
  - **Alta de Mercancía**: el precio empieza en $0.00 y no deja registrar
    sin capturarlo.
- **Compra incompleta** (diagrama, paso 1):
  - Compras a Proveedor captura "Facturadas" y "Llegaron"; la tabla marca en
    ámbar lo que faltó.
  - Si la API abre una incidencia, la app ofrece abrir el programa de correo
    del equipo con el aviso al proveedor (`Documentos::abrirCorreo`, mailto:).
    Si el proveedor no tiene correo o no hay programa de correo, copia el
    texto al portapapeles.
  - Pantalla nueva **Incidencias de Compra** (Bodega): faltantes por
    factura, reenviar el aviso y registrar lo que llega después.
- **Recepción de la salida** (pasos 2 y 3):
  - Pantalla nueva **Recepción de Salida** (Vendedor): por cada salida
    pendiente se captura lo recibido y el mueble de piso de cada libro. Si
    algo no coincide, la requisición se corrige y se reimprime.
  - Pantalla nueva **Seguimiento de Salidas** (Bodega, solo lectura): estado
    de cada salida y reimpresión.
  - El PDF de la requisición recibida lleva columna MUEBLE, la cantidad
    original tachada donde hubo diferencia, el sello CORREGIDA y quién
    recibió.
- **Cierre de sesión**:
  - Automático cuando la API responde 401 (token vencido o usuario dado de
    baja).
  - Por inactividad, con `InactivityWatcher`; el tiempo se configura en
    `INACTIVIDAD_MINUTOS` (15 por defecto).
- **PDFs** (módulo `Documentos`): se guardan en `Documentos/NovaBook` y se
  abren en el visor del sistema para imprimir.
  - La requisición se genera con `QPdfWriter`.
  - El acta de devolución se descarga de la API.
- **Requisiciones con folio** `REQ-AAAA-n`:
  - Un solo documento, todo o nada; se imprime al registrarla.
  - Una sola editorial por documento: bloquea al agregar un artículo de
    otra y muestra la editorial en el pie y en el PDF.
- **Vendedor**:
  - **Mis Devoluciones**: estado, acta aprobada y motivo del rechazo.
  - **Devolución de Cliente**: busca el ticket, marca las piezas (con o sin
    defecto) y permite reembolso o cambio por igual o mayor valor.
  - **Inventario Mensual**: abrir, escanear (cada escaneo suma 1), corregir
    y cerrar el conteo.
- **Jefe**: revisión del inventario mensual (aprobar o rechazar con motivo)
  y registro de las solicitudes de editorial.
- **Gerente**: **Devoluciones a Proveedor** en solo lectura (consulta y
  descarga de actas; aprobar es solo del Jefe).
- **Bitácora**:
  - Paginación en el servidor (5 filas) y filtro por categoría.
  - Columna única "Descripción" con chips de color: VENTA verde, TRASLADO
    azul, COMPRA morado, ACCESO gris, DEVOLUCIÓN ámbar, INVENTARIO rosa,
    USUARIOS dorado, CATÁLOGO índigo; los accesos fallidos van en rojo.
- **Ventas**: método de pago. **Compras**: número de factura.

### Changed
- **Revisión visual** contra `docs/disenos/` (1–21) en Login, Consulta,
  Ventas, Devoluciones, Requisiciones, Compras, Alta, Bitácora, Aprobación,
  Autores, Géneros, Motivos, Muebles, Usuarios y sus diálogos.
  - `pageHeader` muestra la etiqueta superior (eyebrow) correcta.
  - Chips y avatares completos y sin recortes en las celdas
    (`Ui::fitColumn`).
  - Etiquetas de campo en mayúsculas espaciadas.
  - Se arreglaron las flechas de fecha y cantidad.
- **Alta de mercancía**:
  - Sin campo Cantidad.
  - Mueble de bodega y editorial reales.
  - Validación de EAN-13 (978/979 libros, 977 revistas) y de SKU de 6 a 7
    dígitos.
- **Menús de catálogos** según el diagrama de procesos:
  - Autores, géneros, motivos y muebles: solo el Jefe.
  - Proveedores y editoriales: el Jefe; Bodega solo da de alta.
- **Usuarios** (Gerente y Jefe), con las mismas reglas que la API:
  - Trim en los nombres; teléfono solo con dígitos.
  - Guardar se deshabilita mientras responde la API.
  - Nadie edita su propio rol.
  - El Jefe no toca roles ni cuentas de Gerente.
- **Catálogos**: el ícono de borrar se deshabilita cuando un registro no se
  puede eliminar.
- **Calendarios y rangos de fecha** legibles en la bitácora y en el
  historial de devoluciones.
- **Devoluciones**:
  - El motivo se envía por id.
  - La nota del Jefe se lee de `devolucion.observaciones`.
- **Existencias de Bodega**: la consulta usa `/api/bodega/existencias`, de
  modo que la requisición de salida ya muestra los títulos.
- **`StatCard`**: el acento por defecto se toma de `Theme::Accent`; antes
  era el dorado viejo `#C8973A`.
- **Documentación**:
  - `README.md` reescrito: estructura real, menús por rol, configuración
    (`API_URL`, `INACTIVIDAD_MINUTOS`), sesión, PDFs, compilación en Windows
    y Linux.
  - `CAMBIOS_FIGMA.md` reescrito:
    - mapa de los diseños 1–21 a su vista Qt;
    - paleta real de `Theme.h` (acento `#D4A346`) y colores por rol;
    - diferencias con el diseño hechas a propósito;
    - pantallas sin diseño.
  - `AI_CONTEXT.md` nuevo: contexto técnico del cliente (`ApiClient`, kit
    `Ui`, patrón de vistas, errores conocidos de Qt).
  - `CHANGELOG.md` con los cambios de esta rama.

### Fixed
- **Avisos (`Toast`)**: no pintaban su fondo y quedaban transparentes
  encima de la pantalla; ahora toman el ancho del texto (hasta 560 px).
- **Requisición**: al agregar un artículo se usa el renglón exacto del EAN
  y no el primer resultado de la búsqueda.
- **Cantidades en tablas**: los QSpinBox ya no se ven recortados dentro de
  las celdas.

## [0.1.0] - 2026-09-18

### Added
- Primera versión del frontend de escritorio Qt 6 para NovaBook.
- Implementación de dashboard, login, vistas por rol y cliente HTTP con `QNetworkAccessManager`.
- Componentes visuales reutilizables (`StatCard`, `Toast`, `Ui`, catálogo y requisiciones).
- Integración con la API REST `api_migrado`.

### Changed
- Ajustes de tema visual y navegación por roles.
- Estructuración modular del frontend por dominio de negocio.
