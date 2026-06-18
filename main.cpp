#include "orderbook/OrderBook.hpp"
#include <iostream>
#include <cassert>

using namespace orderbook;

void test_order_addition() {
    OrderBook ob;
    ob.add_order(1, 100, 10, Side::Buy);
    ob.add_order(2, 100, 15, Side::Buy);
    ob.add_order(3, 101, 5, Side::Buy);
    
    assert(ob.size() == 3);
    assert(ob.get_volume_at_price(100, Side::Buy) == 25);
    assert(ob.get_volume_at_price(101, Side::Buy) == 5);
    assert(ob.get_volume_at_price(100, Side::Sell) == 0);
    
    std::cout << "test_order_addition passed!\n";
}

void test_matching_exact() {
    OrderBook ob;
    // Rest a buy order
    ob.add_order(1, 100, 10, Side::Buy);
    
    // Sell exact same quantity at crossing price
    ob.add_order(2, 100, 10, Side::Sell);
    
    // Both should match and be removed
    assert(ob.size() == 0);
    assert(ob.get_volume_at_price(100, Side::Buy) == 0);
    assert(ob.get_volume_at_price(100, Side::Sell) == 0);
    
    std::cout << "test_matching_exact passed!\n";
}

void test_matching_partial() {
    OrderBook ob;
    // Rest a buy order
    ob.add_order(1, 100, 10, Side::Buy);
    
    // Sell smaller quantity at crossing price
    ob.add_order(2, 100, 4, Side::Sell);
    
    // Buy order should have 6 left, sell order is fully filled
    assert(ob.size() == 1);
    assert(ob.get_volume_at_price(100, Side::Buy) == 6);
    assert(ob.get_volume_at_price(100, Side::Sell) == 0);
    
    std::cout << "test_matching_partial passed!\n";
}

void test_matching_multiple_levels() {
    OrderBook ob;
    // Sell levels: 10 at 101, 10 at 102
    ob.add_order(1, 101, 10, Side::Sell);
    ob.add_order(2, 102, 10, Side::Sell);
    
    // Large buy order crossing both levels
    ob.add_order(3, 103, 15, Side::Buy);
    
    // 10 matched at 101, 5 matched at 102
    // Remaining at 102 should be 5
    assert(ob.size() == 1);
    assert(ob.get_volume_at_price(101, Side::Sell) == 0);
    assert(ob.get_volume_at_price(102, Side::Sell) == 5);
    assert(ob.get_volume_at_price(103, Side::Buy) == 0);
    
    std::cout << "test_matching_multiple_levels passed!\n";
}

void test_order_cancellation() {
    OrderBook ob;
    ob.add_order(1, 100, 10, Side::Buy);
    ob.add_order(2, 100, 15, Side::Buy);
    
    // Cancel the second order
    ob.cancel_order(2);
    
    assert(ob.size() == 1);
    assert(ob.get_volume_at_price(100, Side::Buy) == 10);
    
    // Cancel the first order
    ob.cancel_order(1);
    
    assert(ob.size() == 0);
    assert(ob.get_volume_at_price(100, Side::Buy) == 0);
    
    std::cout << "test_order_cancellation passed!\n";
}

void test_duplicate_order_id() {
    OrderBook ob;
    ob.add_order(1, 100, 10, Side::Buy);
    ob.add_order(1, 100, 20, Side::Buy); // Should be ignored
    
    assert(ob.size() == 1);
    assert(ob.get_volume_at_price(100, Side::Buy) == 10);
    
    std::cout << "test_duplicate_order_id passed!\n";
}

void test_non_crossing_insertion() {
    OrderBook ob;
    ob.add_order(1, 99, 10, Side::Buy);
    ob.add_order(2, 101, 20, Side::Sell);
    
    assert(ob.size() == 2);
    assert(ob.get_volume_at_price(99, Side::Buy) == 10);
    assert(ob.get_volume_at_price(101, Side::Sell) == 20);
    
    std::cout << "test_non_crossing_insertion passed!\n";
}

void test_invalid_cancellations() {
    OrderBook ob;
    ob.add_order(1, 100, 10, Side::Buy);
    
    // Cancel invalid order id
    ob.cancel_order(999);
    
    assert(ob.size() == 1);
    assert(ob.get_volume_at_price(100, Side::Buy) == 10);
    
    std::cout << "test_invalid_cancellations passed!\n";
}

int main() {
    std::cout << "Running OrderBook tests...\n";
    test_order_addition();
    test_matching_exact();
    test_matching_partial();
    test_matching_multiple_levels();
    test_order_cancellation();
    test_duplicate_order_id();
    test_non_crossing_insertion();
    test_invalid_cancellations();
    std::cout << "All tests passed successfully!\n";
    return 0;
}
