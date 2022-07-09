#include "orderside.hpp"
#include <cstddef>
namespace orderbook {
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
        // creates an invalid order
        Order();
        // creates a valid order
        Order(Orderside side, int clientId, int orderId, int price, int size);

        bool isInvalid() const;
        bool operator==(const Order& other) const;

        Order& operator=(const Order& other) = default;
    };
}