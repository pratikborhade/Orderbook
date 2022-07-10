#include <iostream>
#include <memory>
#include <fstream>
#include <algorithm>
#include <string>
#include <sstream>
#include "orderbook/orderbook.hpp"

using namespace orderbook;
using namespace std::placeholders;

using OrderbookManager = std::map<std::string, Orderbook>;

// interface which will help us to parse and execute commands later
struct InputCommand
{
    virtual void execute(OrderbookManager& orderbooks, std::ostream& o) const = 0;
};

struct OrderbookChangesTracker
{
    
    std::pair<int, int> minAsk, maxBid;
    OrderbookChangesTracker(const Orderbook& orderbook) : minAsk(orderbook.get_min_ask()), maxBid(orderbook.get_max_bid())
    {
    }

    void check(const Orderbook& orderbook, std::ostream& o) const
    {
        std::pair<int, int> newMinAsk(orderbook.get_min_ask()), newMaxBid(orderbook.get_max_bid());
        if (newMinAsk != minAsk)
        {
            if (newMinAsk.second == -1)
            {
                o << "B, S, -, -\n";
                return;
            }
            o << "B, S, " << newMinAsk.first << ", " << newMinAsk.second << "\n";
        }
        if (newMaxBid != maxBid)
        {
            if (newMaxBid.second == -1)
            {
                o << "B, B, -, -\n";
                return;
            }
            o << "B, B, " << newMaxBid.first << ", " << newMaxBid.second << "\n";
        }
    }
};

void print_matched_transaction(std::ostream& o, Orderside orderside, int bookClientId, int bookClientOrderId, int clientId, int clientOrderId, int price, int quantity)
{
    const auto buyer = (orderside == Orderside::buy ? std::make_pair(clientId, clientOrderId) : std::make_pair(bookClientId, bookClientOrderId));
    const auto seller = (orderside == Orderside::sell ? std::make_pair(clientId, clientOrderId) : std::make_pair(bookClientId, bookClientOrderId));
    o << "T, " << buyer.first << ", " << buyer.second << ", " << seller.first << ", " << seller.second << ", " << price << ", " << quantity << "\n";
}

// This will help us print the comments in input file
struct PrintCommand : InputCommand
{
    std::string line;
    PrintCommand(std::string&& line) : line(std::move(line))
    {
    }

    virtual void execute(OrderbookManager&, std::ostream& o) const override
    {
        o << line << "\n";
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

    virtual void execute(OrderbookManager& orderbooks, std::ostream& o) const override
    {
        OrderbookChangesTracker tracker(orderbooks[symbol]);
        std::stringstream matchOrderSS;
        Orderbook::MatchFunctor matchFunctor = [&matchOrderSS](Orderside orderside, int bookClientId, int bookClientOrderId, int clientId, int clientOrderId, int price, int quantity) -> bool
        {
            print_matched_transaction(matchOrderSS, orderside, bookClientId, bookClientOrderId, clientId, clientOrderId, price, quantity);
            return true;
        };
        if (orderbooks[symbol].add_order(side, userId, orderId, price, quantity, matchFunctor))
        {
            o << "A, " << userId << ", " << orderId << "\n";
            auto matchedString = matchOrderSS.str();
            if (!matchedString.empty())
            {
                o << matchedString;
            }
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

    virtual void execute(OrderbookManager& orderbooks, std::ostream& o) const override
    {
        std::for_each(orderbooks.begin(), orderbooks.end(), [this, &o](auto& orderbook)
            {
                OrderbookChangesTracker tracker(orderbook.second);
                if (orderbook.second.cancel_order(userId, orderId))
                {
                    o << "C, " << userId << ", " << orderId << "\n";
                    tracker.check(orderbook.second, o);
                }
            });
    }
};

struct FlushCommand : InputCommand
{
    virtual void execute(OrderbookManager& orderbooks, std::ostream& o) const override
    {
        orderbooks.clear();
        o << "\n";
    }
};

using InputCommandPtr = std::unique_ptr<InputCommand>;
std::vector<InputCommandPtr> ParseInputCommands(std::istream& stream)
{
    std::string line;
    std::vector<InputCommandPtr> commands;
    while (std::getline(stream, line))
    {
        if (line.empty())
            continue;
        switch (*line.begin())
        {
        case '#':
            commands.push_back(InputCommandPtr(new PrintCommand(std::move(line))));
            break;
        case 'N':
        {
            int userId;
            char symbol[100];
            int price;
            int quantity;
            char side;
            int orderId;
            auto ret = sscanf(line.c_str(), "N, %d, %s %d, %d, %c, %d", &userId, symbol, &price, &quantity, &side, &orderId);
            Orderside orderside = side == 'B' ? Orderside::buy : Orderside::sell;
            commands.push_back(InputCommandPtr(new NewOrderCommand(userId, symbol, price, quantity, orderside, orderId)));
        }
        break;
        case 'C':
        {
            int userId, orderId;
            auto ret = sscanf(line.c_str(), "C, %d, %d", &userId, &orderId);
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
    if (argc != 2)
    {
        std::cout << "Input format is command input_file\n";
        return -1;
    }

    std::fstream inFile(argv[1], std::ios_base::in);
    if (!inFile.is_open())
    {
        std::cout << "Cannot open input file " << argv[1] << " please check if the location exists";
    }

    auto commands = ParseInputCommands(inFile);
    inFile.close();

    OrderbookManager orderbooks;
    for (const auto& command : commands)
    {
        command->execute(orderbooks, std::cout);
    }
    return 0;
}
