#include "orderbook/orderbook.hpp"
#include "test_utils.hpp"
#include <cstring>
#include <vector>
#include <iostream>

using namespace orderbook;
int orderbook_test_empty_orderbook()
{
    Orderbook book;
    assert_equal(book.get_max_bid(), std::pair(-1, -1));
    assert_equal(book.get_min_ask(), std::pair(-1, -1));
    assert_equal(book.cancel_order(1,1), false);
    return 0;
}

int orderbook_test_add_orders()
{
    Orderbook book;
    assert_equal(book.add_order(Orderside::buy, 1, 1, 100, 100, nullptr), true);
    assert_equal(book.add_order(Orderside::sell, 2, 1, 110, 100, nullptr), true);
    assert_equal(book.get_max_bid(), std::pair(100, 100));
    assert_equal(book.get_min_ask(), std::pair(110, 100));

    assert_equal(book.add_order(Orderside::buy, 2, 2, 100, 100, nullptr), true);
    assert_equal(book.add_order(Orderside::sell, 1, 2, 110, 100, nullptr), true);
    assert_equal(book.get_max_bid(), std::pair(100, 200));
    assert_equal(book.get_min_ask(), std::pair(110, 200));

    assert_equal(book.add_order(Orderside::buy, 2, 3, 104, 100, nullptr), true);
    assert_equal(book.add_order(Orderside::sell, 1, 3, 106, 100, nullptr), true);
    assert_equal(book.get_max_bid(), std::pair(104, 100));
    assert_equal(book.get_min_ask(), std::pair(106, 100));

    // direct match
    assert_equal(book.add_order(Orderside::buy, 2, 4, 105, 100, nullptr), true);
    assert_equal(book.add_order(Orderside::sell, 1, 4, 105, 100, nullptr), true);
    assert_equal(book.get_max_bid(), std::pair(104, 100));
    assert_equal(book.get_min_ask(), std::pair(106, 100));

    return 0;
}

int orderbook_test_cancel_orders()
{
    Orderbook book;
    book.add_order(Orderside::buy, 1, 1, 100, 100, nullptr);
    book.add_order(Orderside::sell, 2, 1, 110, 100, nullptr);
    book.add_order(Orderside::buy, 2, 2, 100, 100, nullptr);
    book.add_order(Orderside::sell, 1, 2, 110, 100, nullptr);
    book.add_order(Orderside::buy, 2, 3, 104, 100, nullptr);
    book.add_order(Orderside::sell, 1, 3, 106, 100, nullptr);
    book.add_order(Orderside::buy, 2, 5, 100, 100, nullptr);
    book.add_order(Orderside::sell, 1, 5, 110, 100, nullptr);
    // direct match
    book.add_order(Orderside::buy, 2, 4, 105, 100, nullptr);
    book.add_order(Orderside::sell, 1, 4, 105, 100, nullptr);

    assert_equal(book.cancel_order(2,4), false);
    assert_equal(book.cancel_order(1,4), false);

    assert_equal(book.get_max_bid(), std::pair(104, 100));
    assert_equal(book.get_min_ask(), std::pair(106, 100));
    assert_equal(book.cancel_order(1,3), true);
    assert_equal(book.cancel_order(2,3), true);
    assert_equal(book.get_max_bid(), std::pair(100, 300));
    assert_equal(book.get_min_ask(), std::pair(110, 300));

    assert_equal(book.cancel_order(1,2), true);
    assert_equal(book.cancel_order(2,2), true);
    assert_equal(book.get_max_bid(), std::pair(100, 200));
    assert_equal(book.get_min_ask(), std::pair(110, 200));

    assert_equal(book.cancel_order(1,1), true);
    assert_equal(book.cancel_order(2,1), true);
    assert_equal(book.get_max_bid(), std::pair(100, 100));
    assert_equal(book.get_min_ask(), std::pair(110, 100));

    assert_equal(book.cancel_order(1,5), true);
    assert_equal(book.cancel_order(2,5), true);
    assert_equal(book.get_max_bid(), std::pair(-1, -1));
    assert_equal(book.get_min_ask(), std::pair(-1, -1));
    return 0;
}

struct Match
{
    Orderside side;
    int bookUserId;
    int bookOrderId;
    int userId;
    int orderId;
    int price;
    int quantity;

    bool operator==(const Match& other) const
    {
        return side == other.side && 
            bookUserId == other.bookUserId && 
            bookOrderId == other.bookOrderId &&
            userId == other.userId &&
            orderId == other.orderId &&
            price == other.price &&
            quantity == other.quantity; 
    }
};

