#pragma once

// Límites de los datos que captura el usuario (largo de los textos y montos).
// Deben coincidir con la API (API-Novabook/src/limites.rs), que también los
// valida y responde con un mensaje claro si llegan más largos. Si cambias uno,
// cambia el otro. En pantalla se aplican con Ui::contadorCaracteres /
// Ui::conLimite (tope al escribir y contador "12/60").
namespace Limites {

// Punto de venta y búsquedas
inline constexpr int kNombreCliente = 60;   // nombre del cliente
inline constexpr int kBusqueda = 100;       // buscadores de catálogos (autores, usuarios, facturas…)
inline constexpr int kBusquedaProducto = 30; // campos de EAN o título y consulta de inventario (API: BUSQUEDA_MAX)
inline constexpr int kEan = 13;              // con prefijo de EAN (977/978/979) solo 13 dígitos

// Motivos (catálogo y notas)
inline constexpr int kMotivoNombre = 50;          // motivo.nombre_motivo
inline constexpr int kDescripcion = 200;          // descripción del motivo y del mueble
inline constexpr int kObservaciones = 200;        // observaciones / motivo de aprobar o rechazar
inline constexpr int kObservacionesRequisicion = 150; // requisición y su recepción
inline constexpr int kReferenciaSolicitud = 150;  // solicitud de editorial

// Muebles
inline constexpr int kMuebleCodigo = 10;    // P-… / B-… (con el prefijo)
inline constexpr int kMuebleNombre = 50;

// Catálogos
inline constexpr int kGenero = 45;
inline constexpr int kAutorNombre = 50;
inline constexpr int kAutorApellidos = 100;
inline constexpr int kEditorialNombre = 100;
inline constexpr int kRfc = 13;
inline constexpr int kProveedorNombre = 50;
inline constexpr int kTelefono = 15;
inline constexpr int kCorreo = 100;
inline constexpr int kCalle = 60;          // calle, número, colonia y ciudad se guardan
inline constexpr int kNumero = 10;         // juntos en proveedor.direccion (200)
inline constexpr int kColonia = 50;
inline constexpr int kCiudad = 50;
inline constexpr int kContacto = 100;

// Productos y compras
inline constexpr int kTitulo = 150;
inline constexpr int kPeriodicidad = 30;
inline constexpr int kFactura = 28;        // + "F-" = 30 (compras_proveedor.factura)
inline constexpr double kMontoMax = 99999.99; // precio o costo más alto

// Usuarios
inline constexpr int kUsuarioNombre = 50;
inline constexpr int kUsuarioApellido = 30;
inline constexpr int kPassword = 16;

} // namespace Limites
