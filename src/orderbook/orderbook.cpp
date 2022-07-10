#include "orderbook.hpp"
#include <iostream>
#include <mutex>
using namespace orderbook;

bool Orderbook::add_order(Orderside side, int clientId, int orderId, int price, int quantity, MatchFunctor& matchFunctor)
{
    const auto orderKey = std::make_pair(clientId, orderId);
    if(placedOrders.count(orderKey))
        return false; // order already exists

    std::unique_lock<std::shared_mutex> lk(mtx);
    if (match(side, clientId, orderId, price, quantity, matchFunctor)) // check if order matches any exisiting orders
        return true;

    bool orderAdded = false;
    auto addOrder = [](auto& container, int clientId, int orderId, int price, int quantity) -> bool
    {
        auto ite = container.find(price);
        if(ite == container.end())
        {
            std::unique_ptr<Orders> orders(new Orders);
            orders->add_order(clientId, orderId, quantity);
            container.insert(std::make_pair(price, std::move(orders)));
            return true;
        }
        else
        {
            return ite->second->add_order(clientId, orderId, quantity);
        }
    };
    if (side == Orderside::sell)
    {
        orderAdded = addOrder(asks, clientId, orderId, price, quantity);
    }
    else if(side == Orderside::buy)
    {
        orderAdded = addOrder(bids, clientId, orderId, price, quantity);
    }
    else
    {
        return false;
    }
    auto [_ite, addedInPlacedOrder] = placedOrders.insert(std::make_pair(orderKey, std::make_pair(price, side)));
    return orderAdded && addedInPlacedOrder;
}

bool Orderbook::cancel_order(int clientId, int orderId)
{
    const auto orderKey = std::make_pair(clientId, orderId);
    auto iteOrder = placedOrders.find(orderKey);
    if(iteOrder == placedOrders.end())
        return false;
    
    int price = iteOrder->second.first;
    Orderside side = iteOrder->second.second;
    bool orderErased = false;
    std::unique_lock lk(mtx);
    auto removeOrder = [](auto& container, int clientId, int orderId, int price) -> bool
    {
        auto ite = container.find(price);
        if(ite == container.end())
        {
            return false;
        }
        else
        {
            bool orderRemoved = ite->second->remove_order(clientId, orderId);
            if(ite->second->size == 0)
            {
                container.erase(ite);
            }
            return orderRemoved;
        }
    };

    if (side == Orderside::sell)
    {
        orderErased = removeOrder(asks, clientId, orderId, price);
    }
    else if(side == Orderside::buy)
    {
        orderErased = removeOrder(bids, clientId, orderId, price);
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

std::pair<int, int> Orderbook::get_min_ask() const
{
    if(asks.empty())
        return std::make_pair(-1, -1);
    std::shared_lock lk(mtx);
    return std::make_pair( asks.begin()->first, asks.begin()->second->size);
}
std::pair<int, int> Orderbook::get_max_bid() const
{
    if(bids.empty())
        return std::make_pair(-1, -1);
    std::shared_lock lk(mtx);
    return std::make_pair( bids.begin()->first, bids.begin()->second->size);
}

bool Orderbook::match(Orderside side, int clientId, int orderId, int price, int& quantity, MatchFunctor& matchFunctor)
{
    auto comparator = [](const Orderside side, int price, const auto& asks, const auto& bids) -> bool
    {
        if (side == Orderside::buy) // <- will get min ask
        {
            return asks.size() > 0 && (asks.begin()->first <= price || price == 0);
        }
        else if (side == Orderside::sell) // <- will get max bid
        {
            return bids.size() > 0 && (bids.begin()->first >= price || price == 0);
        }
        return false;
    };

    auto consume = [side](auto& container, int clientId, int orderId, int& quantity, MatchFunctor& matchFunctor)
    {
        auto ite = container.begin();
        int book_price = ite->first;
        auto& details = ite->second;
        bool eraseIterator = false;
        if (details->size > quantity)
        {
            auto current = details->orderDetails.begin();
            while(current != details->orderDetails.end() && quantity > 0)
            {
                matchFunctor(side, current->clientId, current->orderId, clientId, orderId, book_price, std::min(current->quantity, quantity));
                if(current->quantity > quantity)
                {
                    current->quantity -= quantity;
                    details->size -= quantity;
                    quantity = 0;
                    break;
                }
                quantity -= current->quantity;
                details->size -= current->quantity;
                ++current;
            }
            details->orderDetails.erase(details->orderDetails.begin(), current);
        }
        else
        {
            quantity -= details->size;
            for (const auto& detail : details->orderDetails)
            {
                matchFunctor(side, detail.clientId, detail.orderId, clientId, orderId, book_price, detail.quantity);
            }
            container.erase(ite);
        }
    };

    while (quantity > 0 && comparator(side, price, asks, bids))
    {
        if (side == Orderside::buy) // <- will get min ask
        {
            consume(asks, clientId, orderId, quantity, matchFunctor);
        }
        else if (side == Orderside::sell) // <- will get max bid
        {
            consume(bids, clientId, orderId, quantity, matchFunctor);
        }
    }
    return quantity > 0 ? false : true;
}