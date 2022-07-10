#include "orderbook_tests.hpp"
#include <iostream>
#include <cstring>

int main(int argc, const char ** argv)
{
    if(argc >= 2)
    {
        if( std::strncmp(argv[1], "orderbook", 9) == 0 )
        {
            return run_orderbook_tests(argv);
        }
        else
        {
            std::cout << "no test named " << argv[1];
            return -1;
        }
    }
    std::cout << "invalid arguments ";
    return -1;
}