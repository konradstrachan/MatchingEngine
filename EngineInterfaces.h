#pragma once

#include "Types.h"

class IEngineEvents
{
public:
    virtual OrderPlaceEventResult OnOrderPlace(Order&& o) = 0;
    virtual OrderCancelEventResult OnOrderCancel(OrderID oid) = 0;
};

class IExchangeEvents
{
public:
    virtual void OnNewOrder(OrderID oid, const Order& o) = 0;
    virtual void OnCancelledOrder(OrderID oid) = 0;
    virtual void OnOrderMatched(const MatchedOrder& mo) = 0;
};
