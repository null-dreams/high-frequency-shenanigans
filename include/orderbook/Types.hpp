#pragma once

/**
 * @file Types.hpp
 * @brief Common type definitions and enums used across the OrderBook implementation.
 */

#include <cstdint>

namespace orderbook {

/**
 * @enum Side
 * @brief Represents the trade side of an order or price level (Buy/Bid or Sell/Ask).
 * 
 * Uses a uint8_t underlying type to minimize memory footprint.
 */
enum class Side : uint8_t  { Buy, Sell };

/**
 * @typedef OrderId
 * @brief Unique identifier for an order.
 */
using OrderId = uint64_t;

/**
 * @typedef Price
 * @brief Representation of price.
 */
using Price = uint64_t;

/**
 * @typedef Quantity
 * @brief Representation of order quantity (volume).
 */
using Quantity = uint64_t;

} // namespace orderbook