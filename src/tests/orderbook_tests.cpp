#include "orderbook/orderbook.hpp"
#include "test_utils.hpp"
#include <cstring>

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
    else if(std::strcmp("orderbook_test_flush", testName) == 0)
    {
        return orderbook_test_flush();
    }
    else
    {
        return -1;
    }
}