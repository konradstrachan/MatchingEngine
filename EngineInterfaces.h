#pragma once

#include "Types.h"

class IEngineEvent
{
public:
    virtual OrderPlaceEventResult OnOrderPlace(Order&& o) = 0;
    virtual OrderCancelEventResult OnOrderCancel(OrderID oid) = 0;
};

class IMatchingEvent
{
public:
    virtual void OnOrderMatched(Order&& o) = 0;
};