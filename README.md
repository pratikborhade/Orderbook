# Build
Do the following command at the source root directory.
`mkdir release && cd release && cmake .. && make`

# Run
`./kraken-test inputFile.csv`

# Run Unittests
`make test`

# Design
## Architectural design
The code is written in C++17 and I tried to optimize as much as possible keeping design very simple. The class which implements orderbook is `orderbook::Orderbook`.

Following are the complexities:
### `add_order`
For `k` = number of matches in the orderbook (should be relatively small)
For `n` = number of orders present in orderbook
Complexity : `O(k) + O(log(n))`

### `cancel_order`
For `n` = number of orders present in orderbook

Worstcase complexity : `O(n)` -> If all n orders have the same price
Averagecase complexity : `O(nlog(n))`

There are lot of things that can be improved. The most of the improvement is dependent on specs. Ideally when order is matched there should be two onMatched functors on for order that is matched on order side and other for current order.
Also the cancel_order can be done in O(log(n)) for worst case complexity it involves using list instead of vector and tracking all the iterators.