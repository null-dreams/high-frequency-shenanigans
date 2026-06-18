#pragma once

/**
 * @file Order.hpp
 * @brief Definition of the Order struct representing an active or incoming trade order.
 */

#include "Types.hpp"

namespace orderbook {

/**
 * @struct Order
 * @brief Represents an individual order in the system.
 * 
 * Contains all primary attributes of an order including its unique ID,
 * limit price, remaining quantity, and side (Buy/Sell).
 */
struct Order {
    OrderId id;        ///< Unique identifier for the order
    Price price;       ///< Limit price of the order
    Quantity quantity; ///< Remaining quantity to be matched
    Side side;         ///< Order side (Buy or Sell)

    /**
     * @brief Constructs an Order with the given parameters.
     * 
     * @param oid The unique identifier for the order.
     * @param p The limit price of the order.
     * @param q The quantity of the order.
     * @param s The side of the order (Buy/Sell).
     */
    Order(OrderId oid, Price p, Quantity q, Side s)
        : id(oid), price(p), quantity(q), side(s) {}
};

} // namespace orderbook