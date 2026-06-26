#pragma once

/**
 * @file OrderBook.hpp
 * @brief Definition of the OrderBook class which manages buy and sell order hierarchies and execution.
 */

#include "Types.hpp"
#include "Order.hpp"
#include "Limit.hpp"
#include "ObjectPool.hpp"

#include <map>
#include <unordered_map>

namespace orderbook {

/**
 * @class OrderBook
 * @brief Represents a Limit Order Book (LOB) for financial matching engines.
 * 
 * OrderBook implements price-time priority (FIFO matching) for bids and asks:
 * - Bids (Buy orders) are sorted in descending order (highest price level matched first).
 * - Asks (Sell orders) are sorted in ascending order (lowest price level matched first).
 * 
 * It supports O(1) order additions (when not crossing or once a price level exists),
 * O(log N) price level lookups (where N is the number of active price levels),
 * and O(1) order cancellations via a direct hash registry.
 */
class OrderBook {
public:
    /**
     * @brief Constructs an empty OrderBook.
     */
    explicit OrderBook(size_t order_pool_capacity = 1000000);

    /**
     * @brief Destructs the OrderBook, reclaiming all internal structures.
     */
    ~OrderBook() = default;

    // Copy operations are disabled to prevent accidental performance overhead and state cloning.
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;
    
    // Move operations are allowed.
    OrderBook(OrderBook&&) noexcept = default;
    OrderBook& operator=(OrderBook&&) noexcept = default;

    /**
     * @brief Adds a new order to the book and attempts to match it against resting orders.
     * 
     * If the incoming order matches existing resting orders on the opposite side of the book,
     * trades are executed up to the incoming order's quantity or until prices no longer cross.
     * Any remaining unexecuted quantity is placed on the book as a resting order.
     * Duplicate order IDs are ignored.
     * 
     * @param id The unique identifier for the new order.
     * @param price The limit price of the order.
     * @param qty The quantity to buy/sell.
     * @param side The side (Buy/Sell) of the order.
     */
    void add_order(OrderId id, Price price, Quantity qty, Side side);

    /**
     * @brief Cancels an existing order, removing it from the order book.
     * 
     * This operation executes in O(1) time using the order registry to locate
     * and erase the order directly from its list level without traversing the book.
     * If the ID is not found, the operation returns silently.
     * 
     * @param id The unique identifier of the order to cancel.
     */
    void cancel_order(OrderId id);
    
    /**
     * @brief Retrieves the aggregate volume (quantity) resting at a specific price level.
     * 
     * Runs in O(log N) time, where N is the number of active price levels.
     * 
     * @param price The price level to query.
     * @param side The side of the book to check.
     * @return The total active quantity resting at that price level, or 0 if empty.
     */
    [[nodiscard]] Quantity get_volume_at_price(Price price, Side side) const;

    /**
     * @brief Returns the total number of active orders resting in the book.
     * 
     * Runs in O(1) time.
     * 
     * @return The number of resting orders in the registry.
     */
    [[nodiscard]] std::size_t size() const noexcept;

private:
    /// Bids sorted in descending order (highest price first).
    std::map<Price, Limit, std::greater<Price>> bids_;

    /// Asks sorted in ascending order (lowest price first).
    std::map<Price, Limit, std::less<Price>> asks_;

    /**
     * @struct OrderRecord
     * @brief Metadata stored in the registry to locate a resting order in O(1) time.
     */
    struct OrderRecord {
        Order* order_ptr;       ///< Pointer to the order's node in the Limit's doubly-linked list
        Side side;              ///< Side of the book the order is resting on
        Price price;            ///< Price level the order is located at
    };

    /// Registry mapping OrderId to its metadata record for rapid cancellation lookups.
    std::unordered_map<OrderId, OrderRecord> order_registry_;
    OrderPool pool_;

    /**
     * @brief Internal helper to match an incoming order against existing resting orders.
     * 
     * Traverses the opposite side of the book starting from the best available price.
     * Fills the incoming order and resting orders under price-time priority.
     * 
     * @param incoming_order Reference to the incoming order to match.
     */
    void match_order(Order& incoming_order);
};

} // namespace orderbook