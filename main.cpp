#include "orderbook/OrderBook.hpp"
#include <iostream>
#include <cassert>

using namespace orderbook;

/**
 * @file main.cpp
 * @brief Unit tests validating OrderBook operations, execution rules, and edge cases.
 */

/**
 * @brief Tests basic order addition and volume querying.
 * 
 * Verifies that multiple buy orders are correctly added to the book,
 * that volumes are aggregated correctly at the respective price levels,
 * and that the size of the book increases accordingly.
 */
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

/**
 * @brief Tests full order matching with exact quantity.
 * 
 * Verifies that when an incoming sell order matches the exact quantity of
 * a resting buy order at a crossing price, both orders are fully filled and 
 * removed from the book.
 */
void test_matching_exact() {
    OrderBook ob;
    ob.add_order(1, 100, 10, Side::Buy);
    ob.add_order(2, 100, 10, Side::Sell);
    
    assert(ob.size() == 0);
    assert(ob.get_volume_at_price(100, Side::Buy) == 0);
    assert(ob.get_volume_at_price(100, Side::Sell) == 0);
    
    std::cout << "test_matching_exact passed!\n";
}

/**
 * @brief Tests partial matching of orders.
 * 
 * Verifies that when a sell order with a smaller quantity than the resting
 * buy order is added, the sell order is fully filled while the buy order is
 * partially filled and remains in the book with the remaining quantity.
 */
void test_matching_partial() {
    OrderBook ob;
    ob.add_order(1, 100, 10, Side::Buy);
    ob.add_order(2, 100, 4, Side::Sell);
    
    assert(ob.size() == 1);
    assert(ob.get_volume_at_price(100, Side::Buy) == 6);
    assert(ob.get_volume_at_price(100, Side::Sell) == 0);
    
    std::cout << "test_matching_partial passed!\n";
}

/**
 * @brief Tests order matching across multiple price levels.
 * 
 * Verifies that a large incoming order matches sequentially across multiple
 * price levels (following price-time priority) until its quantity is filled or
 * no more crossing levels remain.
 */
void test_matching_multiple_levels() {
    OrderBook ob;
    ob.add_order(1, 101, 10, Side::Sell);
    ob.add_order(2, 102, 10, Side::Sell);
    ob.add_order(3, 103, 15, Side::Buy);
    
    assert(ob.size() == 1);
    assert(ob.get_volume_at_price(101, Side::Sell) == 0);
    assert(ob.get_volume_at_price(102, Side::Sell) == 5);
    assert(ob.get_volume_at_price(103, Side::Buy) == 0);
    
    std::cout << "test_matching_multiple_levels passed!\n";
}

/**
 * @brief Tests standard order cancellation.
 * 
 * Verifies that orders can be successfully cancelled and removed from the book
 * in O(1) time, updating the total volume at the price level and removing the
 * level entirely if no orders remain.
 */
void test_order_cancellation() {
    OrderBook ob;
    ob.add_order(1, 100, 10, Side::Buy);
    ob.add_order(2, 100, 15, Side::Buy);
    
    ob.cancel_order(2);
    
    assert(ob.size() == 1);
    assert(ob.get_volume_at_price(100, Side::Buy) == 10);
    
    ob.cancel_order(1);
    
    assert(ob.size() == 0);
    assert(ob.get_volume_at_price(100, Side::Buy) == 0);
    
    std::cout << "test_order_cancellation passed!\n";
}

/**
 * @brief Tests that duplicate order IDs are rejected.
 * 
 * Verifies that adding an order with an ID that already exists in the book 
 * is ignored and has no impact on the book's state.
 */
void test_duplicate_order_id() {
    OrderBook ob;
    ob.add_order(1, 100, 10, Side::Buy);
    ob.add_order(1, 100, 20, Side::Buy);
    
    assert(ob.size() == 1);
    assert(ob.get_volume_at_price(100, Side::Buy) == 10);
    
    std::cout << "test_duplicate_order_id passed!\n";
}

/**
 * @brief Tests non-crossing order insertion.
 * 
 * Verifies that buy and sell orders that do not cross (bid price < ask price)
 * rest on the book without triggering any matches.
 */
void test_non_crossing_insertion() {
    OrderBook ob;
    ob.add_order(1, 99, 10, Side::Buy);
    ob.add_order(2, 101, 20, Side::Sell);
    
    assert(ob.size() == 2);
    assert(ob.get_volume_at_price(99, Side::Buy) == 10);
    assert(ob.get_volume_at_price(101, Side::Sell) == 20);
    
    std::cout << "test_non_crossing_insertion passed!\n";
}

/**
 * @brief Tests cancellation of non-existent order IDs.
 * 
 * Verifies that attempting to cancel an order with an invalid or unknown ID
 * behaves as a safe, silent no-op.
 */
void test_invalid_cancellations() {
    OrderBook ob;
    ob.add_order(1, 100, 10, Side::Buy);
    
    ob.cancel_order(999);
    
    assert(ob.size() == 1);
    assert(ob.get_volume_at_price(100, Side::Buy) == 10);
    
    std::cout << "test_invalid_cancellations passed!\n";
}

/**
 * @brief Main entry point to run all OrderBook unit tests.
 */
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
