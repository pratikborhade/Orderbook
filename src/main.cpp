#include <iostream>
#include <memory>
#include <fstream>
#include "orderbook/orderbook.hpp"

using namespace orderbook;
/*
#Format new order:
# N, user(int),symbol(string),price(int),qty(int),side(char B or S),userOrderId(int)
#
#Format cancel order:
# C, user(int),userOrderId(int)
#
#Format flush order book:
# F

# Notes:
# * Price is 0 for market order, <>0 for limit order
# * TOB = Top Of Book, highest bid, lowest offer
# * Between scenarios flush order books
*/
struct InputCommand
{
    virtual void execute(std::map<std::string, Orderbook>& orderbooks, std::ostream& o) const = 0;
};

struct OrderbookChangesTracker
{
    const Order minAsk, maxBid;
    OrderbookChangesTracker(const Orderbook& orderbook) : minAsk(orderbook.getMinAsk()), maxBid(orderbook.getMaxBid())
    {}

    static bool CompareBids(const Order& a, const Order& b)
    {
        return a.orderId != b.orderId || a.clientId != b.clientId || a.size != b.size;
    }

    void check(const Orderbook& orderbook, std::ostream& o) const
    {
        Order newMinAsk = orderbook.getMinAsk(), newMaxBid = orderbook.getMaxBid();
        if(CompareBids(newMinAsk, minAsk))
        {
            if(newMinAsk == Order())
            {
                o << "S - -\n";
            }
            o << "S " << newMinAsk.price << " " << newMinAsk.size << "\n";
        }
        if(CompareBids(newMaxBid, maxBid))
        {
            if(newMaxBid == Order())
            {
                o << "B - -";
            }
            o << "B " << newMaxBid.price << " " << newMaxBid.size << "\n";
        }
    }
};

struct NewOrderCommand : InputCommand 
{
    int userId;
    std::string symbol;
    int price;
    int quantity;
    Orderside side;
    int orderId;
    NewOrderCommand(int userId, const std::string& symbol, int price, int quantity, Orderside side, int orderId)
    : userId(userId), symbol(symbol), price(price), quantity(quantity), side(side), orderId(orderId)
    {
    }

    virtual void execute(std::map<std::string, Orderbook>& orderbooks, std::ostream& o) const override
    {
        OrderbookChangesTracker tracker(orderbooks[symbol]);
        Order newOrder(side, userId, orderId, price, quantity);
        if( orderbooks[symbol].add_order(newOrder) )
        {
            o << "A " << userId << " " << orderId << "\n";
            tracker.check(orderbooks[symbol], o);
        }
    }
};

struct CancelOrderCommand : InputCommand
{
    int userId;
    int orderId;
    CancelOrderCommand(int userId, int orderId) : userId(userId), orderId(orderId)
    {}

    virtual void execute(std::map<std::string, Orderbook>& orderbooks, std::ostream& o) const override
    {
        std::for_each(orderbooks.begin(), orderbooks.end(), [this, &o](auto &orderbook)
        {
            OrderbookChangesTracker tracker(orderbook.second);
            if( orderbook.second.cancel_order(userId, orderId) )
            {
                o << "C " << userId << " " << orderId << "\n";
                tracker.check(orderbook.second, o);
            }
        });
    }
};

struct FlushCommand : InputCommand
{
    virtual void execute(std::map<std::string, Orderbook>& orderbooks, std::ostream& o) const override
    {
        orderbooks.clear();
        o << "\n\n\n";
    }
};
using InputCommandPtr = std::unique_ptr<InputCommand>;
std::vector<InputCommandPtr> ParseInputCommands(std::istream &stream)
{
    std::string line;
    std::vector<InputCommandPtr> commands;
    while (std::getline(stream, line))
    {
        switch (*line.begin())
        {
        case '#':
            break;
        case 'N':
        {
            int userId;
            char symbol[100];
            int price;
            int quantity;
            char side;
            int orderId;
            sscanf(line.c_str(), "N %d %s %d %d %c %d", &userId, symbol, &price, &quantity, &side, &orderId);
            Orderside orderside = side == 'B'? Orderside::buy : Orderside::sell;
            commands.push_back( InputCommandPtr(new NewOrderCommand(userId, symbol, price, quantity, orderside, orderId)) );
        }
            break;
        case 'C':
        {
            int userId, orderId;
            sscanf(line.c_str(), "C %d %d", &userId, &orderId);
            commands.push_back(InputCommandPtr(new CancelOrderCommand(userId, orderId)));
        }
            break;
        case 'F':
            commands.push_back(InputCommandPtr(new FlushCommand));
            break;
        default:
            break;
        }
    }
    return commands;
}

int main(int argc, char** argv) {
    if(argc != 3)
    {
        std::cout << "Input format is command input_file output_file\n";
        return -1;
    }
    
    std::fstream inFile(argv[1], std::ios_base::in);
    if(!inFile.is_open())
    {
        std::cout << "Cannot open input file " << argv[1] << " please check if the location exists";
    }

    auto commands = ParseInputCommands(inFile);
    inFile.close();

    std::fstream outFile(argv[2], std::ios_base::out);
    if(!outFile.is_open())
    {
        std::cout << "Cannot open output file " << argv[2];
    }

    std::map<std::string, Orderbook> orderbooks;
    for (const auto& command : commands)
    {
        command->execute(orderbooks, outFile);
    }
    return 0;
}
