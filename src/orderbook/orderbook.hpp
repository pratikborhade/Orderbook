#include <vector>
#include <map>
#include <set>
#include <chrono>
#include <functional>
#include <shared_mutex>
#include <atomic>

#include "order.hpp"

namespace orderbook {
    /**
     * @brief Orderbook to track bid and ask orders
     * Orders must follow assumptions of the structure Order, two orders with same side, clientId and orderId with different prices should not be added
     */
    class Orderbook
    {
        struct AsksComparator
        {
            bool operator()(const Order& a, const Order& b) const;
        };

        struct BidsComparator
        {
            bool operator()(const Order& a, const Order& b) const;
        };
        // to store asks
        std::set<Order, AsksComparator> asks;
        // to store bids
        std::set<Order, BidsComparator> bids;
        // to map order_key i.e (clientId, orderId) -> Order
        std::map<std::pair<int, int>, Order> placedOrders;
        // to support multiple threads
        mutable std::shared_mutex mtx;

        // for naive timestamp we will just use a counter
        std::atomic<std::size_t> gTimestamp = 1;

    public:
        // functor which is called in case of match
        // calls with order in orderbook, added order, price and size
        using MatchFunctor = std::function<bool(const Order&, const Order&, int, int)>;

        // To add order to orderbook
        bool add_order(Order& order, MatchFunctor &matchFunctor);
        // to remove order from orderbook
        bool cancel_order(int clientId, int orderId);
        // to clear orderbook
        void flush();

        // Get max ask order
        Order getMinAsk() const;

        // Get min bid order
        Order getMaxBid() const;

    private:
        // call to match orders / should aquire write lock to mutex
        // returns if the order is fullfilled or not during match
        bool match(Order& order, MatchFunctor& matchFunctor);
    };

}