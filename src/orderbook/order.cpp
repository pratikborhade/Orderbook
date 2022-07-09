#include "order.h"

using namespace orderbook;

Order::Order() : side(Orderside::buy), price(-1), size(-1), clientId(-1), orderId(-1)
{
}

Order::Order(Orderside side, int clientId, int orderId, int price, int size) :side(side), price(price), size(size), clientId(clientId), orderId(orderId)
{
}

bool Order::isInvalid() const
{
    static Order invalid = Order();
    return *this == invalid;
}

bool Order::operator==(const Order& other) const
{
    return side == other.side && clientId == other.clientId && orderId == other.orderId;
}
