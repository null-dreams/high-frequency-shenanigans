#pragma once

#include <cstdint>

namespace market::protocol {

enum class ActionType : uint8_t {
    Add = 1,
    Cancel = 2
};

enum class Side : uint8_t {
    Buy = 0,
    Sell = 1
};

#pragma pack(push, 1)
struct MarketEvent {
    uint8_t type;       // 1 = Add, 2 = Cancel (ActionType)
    uint64_t order_id;
    uint64_t price;
    uint32_t qty;
    uint8_t side;       // 0 = Buy, 1 = Sell (Side)
};
#pragma pack(pop)

} // namespace market::protocol