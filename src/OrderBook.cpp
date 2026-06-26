#include "orderbook/OrderBook.hpp"

/**
 * @file OrderBook.cpp
 * @brief Implementation of the OrderBook class methods.
 */

namespace orderbook {

OrderBook::OrderBook(size_t order_pool_capacity) : pool_(order_pool_capacity) {
    order_registry_.reserve(order_pool_capacity);
}

/**
 * @brief Processes and places a new order.
 * 
 * Flow:
 * 1. Checks for duplicate order ID; if found, the order is ignored.
 * 2. Attempts to match the incoming order against resting orders on the opposite side.
 * 3. If the incoming order is not fully filled (remaining quantity > 0):
 *    - Obtains or creates the Limit level for the order's price.
 *    - Updates the Limit level's total volume.
 *    - Appends the order to the Limit level's FIFO queue.
 *    - Inserts a record of the order into the registry for O(1) lookups.
 */
void OrderBook::add_order(OrderId id, Price price, Quantity qty, Side side) {
    if (order_registry_.find(id) != order_registry_.end()) {
        return; 
    }

    Order incoming{id, price, qty, side};
    match_order(incoming);

    if (incoming.quantity > 0) {
        Order *resting = pool_.acquire(id, price, incoming.quantity, side);
        if (side == Side::Buy) {
            auto [map_it, inserted] = bids_.try_emplace(price, price);
            Limit& limit = map_it->second;
            
            limit.totalQuantity += resting->quantity;
            limit.push_back(resting);
            
            order_registry_.try_emplace(id, OrderRecord{resting, side, price});
        } else {
            auto [map_it, inserted] = asks_.try_emplace(price, price);
            Limit& limit = map_it->second;
            
            limit.totalQuantity += resting->quantity;
            limit.push_back(resting);
            
            order_registry_.try_emplace(id, OrderRecord{resting, side, price});
        }
    }
}

/**
 * @brief Matches an incoming order against existing resting orders on the opposite side of the book.
 * 
 * Iterates through the opposite side's price levels starting from the best available price:
 * - Bids match against asks (lowest price first).
 * - Asks match against bids (highest price first).
 * 
 * Within each price level, orders are matched in FIFO (price-time priority) order.
 * If a resting order is fully filled, it is removed from the book and the registry.
 * If a resting order is partially filled, its quantity is reduced.
 * If a price level becomes empty, the level is erased from the book.
 * Matching stops when the incoming order is fully filled or price crossing limits are exceeded.
 */
void OrderBook::match_order(Order& incoming) {
    if (incoming.side == Side::Buy) {
        auto it = asks_.begin();
        while (it != asks_.end() && incoming.quantity > 0 && it->first <= incoming.price) {
            Limit& limit = it->second;
            Order *order_it = limit.head;

            while (order_it != nullptr && incoming.quantity > 0) {
                Order* next_order = order_it->next;
                if (incoming.quantity >= order_it->quantity) {
                    incoming.quantity -= order_it->quantity;
                    limit.totalQuantity -= order_it->quantity;
                    order_registry_.erase(order_it->id);
                    limit.erase(order_it);
                    pool_.release(order_it);
                    order_it = next_order;
                } else {
                    order_it->quantity -= incoming.quantity;
                    limit.totalQuantity -= incoming.quantity;
                    incoming.quantity = 0;
                    break;
                }
            }

            if (limit.empty()) {
                it = asks_.erase(it);
            } else {
                ++it;
            }
        }
    } else {
        auto it = bids_.begin();
        while (it != bids_.end() && incoming.quantity > 0 && it->first >= incoming.price) {
            Limit& limit = it->second;
            Order *order_it = limit.head;

            while (order_it != nullptr && incoming.quantity > 0) {
                Order* next_order = order_it->next;
                if (incoming.quantity >= order_it->quantity) {
                    incoming.quantity -= order_it->quantity;
                    limit.totalQuantity -= order_it->quantity;
                    order_registry_.erase(order_it->id);
                    limit.erase(order_it);
                    pool_.release(order_it);
                    order_it = next_order;
                } else {
                    order_it->quantity -= incoming.quantity;
                    limit.totalQuantity -= incoming.quantity;
                    incoming.quantity = 0;
                    break;
                }
            }

            if (limit.empty()) {
                it = bids_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

/**
 * @brief Cancels an existing order by its unique ID.
 * 
 * Flow:
 * 1. Checks if the order exists in the registry.
 * 2. Deducts the order's remaining quantity from the Limit level's total volume.
 * 3. Erases the order from the Limit level's list in O(1) time using the cached iterator.
 * 4. If the Limit level is now empty, removes the price level from the map.
 * 5. Erases the order record from the registry.
 */
void OrderBook::cancel_order(OrderId id) {
    auto lookup = order_registry_.find(id);
    if (lookup == order_registry_.end()) {
        return;
    }

    const auto& record = lookup->second;
    Price price = record.price;
    Side side = record.side;
    Order* target_order = record.order_ptr;

    if (side == Side::Buy) {
        auto map_it = bids_.find(price);
        if (map_it != bids_.end()) {
            Limit& limit = map_it->second;
            limit.totalQuantity -= target_order->quantity;
            limit.erase(target_order);
            if (limit.empty()) {
                bids_.erase(map_it);
            }
        }
    } else {
        auto map_it = asks_.find(price);
        if (map_it != asks_.end()) {
            Limit& limit = map_it->second;
            limit.totalQuantity -= target_order->quantity;
            limit.erase(target_order);
            if (limit.empty()) {
                asks_.erase(map_it);
            }
        }
    }
    pool_.release(target_order);
    order_registry_.erase(lookup);
}

/**
 * @brief Retrieves the aggregate resting volume (quantity) at the specified price level and side.
 */
Quantity OrderBook::get_volume_at_price(Price price, Side side) const {
    if (side == Side::Buy) {
        auto it = bids_.find(price);
        return (it != bids_.end()) ? it->second.totalQuantity : 0;
    } else {
        auto it = asks_.find(price);
        return (it != asks_.end()) ? it->second.totalQuantity : 0;
    }
}

/**
 * @brief Returns the total number of active orders resting in the book.
 */
std::size_t OrderBook::size() const noexcept {
    return order_registry_.size();
}

} // namespace orderbook