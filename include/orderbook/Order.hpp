#pragma once

#include "Types.hpp"
namespace orderbook {

    struct Order {
        OrderId id;
        Price price;
        Quantity quantity;
        Side side;

        Order(OrderId oid, Price p, Quantity q, Side s)
        : id(oid), price(p), quantity(q), side(s) {}
    };
}