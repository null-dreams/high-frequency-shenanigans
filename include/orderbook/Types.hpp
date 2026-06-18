#pragma once

#include <cstdint>

namespace orderbook {

enum class Side : uint8_t  { Buy, Sell };
using OrderId = uint64_t;
using Price = uint64_t;
using Quantity = uint64_t;

} // namespace orderbook