#pragma once

#include <vector>
#include <map>
#include <functional>
#include <shared_mutex>

#include "orders.hpp"
#include "orderside.hpp"
#include <memory>

namespace orderbook {
    /**
     * @brief Orderbook to track bid and ask orders
     * Orders must follow assumptions that there are no two orders with same side, clientId and orderId
     */
    class Orderbook
    {
        // to store asks
        std::map<int, std::unique_ptr<Orders>> asks;
        // to store bids
        std::map<int, std::unique_ptr<Orders>, std::greater<int>> bids;
        // to map order_key i.e (clientId, orderId) -> (price, side) / used for cancel
        std::map<std::pair<int, int>, std::pair<int, Orderside>> placedOrders;
        // to support multiple threads
        mutable std::shared_mutex mtx;

    public:
        // functor which is called in case of match
        // calls with orderside, clientIdInBook, clientOrderIdInBook, clientId, OrderderId, price, quantity
        using MatchFunctor = std::function<bool(Orderside orderside, int, int, int, int, int, int)>;

        // To add order to orderbook
        bool add_order(Orderside side, int clientId, int orderId, int price, int quantity, MatchFunctor matchFunctor);
        // to remove order from orderbook
        bool cancel_order(int clientId, int orderId);
        // to clear orderbook
        void flush();
        // Get max (price, quantity) in ask orders
        std::pair<int, int> get_min_ask() const;
        // Get min (price, quantity) in bid orders
        std::pair<int, int> get_max_bid() const;

    private:
        // call to match orders / should aquire write lock to mutex
        // returns if the order is fullfilled or not during match
        bool match(Orderside side, int clientId, int orderId, int price, int& quantity, MatchFunctor& matchFunctor);
    };

}