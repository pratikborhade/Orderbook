#pragma once
#include "orderside.hpp"
#include <cstddef>
#include <vector>
namespace orderbook {
    /**
     * @brief Structure that maintain order in order book
     * Assumtions while using this structure, two orders are same if the side clientId and orderId are equal
     * Size is always updated and the latest size will be found in orderbook.
     * Invalid order is where price, size, clientId and orderId are set to -1
     */
    struct Orders
    {
        int size = 0; // size will change as the orders are matched

        struct OrderDetails
        {
            int clientId;
            int orderId;
            int quantity;
            bool operator==(const OrderDetails& other) const
            {
                return clientId == other.clientId && orderId == other.orderId;
            }
        };

        std::vector<OrderDetails> orderDetails;
        // constructor
        // creates a valid order
        Orders() = default;
        Orders(Orders&&) = default;
        Orders(const Orders&) = delete;
        bool operator==(const Orders& other) const;
        Orders& operator=(const Orders& other) = delete;

        bool add_order(int clientId, int orderId, int size);
        bool remove_order(int clientId, int orderId);
    };
}