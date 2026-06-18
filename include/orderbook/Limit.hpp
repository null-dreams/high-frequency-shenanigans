#pragma once

/**
 * @file Limit.hpp
 * @brief Definition of the Limit struct representing a single price level (limit level) in the order book.
 */

#include "Types.hpp"
#include "Order.hpp"
#include <list>

namespace orderbook {

/**
 * @struct Limit
 * @brief Represents a single price level in the order book.
 * 
 * Aggregates all orders at the same price. Standard price-time priority
 * (FIFO) is maintained by storing orders in a doubly-linked list (`std::list`).
 * It also keeps a cache of the total quantity resting at this price level
 * for fast volume lookups.
 */
struct Limit {
    Price price;                    ///< The price level of this limit
    Quantity totalQuantity{0};      ///< Aggregate quantity of all active orders at this price
    std::list<Order> orders;        ///< Queue of orders at this price level (FIFO order)

    /**
     * @brief Constructs a Limit level for a specific price.
     * 
     * @param price The price value for this level.
     */
    explicit Limit(Price price) : price(price), totalQuantity(0) {}
};

} // namespace orderbook