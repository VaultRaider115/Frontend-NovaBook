#pragma once

// Límites de largo de los campos de texto que captura el usuario.
// Deben coincidir con la API (API-Novabook/src/limites.rs), que también los
// valida y responde con un mensaje claro si llegan más largos. Si cambias uno,
// cambia el otro.
namespace Limites {

inline constexpr int kNombreCliente = 60;   // Punto de Venta: nombre del cliente
inline constexpr int kBusqueda = 100;       // búsqueda de productos (título, autor, EAN o SKU)

} // namespace Limites
