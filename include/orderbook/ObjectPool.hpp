#pragma once

#include "Order.hpp"
#include <vector>
#include <stdexcept>

namespace orderbook {

class OrderPool {
public:
    explicit OrderPool(size_t capacity) {
        pool_.reserve(capacity);
        for (size_t i = 0; i < capacity; ++i) {
            pool_.emplace_back(0, 0, 0, Side::Buy);
        }
        
        // Link objects into a free-list
        for (size_t i = 0; i < capacity - 1; ++i) {
            pool_[i].next = &pool_[i + 1];
        }
        free_head_ = &pool_[0];
    }

    Order* acquire(OrderId id, Price price, Quantity qty, Side side) {
        if (!free_head_) {
            throw std::runtime_error("Order pool exhausted! Increase capacity.");
        }
        Order* order = free_head_;
        free_head_ = free_head_->next;

        order->id = id;
        order->price = price;
        order->quantity = qty;
        order->side = side;
        order->next = nullptr;
        order->prev = nullptr;

        return order;
    }

    void release(Order* order) noexcept {
        order->next = free_head_;
        order->prev = nullptr;
        free_head_ = order;
    }

private:
    std::vector<Order> pool_;
    Order* free_head_{nullptr};
};

} // namespace orderbook