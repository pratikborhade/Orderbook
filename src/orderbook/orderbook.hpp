#include <vector>
#include <map>
#include <set>
#include <chrono>
#include <functional>
#include <shared_mutex>

namespace orderbook {
    enum class Orderside : short {
        ask,
        bid
    };

    /**
     * @brief Structure that maintain order in order book
     * Assumtions while using this structure, two orders are same if the side clientId and orderId are equal
     * Size is always updated and the latest size will be found in orderbook.
     */
    struct Order
    {
        const Orderside side;
        const uint64_t timestamp;
        const float price;
        float size; // size will change as the orders are matched

        const uint clientId;
        const uint orderId;
        // constructor
        Order(Orderside side, uint clientId, uint orderId, uint64_t timestamp, float price, float size);

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
        std::shared_mutex mtx;

        void match(Order& order);
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