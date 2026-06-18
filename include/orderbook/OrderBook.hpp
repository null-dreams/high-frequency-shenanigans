#pragma once

#include "Types.hpp"
#include "Order.hpp"
#include "Limit.hpp"

#include <map>
#include <unordered_map>

namespace orderbook {

class OrderBook {
public:
    OrderBook() = default;
    ~OrderBook() = default;

    // Disable copying to prevent accidental overhead or state inconsistency
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;
    OrderBook(OrderBook&&) noexcept = default;
    OrderBook& operator=(OrderBook&&) noexcept = default;

    // Core Public API
    void add_order(OrderId id, Price price, Quantity qty, Side side);
    void cancel_order(OrderId id);
    
    // Read-only helpers for inspection or testing
    [[nodiscard]] Quantity get_volume_at_price(Price price, Side side) const;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    // Sorted price levels
    std::map<Price, Limit, std::greater<Price>> bids_; // Highest buy price first
    std::map<Price, Limit, std::less<Price>> asks_;    // Lowest sell price first

    // Structure to track the location of an order for O(1) cancellations
    using OrderIterator = std::list<Order>::iterator;
    struct OrderRecord {
        OrderIterator iterator;
        Side side;
        Price price;
    };

    std::unordered_map<OrderId, OrderRecord> order_registry_;

    // Internal helper for order matching
    void match_order(Order& incoming_order);
};

} // namespace orderbook