void print_matches(const std::vector<Match>& matches)
{
    for(const auto& match : matches)
    {
        std::cerr << short(match.side) << " " << match.bookUserId << " " << match.bookOrderId << " " << match.userId << " " << match.orderId << " " << match.price << " " << match.quantity << "\n";
    }
}

int orderbook_test_match_buy_side()
{
    Orderbook book;
    std::vector<Match> recent_matches;
    Orderbook::MatchFunctor functor = [&recent_matches](Orderside side, int a, int b, int c, int d, int p, int q) -> bool
    {
        recent_matches.push_back(Match{side, a, b, c, d, p, q});
        return true;
    };

    // test batch on an order
    // create a buy order for price = 90 and quantity = 100
    assert_equal(book.add_order(Orderside::buy, 1, 1, 90, 100, functor), true);
    assert_equal(recent_matches.size(), 0);
    // create a sell order at price and quantity = 30
    assert_equal(book.add_order(Orderside::sell, 2, 101, 90, 30, functor), true);
    assert_equal(recent_matches.size(), 1);
    assert_equal(recent_matches, std::vector<Match>({ Match{Orderside::sell, 1, 1, 2, 101, 90, 30} }));
    recent_matches.clear();

    // create a sell order for below the price and quantity 30
    assert_equal(book.add_order(Orderside::sell, 2, 101, 20, 30, functor), true);
    assert_equal(recent_matches.size(), 1);
    assert_equal(recent_matches, std::vector<Match>({ Match{Orderside::sell, 1, 1, 2, 101, 90, 30} }));
    recent_matches.clear();

    // create a sell order for below the price and quantity = 1000, should match only 40 orders as there are only 40 remaining in orderbook
    assert_equal(book.add_order(Orderside::sell, 2, 101, 15, 1000, functor), true);
    assert_equal(recent_matches.size(), 1);
    assert_equal(recent_matches, std::vector<Match>({ Match{Orderside::sell, 1, 1, 2, 101, 90, 40} }));
    recent_matches.clear();
    // remaining order
    assert_equal(book.get_min_ask(), std::make_pair(15, 1000-40));
    book.flush();
    
    // create multiple buy orders
    auto market_maker = [&book, functor]()
    {
        book.add_order(Orderside::buy, 1, 1, 100, 20, functor);
        book.add_order(Orderside::buy, 1, 2, 99, 30, functor);
        book.add_order(Orderside::buy, 1, 3, 98, 30, functor);
        book.add_order(Orderside::buy, 1, 4, 97, 40, functor);
        book.add_order(Orderside::buy, 1, 5, 98, 40, functor);
    };
    // test match on multiple orders
    market_maker();
    assert_equal(book.add_order(Orderside::sell, 2, 101, 99, 50, functor), true);
    assert_equal(recent_matches.size(), 2);
    assert_equal(recent_matches, 
        std::vector<Match>(
            { 
                Match{Orderside::sell, 1, 1, 2, 101, 100, 20},
                Match{Orderside::sell, 1, 2, 2, 101, 99, 30}
            }
        ));
    assert_equal(book.get_max_bid(), std::make_pair(98, 70));
    recent_matches.clear();
    book.flush();
    
    market_maker();
    assert_equal(book.add_order(Orderside::sell, 2, 101, 98, 100, functor), true);
    assert_equal(recent_matches.size(), 4);
    assert_equal(recent_matches, 
        std::vector<Match>(
            { 
                Match{Orderside::sell, 1, 1, 2, 101, 100, 20},
                Match{Orderside::sell, 1, 2, 2, 101, 99, 30},
                Match{Orderside::sell, 1, 3, 2, 101, 98, 30},
                Match{Orderside::sell, 1, 5, 2, 101, 98, 20}
            }
        ));
    assert_equal(book.get_max_bid(), std::make_pair(98, 20));
    recent_matches.clear();
    book.flush();

    market_maker();
    assert_equal(book.add_order(Orderside::sell, 2, 101, 90, 80, functor), true);
    assert_equal(recent_matches.size(), 3);
    assert_equal(recent_matches, 
        std::vector<Match>(
            { 
                Match{Orderside::sell, 1, 1, 2, 101, 100, 20},
                Match{Orderside::sell, 1, 2, 2, 101, 99, 30},
                Match{Orderside::sell, 1, 3, 2, 101, 98, 30}
            }
        ));
    assert_equal(book.get_max_bid(), std::make_pair(98, 40));
    recent_matches.clear();
    book.flush();
    
    return 0;
}

