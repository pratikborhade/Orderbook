#include "orderbook.hpp"
#include <iostream>
#include <mutex>
using namespace orderbook;


bool Orderbook::AsksComparator::operator()(const Order& a, const Order& b) const
{
    if (a.price != b.price)
    {
        return a.price < b.price;
    }
    else if (a.timestamp != b.timestamp)
    {
        return a.timestamp < b.timestamp;
    }
    else if (a.clientId != b.clientId)
    {
        return a.clientId < b.clientId;
    }
    return a.orderId < b.orderId;
}

bool Orderbook::BidsComparator::operator()(const Order& a, const Order& b) const
{
    if (a.price != b.price)
    {
        return a.price > b.price;
    }
    else if (a.timestamp != b.timestamp)
    {
        return a.timestamp < b.timestamp;
    }
    else if (a.clientId != b.clientId)
    {
        return a.clientId < b.clientId;
    }
    return a.orderId < b.orderId;
}

bool Orderbook::add_order(Order& order, MatchFunctor& matchFunctor)
{
    const auto orderKey = std::make_pair(order.clientId, order.orderId);
    if(placedOrders.count(orderKey))
        return false; // order already exists
    order.timestamp = ++gTimestamp;

    std::unique_lock<std::shared_mutex> lk(mtx);
    if (match(order, matchFunctor)) // check if order matches any exisiting orders
        return true;

    bool orderAdded = false;
    if (order.side == Orderside::sell)
    {
        auto [_a,b] = asks.insert(order);
        orderAdded = b;
    }
    else if(order.side == Orderside::buy)
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

bool Orderbook::cancel_order(int clientId, int orderId)
{
    const auto orderKey = std::make_pair(clientId, orderId);
    auto iteOrder = placedOrders.find(orderKey);
    if(iteOrder == placedOrders.end())
        return false;
    
    const Order& order = iteOrder->second;
    bool orderErased = false;
    std::unique_lock lk(mtx);
    if(order.side == Orderside::sell)
    {
        orderErased = asks.erase(order) == 1;
    }
    else if(order.side == Orderside::buy)
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

bool Orderbook::match(Order& order, MatchFunctor& matchFunctor)
{
    auto compartor = [](const Order& order, const Order& book_order) -> bool
    {
        if (order.side == Orderside::buy) // <- will get min ask
        {
            return book_order.price <= order.price || order.price == 0;
        }
        else if (order.side == Orderside::sell) // <- will get max bid
        {
            return book_order.price >= order.price || order.price == 0;
        }
        return false;
    };
    
    auto next = [this](Orderside side) -> Order
    {
        if (side == Orderside::buy)
        {
            if (asks.empty())
                return Order();
            return *asks.begin();
        }
        else if (side == Orderside::sell)
        {
            if (bids.empty())
                return Order();
            return *bids.begin();
        }
        return Order();
    };

    // returns true if order is not yet fullfilled
    auto consume = [this, matchFunctor](Order& order) -> bool
    {
        Order bookOrder;
        if (order.side == Orderside::buy)
        {
            auto ite = asks.begin();
            bookOrder = *ite;
            asks.erase(ite);
        }
        else if (order.side == Orderside::sell)
        {
            auto ite = bids.begin();
            bookOrder = *ite;
            bids.erase(ite);
        }
        if (matchFunctor)
        {
            matchFunctor(bookOrder, order, bookOrder.price, std::min(bookOrder.size, order.size));
        }
        if (order.size >= bookOrder.size)
        {
            order.size -= bookOrder.size;
            return true;
        }
        else
        {
            bookOrder.size -= order.size;
            order.size = 0;
            if (order.side == Orderside::buy)
            {
                asks.insert(bookOrder);
            }
            else
            {
                bids.insert(bookOrder);
            }
            return false;
        }
    };

    Order current = next(order.side);
    
    while (order.size > 0 && !current.isInvalid() && compartor(order, current))
    {
        if (!consume(order))
            return true;
        current = next(order.side);
    }
    return order.size > 0 ? false : true;
}