#include "pch.h"

#include "MatchingEngine.h"

void MatchingEngine::InitialiseMarkets(const std::vector<std::string>& markets)
{
    for(const auto& market : markets)
    {
        m_markets.insert(std::make_pair(market, Market{}));
    }
}

void MatchingEngine::RegisterEventObserver(IMatchingEvent* pObserver)
{
    m_matchingEventObservers.push_back(pObserver);
}

OrderPlaceEventResult MatchingEngine::OnOrderPlace(Order&& o)
{
    if(o.market.empty() || o.price == 0.0 || o.volume == 0.0)
    {
        return OrderPlaceEventResult::OrderCancelled;
    }

    return HandleOrderBookUpdate(std::move(o));
}

OrderCancelEventResult MatchingEngine::OnOrderCancel(OrderID oid)
{
    return 
        HandleOrderBookCancel(oid) == true 
            ? OrderCancelEventResult::OrderCancelled : OrderCancelEventResult::OrderNotFound;
}

OrderPlaceEventResult MatchingEngine::HandleOrderBookUpdate(Order&& o)
{
    const Markets::iterator itMarket = m_markets.find(o.market);
    if(itMarket == m_markets.end())
    {
        // trying to place an order on a market that doesn't exist
        return OrderPlaceEventResult::OrderCancelled;
    }

    auto& [bids, asks] = itMarket->second;

    const auto UpdateOBBasedOnType = [this](OrderBookPosition& obp, const Order& o)
    {
        // Create position if it doesn't already exist    
        OrderBookOrdersAtPosition& oboap = obp[o.price];
        oboap.insert({m_nextOrderID, o.volume});
        m_orderLookup.insert(std::make_pair(m_nextOrderID, &oboap));
        ++m_nextOrderID;
    };

    if(o.type == OrderType::Bid)
    {
        UpdateOBBasedOnType(bids, o);
    }
    else if(o.type == OrderType::Ask)
    {
        UpdateOBBasedOnType(asks, o);
    }

    // Check for matched events
    const bool matchedOrder = TickOrderBook(itMarket->second);

    return matchedOrder 
        ? OrderPlaceEventResult::OrderMatched
        : OrderPlaceEventResult::OrderPlaced;
}

bool MatchingEngine::HandleOrderBookCancel(OrderID oid)
{
    OrderLookup::iterator it = m_orderLookup.find(oid);

    if(it == m_orderLookup.end())
    {
        return false;
    }

    OrderBookOrdersAtPosition& oboap = *it->second;
    
    const bool cancelledOrder = oboap.erase(oid) > 0;
    m_orderLookup.erase(it);
    return cancelledOrder;
}

bool MatchingEngine::TickOrderBook(Market& market)
{
    auto& [bids, asks] = market;

    if(bids.empty() || asks.empty())
    {
        // Impossible to match an order if one or
        // both sides are empty
        return false;
    }

    bool matchOccurred{false};

    OrderBookPosition::iterator itAskOrders = asks.begin();
    OrderBookPosition::reverse_iterator ritBidOrders = bids.rbegin();

    bool shouldContinue{true};
    
    while(shouldContinue)
    {
        if(ritBidOrders->first >= itAskOrders->first)
        {
            // Topmost bid position crosses the current ask
            // This indicates match(es) can occur

            OrderBookOrdersAtPosition& bidPositionOrders = ritBidOrders->second;
            OrderBookOrdersAtPosition& askPositionOrders = itAskOrders->second;

            OrderBookOrdersAtPosition::iterator itBidPositionOrder = bidPositionOrders.begin();
            OrderBookOrdersAtPosition::iterator itAskPositionOrder = askPositionOrders.begin();
            
            while(    itBidPositionOrder != bidPositionOrders.end()
                   && itAskPositionOrder != askPositionOrders.end())
            {
                matchOccurred = true;
                    
                if(itBidPositionOrder->second == itAskPositionOrder->second)
                {
                    // Equal sizes of orders, remove both
                    // TODO match notification
                    bidPositionOrders.erase(itBidPositionOrder++);
                    askPositionOrders.erase(itAskPositionOrder++);
                }
                else if(itBidPositionOrder->second < itAskPositionOrder->second)
                {
                    // This BID order satisfies part of the ASK order
                    // TODO match notification
                    itAskPositionOrder->second -= itBidPositionOrder->second;
                    bidPositionOrders.erase(itBidPositionOrder++);
                }
                else 
                {
                    // Equivalent to if(itBidPositionOrder->second > itAskPositionOrder->second)

                    // This ASK order satisfies part of the BID order
                    // TODO match notification
                    itBidPositionOrder->second -= itAskPositionOrder->second;
                    askPositionOrders.erase(itAskPositionOrder++);
                }

                bool positionRemoved{false};

                if(bidPositionOrders.empty())
                {
                    // Position on BIDs side has been removed

                    // Aparently erasing a reverse iterator isn't directly supported -_-
                    bids.erase(std::prev(std::end(bids)));
                    ritBidOrders = bids.rbegin();
                    positionRemoved = true;
                }

                if(askPositionOrders.empty())
                {
                    // Position on ASKs side has been removed
                    itAskOrders = asks.erase(itAskOrders);
                    positionRemoved = true;
                }

                if(positionRemoved)
                {
                    break;
                }
            }
        }
        else
        {
            shouldContinue = false;
        }

        if(itAskOrders == asks.end() || ritBidOrders == bids.rend())
        {
            shouldContinue = false;
        }
    }

    return matchOccurred;
}