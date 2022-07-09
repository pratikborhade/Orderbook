#include "orderbook.hpp"
#include <iostream>
#include <mutex>
using namespace orderbook;

Order::Order() : side(Orderside::buy), price(-1), size(-1), clientId(-1), orderId(-1)
{
}

Order::Order(Orderside side, int clientId, int orderId, int price, int size) :side(side), price(price), size(size), clientId(clientId), orderId(orderId)
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
    order.timestamp = ++gTimestamp;
    if (order.side == Orderside::buy)
    {
        auto [_a,b] = asks.insert(order);
        orderAdded = b;
    }
    else if(order.side == Orderside::sell)
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
    if(order.side == Orderside::buy)
    {
        orderErased = asks.erase(order) == 1;
    }
    else if(order.side == Orderside::sell)
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
    if(asks.empty())
        return Order();
    std::shared_lock lk(mtx);
    return *asks.begin();
}
Order Orderbook::getMaxBid() const
{
    if(bids.empty())
        return Order();
    std::shared_lock lk(mtx);
    return *bids.begin();
}