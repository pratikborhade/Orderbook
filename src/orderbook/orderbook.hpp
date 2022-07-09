#include <vector>
#include <map>
#include <set>
#include <chrono>
#include <functional>
#include <shared_mutex>
#include <atomic>

namespace orderbook {
    enum class Orderside : short {
        buy,
        sell
    };

    /**
     * @brief Structure that maintain order in order book
     * Assumtions while using this structure, two orders are same if the side clientId and orderId are equal
     * Size is always updated and the latest size will be found in orderbook.
     * Invalid order is where price, size, clientId and orderId are set to -1
     */
    struct Order
    {
        Orderside side;
        std::size_t timestamp = 0; // timestamp will be maintained by the orderbook
        int price;
        int size; // size will change as the orders are matched

        int clientId;
        int orderId;
        // constructor
        Order();
        Order(Orderside side, int clientId, int orderId, int price, int size);

        bool isInvalid() const;

        bool operator<(const Order& other) const;
        bool operator>(const Order& other) const;
        bool operator==(const Order& other) const;
        Order& operator=(const Order& other) = default;
    };

    /**
     * @brief Orderbook to track bid and ask orders
     * Orders must follow assumptions of the structure Order, two orders with same side, clientId and orderId with different prices should not be added
     */
    class Orderbook
    {
        // to store asks
        std::set<Order> asks;
        // to store bids
        std::set<Order, std::greater<Order>> bids;
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