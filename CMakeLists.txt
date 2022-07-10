cmake_minimum_required(VERSION 3.0.0)
project(kraken-test VERSION 0.1.0)

include(CTest)
enable_testing()
include_directories(src)

add_library(orderbook src/orderbook/orderbook.cpp src/orderbook/orders.cpp)

add_executable(kraken-test src/main.cpp)
target_link_libraries(kraken-test PRIVATE orderbook)

target_compile_features(orderbook PRIVATE cxx_std_17)
target_compile_features(kraken-test PRIVATE cxx_std_17)

add_executable(cpp_test src/tests/orderbook_tests.cpp src/tests/tests.cpp)
target_link_libraries(cpp_test PRIVATE orderbook)

add_test(NAME orderbook_test_empty_orderbook COMMAND $<TARGET_FILE:cpp_test> orderbook_test_empty_orderbook)
add_test(NAME orderbook_test_flush COMMAND $<TARGET_FILE:cpp_test> orderbook_test_flush)
add_test(NAME orderbook_test_add_orders COMMAND $<TARGET_FILE:cpp_test> orderbook_test_add_orders)
add_test(NAME orderbook_test_cancel_orders COMMAND $<TARGET_FILE:cpp_test> orderbook_test_cancel_orders)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
