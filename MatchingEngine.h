#pragma once

#include <vector>

#include "EngineInterfaces.h"

class MatchingEngine : public IEngineEvents
{
public:
    MatchingEngine() = default;
    virtual ~MatchingEngine() = default;

    MatchingEngine(const MatchingEngine&) = delete;
    MatchingEngine(MatchingEngine&&) = delete;
    MatchingEngine& operator =(const MatchingEngine&) = delete;

    void InitialiseMarkets(const std::vector<std::string>& markets);

    void RegisterEventObserver(IExchangeEvents* pObserver);

    //
    // IEngineEvents implementation
    //

    virtual OrderPlaceEventResult OnOrderPlace(Order&& o) override final;
    virtual OrderCancelEventResult OnOrderCancel(OrderID oid) override final;

private:

    using OrderBookOrdersAtPosition = std::map<OrderID, NumericType>;
    using OrderBookPosition = std::map<NumericType, OrderBookOrdersAtPosition>;

    // Each market consists of positions of bids and asks stored individually 
    using Market = std::pair<OrderBookPosition, OrderBookPosition>;
    using Markets = std::unordered_map<std::string, Market>;

    using OrderLookup = std::map<OrderID, OrderBookOrdersAtPosition*>;

    OrderPlaceEventResult HandleOrderBookUpdate(Order&& o);

    bool HandleOrderBookCancel(OrderID o);
    bool TickOrderBook(const std::string& marketName, Market& market);

    void NotifyOrderBookEventObservers(OrderID oid, const Order& mo);
    void NotifyMatchingEventObservers(const MatchedOrder& mo);

    OrderID m_nextOrderID{ 0 };

    std::vector<IExchangeEvents*> m_eventObservers;
    
    // Non-owning collection for fast order ID lookups
    OrderLookup m_orderLookup;

    Markets m_markets;
};