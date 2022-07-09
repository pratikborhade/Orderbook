#include "orderbook.hpp"
#include <iostream>
#include <mutex>
using namespace orderbook;

Order::Order(Orderside side, uint clientId, uint orderId, uint64_t timestamp, float price, float size) : side(side), timestamp(timestamp), price(price), size(size), clientId(clientId), orderId(orderId)
{
}

bool Order::operator<(const Order& other) const
{
    if (price != other.price)
    {
        return price < other.price;
    }
    else if (timestamp != other.timestamp)
    {
        return timestamp < other.timestamp;
    }
    else if (clientId != other.clientId)
    {
        return clientId < other.clientId;
    }
    return orderId < other.orderId;
}

bool Order::operator>(const Order& other) const
{
    if (operator==(other)) // if orders are equal then return false
        return false;
    return !operator<(other);
}

bool Order::operator==(const Order& other) const
{
    return side == other.side && clientId == other.clientId && orderId == other.orderId;
}
bool Orderbook::add_order(Order& order)
{
    const auto orderKey = std::make_pair(order.clientId, order.orderId);
    if(placedOrders.count(orderKey))
        return false; // order already exists
    bool orderAdded = false;
    std::unique_lock lk(mtx);

    if (order.side == Orderside::ask)
    {
        auto [_a,b] = asks.insert(order);
        orderAdded = b;
    }
    else if(order.side == Orderside::bid)
    {
        auto [_a,b] = bids.insert(order);
        orderAdded = b;
    }
    else
    {
        return false;
    }
    auto [_ite, addedInPlacedOrder] = placedOrders.insert(std::make_pair(orderKey, order));
    return orderAdded && addedInPlacedOrder;
}

void Orderbook::match(Order& order)
{
    //TODO
}

bool Orderbook::cancel_order(uint clientId, uint orderId)
{
    const auto orderKey = std::make_pair(clientId, orderId);
    auto iteOrder = placedOrders.find(orderKey);
    if(iteOrder == placedOrders.end())
        return false;
    
    const Order& order = iteOrder->second;
    bool orderErased = false;
    std::unique_lock lk(mtx);
    if(order.side == Orderside::ask)
    {
        orderErased = asks.erase(order) == 1;
    }
    else if(order.side == Orderside::bid)
    {
        orderErased = bids.erase(order) == 1;
    }
    else
    {
        return false;
    }
    return placedOrders.erase(orderKey) == 1 && orderErased;
}
void Orderbook::flush()
{
    std::unique_lock lk(mtx);
    asks.clear();
    bids.clear();
    placedOrders.clear();
}

Order Orderbook::getMinAsk() const
{
    std::shared_lock lk(mtx);
    return *asks.begin();
}
Order Orderbook::getMaxBid() const
{
    std::shared_lock lk(mtx);
    return *bids.begin();
}