int orderbook_test_match_sell_side()
{
    Orderbook book;
    std::vector<Match> recent_matches;
    Orderbook::MatchFunctor functor = [&recent_matches](Orderside side, int a, int b, int c, int d, int p, int q) -> bool
    {
        recent_matches.push_back(Match{side, a, b, c, d, p, q});
        return true;
    };

     // -- test same as above but the opposite side
    assert_equal(book.add_order(Orderside::sell, 1, 1, 90, 100, functor), true);
    assert_equal(recent_matches.size(), 0);
    assert_equal(book.add_order(Orderside::buy, 2, 101, 90, 30, functor), true);
    assert_equal(recent_matches.size(), 1);
    assert_equal(recent_matches, std::vector<Match>({ Match{Orderside::buy, 1, 1, 2, 101, 90, 30} }));
    recent_matches.clear();

    assert_equal(book.add_order(Orderside::buy, 2, 101, 100, 30, functor), true);
    assert_equal(recent_matches.size(), 1);
    assert_equal(recent_matches, std::vector<Match>({ Match{Orderside::buy, 1, 1, 2, 101, 90, 30} }));
    recent_matches.clear();

    assert_equal(book.add_order(Orderside::buy, 2, 101, 110, 1000, functor), true);
    assert_equal(recent_matches.size(), 1);
    assert_equal(recent_matches, std::vector<Match>({ Match{Orderside::buy, 1, 1, 2, 101, 90, 40} }));
    recent_matches.clear();
    // remaining order
    assert_equal(book.get_max_bid(), std::make_pair(110, 1000-40));
    book.flush();


    // create multiple sell orders
    auto market_maker = [&book, functor]()
    {
        book.add_order(Orderside::sell, 1, 1, 100, 20, functor);
        book.add_order(Orderside::sell, 1, 2, 99, 30, functor);
        book.add_order(Orderside::sell, 1, 3, 98, 30, functor);
        book.add_order(Orderside::sell, 1, 4, 97, 40, functor);
        book.add_order(Orderside::sell, 1, 5, 98, 40, functor);
    };
    // test match on multiple orders
    market_maker();
    assert_equal(book.add_order(Orderside::buy, 2, 101, 98, 110, functor), true);
    assert_equal(recent_matches.size(), 3);
    assert_equal(recent_matches, 
        std::vector<Match>(
            { 
                Match{Orderside::buy, 1, 4, 2, 101, 97, 40},
                Match{Orderside::buy, 1, 3, 2, 101, 98, 30},
                Match{Orderside::buy, 1, 5, 2, 101, 98, 40},
            }
        ));
    assert_equal(book.get_min_ask(), std::make_pair(99, 30));
    recent_matches.clear();
    book.flush();
    
    market_maker();
    assert_equal(book.add_order(Orderside::buy, 2, 101, 98, 100, functor), true);
    assert_equal(recent_matches.size(), 3);
    assert_equal(recent_matches, 
        std::vector<Match>(
            { 
                Match{Orderside::buy, 1, 4, 2, 101, 97, 40},
                Match{Orderside::buy, 1, 3, 2, 101, 98, 30},
                Match{Orderside::buy, 1, 5, 2, 101, 98, 30},
            }
        ));
    assert_equal(book.get_min_ask(), std::make_pair(98, 10));
    recent_matches.clear();
    book.flush();

    market_maker();
    assert_equal(book.add_order(Orderside::buy, 2, 101, 98, 200, functor), true);
    assert_equal(recent_matches.size(), 3);
    assert_equal(recent_matches, 
        std::vector<Match>(
            { 
                Match{Orderside::buy, 1, 4, 2, 101, 97, 40},
                Match{Orderside::buy, 1, 3, 2, 101, 98, 30},
                Match{Orderside::buy, 1, 5, 2, 101, 98, 40},
            }
        ));
    assert_equal(book.get_min_ask(), std::make_pair(99, 30));
    recent_matches.clear();
    book.flush();
    return 0;
}

