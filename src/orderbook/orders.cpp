#include "orders.hpp"
#include <algorithm>
using namespace orderbook;

bool Orders::operator==(const Orders& other) const
{
    return size == other.size && orderDetails == orderDetails;
}

bool Orders::add_order(int clientId, int orderId, int quantity)
{
    orderDetails.push_back(OrderDetails {clientId, orderId, quantity});
    size += quantity;
    return true;
}

bool Orders::remove_order(int clientId, int orderId)
{
    auto ite = std::find_if(orderDetails.begin(), orderDetails.end(), [clientId, orderId](const auto& detail) { return detail.clientId == clientId && detail.orderId == orderId; });
    if(ite == orderDetails.end())
        return false;
    size -= ite->quantity;
    orderDetails.erase(ite);
    return true;
}