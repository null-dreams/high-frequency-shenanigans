#pragma once

#include "Types.hpp"
#include "Order.hpp"
#include <list>

namespace orderbook {
    struct Limit {
        Price price;
        Quantity totalQuantity{0};
        std::list<Order> orders;

        explicit Limit(Price price) : price(price), totalQuantity(0) {}
    };
}