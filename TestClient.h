#pragma once

#include "EngineInterfaces.h"

class TestClient : public IExchangeEvents
{
public:
    TestClient() = default;
    virtual ~TestClient() = default;

    //
    //  IExchangeEvents implementation
    //

    virtual void OnNewOrder(OrderID oid, const Order& o) override final
    {
        m_orderBookUpdateEvents.push_back(std::make_pair(oid, o));
    }

    virtual void OnCancelledOrder(OrderID oid) override final
    {
        m_cancelEvents.push_back(oid);
    }

    virtual void OnOrderMatched(const MatchedOrder& mo) override final
    {
        m_matchingEvents.push_back(mo);
    }

    std::vector<std::pair<OrderID, Order>> m_orderBookUpdateEvents;
    std::vector<MatchedOrder> m_matchingEvents;
    std::vector<OrderID> m_cancelEvents;
};