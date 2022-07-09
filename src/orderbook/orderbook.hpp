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
        const Orderside side;
        std::size_t timestamp = 0; // timestamp will be maintained by the orderbook
        const int price;
        int size; // size will change as the orders are matched

        const int clientId;
        const int orderId;
        // constructor
        Order();
        Order(Orderside side, int clientId, int orderId, int price, int size);

        bool operator<(const Order& other) const;
        bool operator>(const Order& other) const;
        bool operator==(const Order& other) const; 
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
        std::map<std::pair<uint, uint>, Order> placedOrders;
        // to support multiple threads
        mutable std::shared_mutex mtx;

        void match(Order& order);

        // for naive timestamp we will just use a counter
        std::atomic<std::size_t> gTimestamp = 1;
    public:
        // To add order to orderbook
        bool add_order(Order& order);
        // to remove order from orderbook
        bool cancel_order(uint clientId, uint orderId);
        // to clear orderbook
        void flush();

        // Get max ask order
        Order getMinAsk() const;

        // Get min bid order
        Order getMaxBid() const;
    };

}