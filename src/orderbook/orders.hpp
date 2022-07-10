#pragma once
#include "orderside.hpp"
#include <cstddef>
#include <vector>
namespace orderbook {
    /**
     * @brief Structure that maintain order details in order book
     * size variable will be updated as we add and remove orders
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