int orderbook_test_market_orders()
{
    Orderbook book;
    std::vector<Match> recent_matches;
    Orderbook::MatchFunctor functor = [&recent_matches](Orderside side, int a, int b, int c, int d, int p, int q) -> bool
    {
        recent_matches.push_back(Match{side, a, b, c, d, p, q});
        return true;
    };

    // create multiple sell orders
    auto market_maker = [&book, functor](Orderside side)
    {
        book.add_order(side, 1, 1, 100, 20, functor);
        book.add_order(side, 1, 2, 99, 30, functor);
        book.add_order(side, 1, 3, 98, 30, functor);
        book.add_order(side, 1, 4, 97, 40, functor);
        book.add_order(side, 1, 5, 98, 40, functor);
    };

    market_maker(Orderside::sell);
    assert_equal(book.add_order(Orderside::buy, 2, 101, 0, 110, functor), true);
    assert_equal(recent_matches.size(), 3);
    assert_equal(recent_matches, 
        std::vector<Match>(
            { 
                Match{Orderside::buy, 1, 4, 2, 101, 97, 40},
                Match{Orderside::buy, 1, 3, 2, 101, 98, 30},
                Match{Orderside::buy, 1, 5, 2, 101, 98, 40},
            }
        ));
    assert_equal(book.get_min_ask(), std::make_pair(99, 30));
    recent_matches.clear();
    book.flush();

    market_maker(Orderside::sell);
    assert_equal(book.add_order(Orderside::buy, 2, 101, 0, 1000, functor), false);
    assert_equal(recent_matches.size(), 5);
    assert_equal(recent_matches, 
        std::vector<Match>(
            { 
                Match{Orderside::buy, 1, 4, 2, 101, 97, 40},
                Match{Orderside::buy, 1, 3, 2, 101, 98, 30},
                Match{Orderside::buy, 1, 5, 2, 101, 98, 40},
                Match{Orderside::buy, 1, 2, 2, 101, 99, 30},
                Match{Orderside::buy, 1, 1, 2, 101, 100, 20},
            }
        ));
    assert_equal(book.get_min_ask(), std::make_pair(-1, -1));
    recent_matches.clear();
    book.flush();

    market_maker(Orderside::buy);
    assert_equal(book.add_order(Orderside::sell, 2, 101, 0, 100, functor), true);
    assert_equal(recent_matches.size(), 4);
    assert_equal(recent_matches, 
        std::vector<Match>(
            { 
                Match{Orderside::sell, 1, 1, 2, 101, 100, 20},
                Match{Orderside::sell, 1, 2, 2, 101, 99, 30},
                Match{Orderside::sell, 1, 3, 2, 101, 98, 30},
                Match{Orderside::sell, 1, 5, 2, 101, 98, 20}
            }
        ));
    assert_equal(book.get_max_bid(), std::make_pair(98, 20));
    recent_matches.clear();
    book.flush();

    market_maker(Orderside::buy);
    assert_equal(book.add_order(Orderside::sell, 2, 101, 0, 1000, functor), false);
    assert_equal(recent_matches.size(), 5);
    assert_equal(recent_matches, 
        std::vector<Match>(
            { 
                Match{Orderside::sell, 1, 1, 2, 101, 100, 20},
                Match{Orderside::sell, 1, 2, 2, 101, 99, 30},
                Match{Orderside::sell, 1, 3, 2, 101, 98, 30},
                Match{Orderside::sell, 1, 5, 2, 101, 98, 40},
                Match{Orderside::sell, 1, 4, 2, 101, 97, 40},
            }
        ));
    assert_equal(book.get_max_bid(), std::make_pair(-1, -1));
    recent_matches.clear();
    book.flush();

    return 0;
}

int orderbook_test_flush()
{
    Orderbook book;
    book.add_order(Orderside::buy, 1, 1, 100, 100, nullptr);
    book.add_order(Orderside::sell, 2, 2, 102, 100, nullptr);
    assert_equal(book.get_max_bid(), std::pair(100, 100));
    assert_equal(book.get_min_ask(), std::pair(102, 100));

    book.flush();

    assert_equal(book.get_max_bid(), std::pair(-1, -1));
    assert_equal(book.get_min_ask(), std::pair(-1, -1));
    assert_equal(book.cancel_order(1,1), false);
    assert_equal(book.cancel_order(2,2), false);
    return 0;
}

int run_orderbook_tests(const char * testName)
{
    if(std::strcmp("orderbook_test_empty_orderbook", testName) == 0)
    {
        return orderbook_test_empty_orderbook();
    }
    else if(std::strcmp("orderbook_test_add_orders", testName) == 0)
    {
        return orderbook_test_add_orders();
    }
    else if(std::strcmp("orderbook_test_cancel_orders", testName) == 0)
    {
        return orderbook_test_cancel_orders();
    }
    else if(std::strcmp("orderbook_test_match_buy_side", testName) == 0)
    {
        return orderbook_test_match_buy_side();
    }
    else if(std::strcmp("orderbook_test_match_sell_side", testName) == 0)
    {
        return orderbook_test_match_sell_side();
    }
    else if(std::strcmp("orderbook_test_market_orders", testName) == 0)
    {
        return orderbook_test_market_orders();
    }
    else if(std::strcmp("orderbook_test_flush", testName) == 0)
    {
        return orderbook_test_flush();
    }
    else
    {
        return -1;
    }
}