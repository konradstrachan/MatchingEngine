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

// Floating point is lossy due to rounding issues,
// however this will suffice for a simple demo
using NumericType = double;

using OrderID = uint64_t;

enum class OrderType
{
    Bid,
    Ask
};

struct Order
{
    std::string market;
    NumericType price{0.0};
    NumericType volume{0.0};
    OrderType type{OrderType::Bid};
};
