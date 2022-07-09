cmake_minimum_required(VERSION 3.0.0)
project(kraken-test VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(orderbook src/orderbook/orderbook.cpp)

add_executable(kraken-test src/main.cpp)
target_link_libraries(kraken-test PRIVATE orderbook)

target_compile_features(orderbook PRIVATE cxx_std_17)
target_compile_features(kraken-test PRIVATE cxx_std_17)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
