#pragma once

#include <map>
#include <unordered_map>

enum class OrderPlaceEventResult
{
    OrderPlaced,
    OrderCancelled,
    OrderMatched
};

enum class OrderCancelEventResult
{
    OrderCancelled,
    OrderNotFound
};

using NumericType = uint32_t;

using OrderID = uint64_t;

enum class OrderType
{
    Bid,
    Ask
};

struct Order
{
    std::string market;
    NumericType price{0};
    NumericType volume{0};
    OrderType type{OrderType::Bid};

    bool operator !=(const Order& rhs) const
    {
        return std::tie(market, price, volume, type) 
            != std::tie(rhs.market, rhs.price, rhs.volume, rhs.type);
    }
};

struct MatchedOrder
{
    std::string market;
    OrderID bidSideOrderID;
    OrderID askSideOrderID;
    NumericType price{0};
    NumericType volume{0};
    OrderType type{OrderType::Bid};
};