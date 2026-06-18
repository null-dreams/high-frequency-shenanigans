#include "orderbook/OrderBook.hpp"

namespace orderbook {

void OrderBook::add_order(OrderId id, Price price, Quantity qty, Side side) {
    // Prevent duplicate OrderIds
    if (order_registry_.find(id) != order_registry_.end()) {
        return; 
    }

    Order incoming{id, price, qty, side};

    // 1. Attempt to match the incoming order against resting orders
    match_order(incoming);

    // 2. If there is remaining quantity, insert the order into the book
    if (incoming.quantity > 0) {
        if (side == Side::Buy) {
            // try_emplace avoids requiring a default constructor for Limit
            auto [map_it, inserted] = bids_.try_emplace(price, price);
            Limit& limit = map_it->second;
            
            limit.totalQuantity += incoming.quantity;
            limit.orders.push_back(incoming);
            
            // Register the order with an iterator to its position in the list
            order_registry_.try_emplace(id, OrderRecord{--limit.orders.end(), side, price});
        } else {
            auto [map_it, inserted] = asks_.try_emplace(price, price);
            Limit& limit = map_it->second;
            
            limit.totalQuantity += incoming.quantity;
            limit.orders.push_back(incoming);
            
            order_registry_.try_emplace(id, OrderRecord{--limit.orders.end(), side, price});
        }
    }
}

void OrderBook::match_order(Order& incoming) {
    if (incoming.side == Side::Buy) {
        // Match against asks starting from the lowest price (asks_.begin())
        auto it = asks_.begin();
        while (it != asks_.end() && incoming.quantity > 0 && it->first <= incoming.price) {
            Limit& limit = it->second;
            auto order_it = limit.orders.begin();

            while (order_it != limit.orders.end() && incoming.quantity > 0) {
                if (incoming.quantity >= order_it->quantity) {
                    // Full fill of resting order
                    incoming.quantity -= order_it->quantity;
                    limit.totalQuantity -= order_it->quantity;
                    order_registry_.erase(order_it->id);
                    order_it = limit.orders.erase(order_it); // O(1) list removal
                } else {
                    // Partial fill of resting order
                    order_it->quantity -= incoming.quantity;
                    limit.totalQuantity -= incoming.quantity;
                    incoming.quantity = 0;
                }
            }

            // Clean up empty price level
            if (limit.orders.empty()) {
                it = asks_.erase(it);
            } else {
                ++it;
            }
        }
    } else {
        // Match against bids starting from the highest price (bids_.begin())
        auto it = bids_.begin();
        while (it != bids_.end() && incoming.quantity > 0 && it->first >= incoming.price) {
            Limit& limit = it->second;
            auto order_it = limit.orders.begin();

            while (order_it != limit.orders.end() && incoming.quantity > 0) {
                if (incoming.quantity >= order_it->quantity) {
                    incoming.quantity -= order_it->quantity;
                    limit.totalQuantity -= order_it->quantity;
                    order_registry_.erase(order_it->id);
                    order_it = limit.orders.erase(order_it);
                } else {
                    order_it->quantity -= incoming.quantity;
                    limit.totalQuantity -= incoming.quantity;
                    incoming.quantity = 0;
                }
            }

            if (limit.orders.empty()) {
                it = bids_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void OrderBook::cancel_order(OrderId id) {
    auto lookup = order_registry_.find(id);
    if (lookup == order_registry_.end()) {
        return;
    }

    const auto& record = lookup->second;
    Price price = record.price;
    Side side = record.side;
    auto order_it = record.iterator;

    if (side == Side::Buy) {
        auto map_it = bids_.find(price);
        if (map_it != bids_.end()) {
            Limit& limit = map_it->second;
            limit.totalQuantity -= order_it->quantity;
            limit.orders.erase(order_it);
            if (limit.orders.empty()) {
                bids_.erase(map_it);
            }
        }
    } else {
        auto map_it = asks_.find(price);
        if (map_it != asks_.end()) {
            Limit& limit = map_it->second;
            limit.totalQuantity -= order_it->quantity;
            limit.orders.erase(order_it);
            if (limit.orders.empty()) {
                asks_.erase(map_it);
            }
        }
    }

    order_registry_.erase(lookup);
}

Quantity OrderBook::get_volume_at_price(Price price, Side side) const {
    if (side == Side::Buy) {
        auto it = bids_.find(price);
        return (it != bids_.end()) ? it->second.totalQuantity : 0;
    } else {
        auto it = asks_.find(price);
        return (it != asks_.end()) ? it->second.totalQuantity : 0;
    }
}

std::size_t OrderBook::size() const noexcept {
    return order_registry_.size();
}

} // namespace orderbook