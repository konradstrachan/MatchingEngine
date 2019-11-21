
#include "pch.h"

#include <iostream>
#include <cassert>

#include "MatchingEngine.h"
#include "TestClient.h"

#define START_TEST( test_name )                                                 \
    bool result{ true };                                                        \
    std::string name{test_name};

#define EXPECTED( var, val )                                                    \
    if( result && var != val )                                                  \
    {                                                                           \
        std::cerr << "Test " << name.c_str()                                    \
                  << " at line [" << __LINE__ << "] failed"                     \
                  << std::endl;                                                 \
        assert(false);                                                          \
        result = false;                                                         \
    }

int main()
{
    const std::string market{"BTC-USD"};

    const auto PlaceOrdersFn = [](MatchingEngine& me, const std::vector<Order>& orders) -> std::vector<OrderPlaceEventResult>
    {
        std::vector<OrderPlaceEventResult> results;

        for(const auto& order : orders)
        {
            // Copy so we can move in to the interface
            Order o{order};
            results.push_back(
                me.OnOrderPlace(
                    std::move(o)));
        }

        return results;
    };

    {
        START_TEST( "Populate simple OB" )
        MatchingEngine me;
        me.InitialiseMarkets({"BTC-USD"});

        TestClient tc;
        me.RegisterEventObserver(&tc);

        std::vector<Order> orders{
            {market, 10, 2, OrderType::Bid},
            {market, 11, 2, OrderType::Bid},
            {market, 20, 2, OrderType::Ask},
            {market, 21, 2, OrderType::Ask}
        };

        auto results = PlaceOrdersFn(me, orders);

        // Check no matches occurred
        EXPECTED(tc.m_matchingEvents.empty(), true);

        // Check we received the response codes we expect
        EXPECTED(results.size(), 4);
        EXPECTED(results[0], OrderPlaceEventResult::OrderPlaced);
        EXPECTED(results[1], OrderPlaceEventResult::OrderPlaced);
        EXPECTED(results[2], OrderPlaceEventResult::OrderPlaced);
        EXPECTED(results[3], OrderPlaceEventResult::OrderPlaced);

        // Check the correct events were emitted
        EXPECTED(tc.m_orderBookUpdateEvents.size(), 4);
        EXPECTED(tc.m_orderBookUpdateEvents[0].first, 0);
        EXPECTED(tc.m_orderBookUpdateEvents[0].second, orders[0]);
        EXPECTED(tc.m_orderBookUpdateEvents[1].first, 1);
        EXPECTED(tc.m_orderBookUpdateEvents[1].second, orders[1]);
        EXPECTED(tc.m_orderBookUpdateEvents[2].first, 2);
        EXPECTED(tc.m_orderBookUpdateEvents[2].second, orders[2]);
        EXPECTED(tc.m_orderBookUpdateEvents[3].first, 3);
        EXPECTED(tc.m_orderBookUpdateEvents[3].second, orders[3]);

        // Check nothing was cancelled
        EXPECTED(tc.m_cancelEvents.empty(), true);
    }

    {
        START_TEST( "Cancel position" )
        MatchingEngine me;
        me.InitialiseMarkets({"BTC-USD"});

        TestClient tc;
        me.RegisterEventObserver(&tc);

        std::vector<Order> orders{
            {market, 10, 2, OrderType::Bid},
            {market, 11, 2, OrderType::Bid},
            {market, 20, 2, OrderType::Ask},
            {market, 21, 2, OrderType::Ask}
        };

        auto results = PlaceOrdersFn(me, orders);

        // Check no matches occurred
        EXPECTED(tc.m_matchingEvents.empty(), true);

        // Check we received the response codes we expect
        EXPECTED(results.size(), 4);
        EXPECTED(results[0], OrderPlaceEventResult::OrderPlaced);
        EXPECTED(results[1], OrderPlaceEventResult::OrderPlaced);
        EXPECTED(results[2], OrderPlaceEventResult::OrderPlaced);
        EXPECTED(results[3], OrderPlaceEventResult::OrderPlaced);

        // Check the correct events were emitted
        EXPECTED(tc.m_orderBookUpdateEvents.size(), 4);
        EXPECTED(tc.m_orderBookUpdateEvents[0].first, 0);
        EXPECTED(tc.m_orderBookUpdateEvents[0].second, orders[0]);
        EXPECTED(tc.m_orderBookUpdateEvents[1].first, 1);
        EXPECTED(tc.m_orderBookUpdateEvents[1].second, orders[1]);
        EXPECTED(tc.m_orderBookUpdateEvents[2].first, 2);
        EXPECTED(tc.m_orderBookUpdateEvents[2].second, orders[2]);
        EXPECTED(tc.m_orderBookUpdateEvents[3].first, 3);
        EXPECTED(tc.m_orderBookUpdateEvents[3].second, orders[3]);

        // Cancel two positions
        OrderCancelEventResult result1 = me.OnOrderCancel(1);
        OrderCancelEventResult result2 = me.OnOrderCancel(3);

        EXPECTED(result1, OrderCancelEventResult::OrderCancelled);
        EXPECTED(result2, OrderCancelEventResult::OrderCancelled);

        // Check the correct events were emitted
        EXPECTED(tc.m_cancelEvents.size(), 2);
        EXPECTED(tc.m_cancelEvents[0], 1);
        EXPECTED(tc.m_cancelEvents[1], 3);

        // Attempt to cancel an order that doesn't exist
        OrderCancelEventResult result3 = me.OnOrderCancel(1000);

        EXPECTED(result3, OrderCancelEventResult::OrderNotFound);

        // Check the no new events were emitted
        EXPECTED(tc.m_cancelEvents.size(), 2);
    }

    {
        START_TEST( "Test rejecting invalid orders" )
        MatchingEngine me;
        me.InitialiseMarkets({"BTC-USD"});

        TestClient tc;
        me.RegisterEventObserver(&tc);

        std::vector<Order> orders{
            {market, 0, 2, OrderType::Bid},     // No price
            {market, 11, 0, OrderType::Bid},    // no volume
            {"BTC-NOTVALID", 11, 0, OrderType::Bid}     // another market
        };

        auto results = PlaceOrdersFn(me, orders);

        // Check no matches occurred
        EXPECTED(tc.m_matchingEvents.empty(), true);

        // Check we received the response codes we expect
        EXPECTED(results.size(), 3);
        EXPECTED(results[0], OrderPlaceEventResult::OrderCancelled);
        EXPECTED(results[1], OrderPlaceEventResult::OrderCancelled);
        EXPECTED(results[2], OrderPlaceEventResult::OrderCancelled);

        // Check no events were emitted
        EXPECTED(tc.m_orderBookUpdateEvents.size(), 0);

        // Check nothing was cancelled
        EXPECTED(tc.m_cancelEvents.empty(), true);
    }

    {
        START_TEST( "Simple matching tests initiated by BID" )
        MatchingEngine me;
        me.InitialiseMarkets({"BTC-USD"});

        TestClient tc;
        me.RegisterEventObserver(&tc);

        std::vector<Order> orders1{
            {market, 10, 2, OrderType::Bid},
            {market, 11, 2, OrderType::Bid},
            {market, 20, 1, OrderType::Ask},
            {market, 20, 1, OrderType::Ask},
            {market, 21, 2, OrderType::Ask}
        };

        auto results1 = PlaceOrdersFn(me, orders1);

        // Check no matches occurred
        EXPECTED(tc.m_matchingEvents.empty(), true);

        // Check we received the response codes we expect
        EXPECTED(results1.size(), 5);
        EXPECTED(results1[0], OrderPlaceEventResult::OrderPlaced);
        EXPECTED(results1[1], OrderPlaceEventResult::OrderPlaced);
        EXPECTED(results1[2], OrderPlaceEventResult::OrderPlaced);
        EXPECTED(results1[3], OrderPlaceEventResult::OrderPlaced);
        EXPECTED(results1[4], OrderPlaceEventResult::OrderPlaced);

        // Check the correct events were emitted
        EXPECTED(tc.m_orderBookUpdateEvents.size(), 5);
        EXPECTED(tc.m_orderBookUpdateEvents[0].first, 0);
        EXPECTED(tc.m_orderBookUpdateEvents[0].second, orders1[0]);
        EXPECTED(tc.m_orderBookUpdateEvents[1].first, 1);
        EXPECTED(tc.m_orderBookUpdateEvents[1].second, orders1[1]);
        EXPECTED(tc.m_orderBookUpdateEvents[2].first, 2);
        EXPECTED(tc.m_orderBookUpdateEvents[2].second, orders1[2]);
        EXPECTED(tc.m_orderBookUpdateEvents[3].first, 3);
        EXPECTED(tc.m_orderBookUpdateEvents[3].second, orders1[3]);
        EXPECTED(tc.m_orderBookUpdateEvents[4].first, 4);
        EXPECTED(tc.m_orderBookUpdateEvents[4].second, orders1[4]);

        // After initially populating a simple orderbook, now
        // place an order that crosses an existing ASK positions

        // This order should match the orders at price 20, then partially at 21
        std::vector<Order> orders2{
            {market, 21, 3, OrderType::Bid},
        };

        auto results2 = PlaceOrdersFn(me, orders2);

        // Check we received the response codes we expect
        EXPECTED(results2.size(), 1);
        EXPECTED(results2[0], OrderPlaceEventResult::OrderMatched);

        EXPECTED(tc.m_matchingEvents.size(), 3);

        // For fairness we expect the earliest placed order at that position to be satisfied first
        MatchedOrder expectedMatch1
        {
            market,
            5,          // bid side orderID will be the one we've just placed (5th)
            2,          // ask side will be the first matching ask side we placed (3rd)
            20,         // execution price
            1,          // size of position as entire position consumed
            OrderType::Ask  // Sell
        };

        // Followed by the next at that position
        MatchedOrder expectedMatch2
        {
            market,
            5,          // bid side orderID will be the one we've just placed (5th)
            3,          // ask side will be the first matching ask side we placed (3rd)
            20,         // execution price
            1,          // size of position as entire position consumed
            OrderType::Ask  // Sell
        };

        // Since that consumes all the orders at the earlier price point, we now move to
        // the next price position that still matches the BID placed
        MatchedOrder expectedMatch3
        {
            market,
            5,          // bid side orderID will be the one we've just placed (5th)
            4,          // ask side will be the first matching ask side we placed (3rd)
            21,         // execution price
            1,          // only a portion of the position has been consumed
            OrderType::Ask  // Sell
        };
        
        EXPECTED(tc.m_matchingEvents[0], expectedMatch1);
        EXPECTED(tc.m_matchingEvents[1], expectedMatch2);
        EXPECTED(tc.m_matchingEvents[2], expectedMatch3);

        // Check nothing was cancelled
        EXPECTED(tc.m_cancelEvents.empty(), true);
    }

    {
        START_TEST( "Simple matching tests initiated by ASK" )
        MatchingEngine me;
        me.InitialiseMarkets({"BTC-USD"});

        TestClient tc;
        me.RegisterEventObserver(&tc);

        std::vector<Order> orders1{
            {market, 10, 2, OrderType::Bid},
            {market, 11, 1, OrderType::Bid},
            {market, 20, 1, OrderType::Ask},
            {market, 21, 1, OrderType::Ask}
        };

        auto results1 = PlaceOrdersFn(me, orders1);

        // Check no matches occurred
        EXPECTED(tc.m_matchingEvents.empty(), true);

        // Check we received the response codes we expect
        EXPECTED(results1.size(), 4);
        EXPECTED(results1[0], OrderPlaceEventResult::OrderPlaced);
        EXPECTED(results1[1], OrderPlaceEventResult::OrderPlaced);
        EXPECTED(results1[2], OrderPlaceEventResult::OrderPlaced);
        EXPECTED(results1[3], OrderPlaceEventResult::OrderPlaced);

        // Check the correct events were emitted
        EXPECTED(tc.m_orderBookUpdateEvents.size(), 4);
        EXPECTED(tc.m_orderBookUpdateEvents[0].first, 0);
        EXPECTED(tc.m_orderBookUpdateEvents[0].second, orders1[0]);
        EXPECTED(tc.m_orderBookUpdateEvents[1].first, 1);
        EXPECTED(tc.m_orderBookUpdateEvents[1].second, orders1[1]);
        EXPECTED(tc.m_orderBookUpdateEvents[2].first, 2);
        EXPECTED(tc.m_orderBookUpdateEvents[2].second, orders1[2]);
        EXPECTED(tc.m_orderBookUpdateEvents[3].first, 3);
        EXPECTED(tc.m_orderBookUpdateEvents[3].second, orders1[3]);

        // After initially populating a simple orderbook, now
        // place an order that crosses an existing BID positions

        // This order should match the orders at price 11, then at 10
        std::vector<Order> orders2{
            {market, 10, 2, OrderType::Ask},
        };

        auto results2 = PlaceOrdersFn(me, orders2);

        // Check we received the response codes we expect
        EXPECTED(results2.size(), 1);
        EXPECTED(results2[0], OrderPlaceEventResult::OrderMatched);

        EXPECTED(tc.m_matchingEvents.size(), 2);

        // We expect to walk backwards through the bids until we reach the price the ask was placed at
        MatchedOrder expectedMatch1
        {
            market,
            1,          // first matched bid side orderID will be the second added
            4,          // ask side will be the first matching ask side we placed (3rd)
            11,         // execution price
            1,          // size of position as entire position consumed
            OrderType::Bid  // Buy
        };

        // Followed by the next at the previous position
        MatchedOrder expectedMatch2
        {
            market,
            0,          // followed by the first orderID which was at an earlier position
            4,          // ask side will be the first matching ask side we placed (3rd)
            10,         // execution price
            1,          // size of position as entire position consumed
            OrderType::Bid  // Buy
        };

        EXPECTED(tc.m_matchingEvents[0], expectedMatch1);
        EXPECTED(tc.m_matchingEvents[1], expectedMatch2);

        // Check nothing was cancelled
        EXPECTED(tc.m_cancelEvents.empty(), true);
    }

    return 0;
}
