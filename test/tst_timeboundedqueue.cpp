#include "gtest/gtest.h"
#include "utils/timeboundedqueue.h"

#include <thread>
#include <vector>
#include <chrono>
#include <numeric>     // For std::iota
#include <atomic>      // For std::atomic
#include <algorithm>   // For std::sort
#include <string>      // For std::string
#include <type_traits> // For std::is_same_v

// Define a simple struct for testing custom types
struct MyData {
    int id;
    std::string name;

    bool operator==(const MyData& other) const {
        return id == other.id && name == other.name;
    }

    bool operator<(const MyData& other) const {
        if (id != other.id) {
            return id < other.id;
        }
        return name < other.name;
    }

    MyData() : id(0), name("") {}
    MyData(int i, const std::string& n) : id(i), name(n) {}

    friend std::ostream& operator<<(std::ostream& os, const MyData& data) {
        return os << "MyData{id: " << data.id << ", name: \"" << data.name << "\"}";
    }
};

// Test fixture for TimeBoundedQueue.
// This allows us to use type-parameterized tests to run the same test logic
// with different data types (int, std::string, MyData).
template<typename T>
class TimeBoundedQueueTest : public ::testing::Test {
protected:
    TimeBoundedQueue<T> queue; // Each test will get a fresh, empty queue object

    void SetUp() override {
        // This method is called before each test.
        // The queue is already default-constructed and empty here.
        // Specific capacity settings are done within individual test cases as needed.
    }
};

// Define the types to be used for the type-parameterized tests.
// This allows us to run the same test suite for int, std::string, and MyData.
using MyTypes = ::testing::Types<int, std::string, MyData>;
TYPED_TEST_SUITE(TimeBoundedQueueTest, MyTypes);


// Test Case 1: Basic try_push (lvalue reference) and try_pop operations
// Verifies that a value can be successfully try_pushed and then try_popped,
// and that the queue state (empty, size) updates correctly.
TYPED_TEST(TimeBoundedQueueTest, BasicLValueTryPushTryPop) {
    TypeParam value_to_try_push;
    // Initialize value_to_try_push based on its type
    if constexpr (std::is_same_v<TypeParam, int>) {
        value_to_try_push = 42;
    } else if constexpr (std::is_same_v<TypeParam, std::string>) {
        value_to_try_push = "hello";
    } else if constexpr (std::is_same_v<TypeParam, MyData>) {
        value_to_try_push = {1, "test_data"};
    }

    this->queue.set_capacity(1); // Set a small capacity for easy testing

    // TryPush the value using the lvalue try_push overload (const T&)
    EXPECT_TRUE(this->queue.try_push(value_to_try_push, std::chrono::milliseconds(100)))
        << "Lvalue try_push failed unexpectedly.";

    // TryPop the value
    TypeParam result;
    EXPECT_TRUE(this->queue.try_pop(result, std::chrono::milliseconds(100)))
        << "TryPop failed unexpectedly after a successful try_push.";
    EXPECT_EQ(result, value_to_try_push)
        << "TryPopped value does not match try_pushed value.";

    // Queue should now be empty and have a size of 0
    EXPECT_TRUE(this->queue.empty())
        << "Queue should be empty after try_popping all elements.";
    EXPECT_EQ(this->queue.size(), 0)
        << "Queue size should be 0 after try_popping all elements.";
}

// Test Case 2: Basic try_push (rvalue reference) and try_pop operations
// Verifies that a value can be successfully try_pushed using move semantics
// and then try_popped, maintaining correctness of the value.
TYPED_TEST(TimeBoundedQueueTest, BasicRValueTryPushTryPop) {
    TypeParam value_to_try_push_original; // To hold the original value for verification
    if constexpr (std::is_same_v<TypeParam, int>) {
        value_to_try_push_original = 88;
    } else if constexpr (std::is_same_v<TypeParam, std::string>) {
        value_to_try_push_original = "world";
    } else if constexpr (std::is_same_v<TypeParam, MyData>) {
        value_to_try_push_original = {2, "rvalue_data"};
    }

    this->queue.set_capacity(1);

    TypeParam value_to_try_push_temp = value_to_try_push_original; // Create a temporary copy to move from
    // TryPush the value using the rvalue try_push overload (T&&)
    EXPECT_TRUE(this->queue.try_push(std::move(value_to_try_push_temp), std::chrono::milliseconds(100)))
        << "Rvalue try_push failed unexpectedly.";

    TypeParam result;
    EXPECT_TRUE(this->queue.try_pop(result, std::chrono::milliseconds(100)))
        << "TryPop failed unexpectedly after an rvalue try_push.";
    EXPECT_EQ(result, value_to_try_push_original) // Check against the original value
        << "TryPopped value does not match original value after rvalue try_push.";

    EXPECT_TRUE(this->queue.empty())
        << "Queue should be empty after try_popping the element.";
}


// Test Case 3: TryPush timeout when the queue is full (using std::chrono::milliseconds)
// Verifies that try_push returns false and waits for approximately the specified timeout
// when the queue has reached its capacity.
TYPED_TEST(TimeBoundedQueueTest, TryPushTimeoutWhenFullChrono) {
    TypeParam value_to_try_push;
    if constexpr (std::is_same_v<TypeParam, int>) { value_to_try_push = 1; }
    else if constexpr (std::is_same_v<TypeParam, std::string>) { value_to_try_push = "item_full"; }
    else if constexpr (std::is_same_v<TypeParam, MyData>) { value_to_try_push = {3, "full_data"}; }

    this->queue.set_capacity(1);
    EXPECT_TRUE(this->queue.try_push(value_to_try_push, std::chrono::milliseconds(1))) // Fill the queue
        << "Failed to fill queue initially for timeout test.";

    // Try to try_push another item with a very short timeout
    auto start_time = std::chrono::steady_clock::now();
    // Use the default sleepInterval (5ms) for try_push here
    EXPECT_FALSE(this->queue.try_push(value_to_try_push, std::chrono::milliseconds(10)))
        << "TryPush unexpectedly succeeded when queue was full and timeout was short.";
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // The elapsed time should be close to the timeout, indicating that it waited.
    // Allow some tolerance for system scheduling and the sleep interval (5ms).
    // It should wait at least one sleep interval.
    EXPECT_GE(elapsed_ms, 8)
        << "TryPush did not wait for the expected duration (chrono timeout).";
    EXPECT_LT(elapsed_ms, 50)
        << "TryPush waited for excessively long (chrono timeout).";
}

// Test Case 4: TryPush timeout when the queue is full (using unsigned int for timeout)
// Verifies the unsigned int overload behaves correctly, returning false and waiting.
TYPED_TEST(TimeBoundedQueueTest, TryPushTimeoutWhenFullUInt) {
    TypeParam value_to_try_push;
    if constexpr (std::is_same_v<TypeParam, int>) { value_to_try_push = 11; }
    else if constexpr (std::is_same_v<TypeParam, std::string>) { value_to_try_push = "item_full_uint"; }
    else if constexpr (std::is_same_v<TypeParam, MyData>) { value_to_try_push = {11, "full_data_uint"}; }

    this->queue.set_capacity(1);
    EXPECT_TRUE(this->queue.try_push(value_to_try_push, (unsigned int)1)) // Fill the queue
        << "Failed to fill queue initially for unsigned int timeout test.";

    auto start_time = std::chrono::steady_clock::now();
    // Use the default sleepInterval (5ms) for try_push here
    EXPECT_FALSE(this->queue.try_push(value_to_try_push, (unsigned int)10))
        << "TryPush unexpectedly succeeded when queue was full (unsigned int timeout).";
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    EXPECT_GE(elapsed_ms, 8)
        << "TryPush did not wait for the expected duration (unsigned int timeout).";
    EXPECT_LT(elapsed_ms, 50)
        << "TryPush waited for excessively long (unsigned int timeout).";
}


// Test Case 5: TryPop timeout when the queue is empty (using std::chrono::milliseconds)
// Verifies that try_pop returns false and waits for approximately the specified timeout
// when the queue is empty.
TYPED_TEST(TimeBoundedQueueTest, TryPopTimeoutWhenEmptyChrono) {
    TypeParam result;
    this->queue.set_capacity(1); // Ensure it's empty initially

    // Try to try_pop from an empty queue with a short timeout
    auto start_time = std::chrono::steady_clock::now();
    // The try_pop implementation uses a hardcoded 5ms sleep, ignoring the sleepInterval parameter.
    EXPECT_FALSE(this->queue.try_pop(result, std::chrono::milliseconds(10)))
        << "TryPop unexpectedly succeeded when queue was empty and timeout was short.";
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    EXPECT_GE(elapsed_ms, 8)
        << "TryPop did not wait for the expected duration (chrono timeout).";
    EXPECT_LT(elapsed_ms, 50)
        << "TryPop waited for excessively long (chrono timeout).";
}

// Test Case 6: TryPop timeout when the queue is empty (using unsigned int for timeout)
// Verifies the unsigned int overload for try_pop behaves correctly.
TYPED_TEST(TimeBoundedQueueTest, TryPopTimeoutWhenEmptyUInt) {
    TypeParam result;
    this->queue.set_capacity(1); // Ensure it's empty initially

    auto start_time = std::chrono::steady_clock::now();
    // The try_pop implementation uses a hardcoded 5ms sleep, ignoring the sleepInterval parameter.
    EXPECT_FALSE(this->queue.try_pop(result, (unsigned int)10))
        << "TryPop unexpectedly succeeded when queue was empty (unsigned int timeout).";
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    EXPECT_GE(elapsed_ms, 8)
        << "TryPop did not wait for the expected duration (unsigned int timeout).";
    EXPECT_LT(elapsed_ms, 50)
        << "TryPop waited for excessively long (unsigned int timeout).";
}


// Test Case 7: TryPush succeeds after some wait (another thread try_pops)
// Simulates a producer waiting for a consumer to free up space.
TYPED_TEST(TimeBoundedQueueTest, TryPushSucceedsAfterWait) {
    TypeParam value1, value2;
    if constexpr (std::is_same_v<TypeParam, int>) {
        value1 = 10; value2 = 20;
    } else if constexpr (std::is_same_v<TypeParam, std::string>) {
        value1 = "first"; value2 = "second";
    } else if constexpr (std::is_same_v<TypeParam, MyData>) {
        value1 = {10, "first_data"}; value2 = {20, "second_data"};
    }

    this->queue.set_capacity(1);
    EXPECT_TRUE(this->queue.try_push(value1, std::chrono::milliseconds(1))) // Fill the queue
        << "Failed to try_push initial item for wait test.";

    std::atomic<bool> try_push_succeeded{false};
    std::thread pusher_thread([&]() {
        // This try_push will block until a spot is available.
        // It uses a 200ms timeout with a 1ms sleep interval.
        try_push_succeeded.store(this->queue.try_push(value2, std::chrono::milliseconds(200), 1));
    });

    // Wait a bit to ensure the pusher thread has started and is blocked.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    TypeParam result1;
    EXPECT_TRUE(this->queue.try_pop(result1, std::chrono::milliseconds(1))) // TryPop value1 from main thread
        << "Failed to try_pop first item from main thread.";

    pusher_thread.join(); // Wait for the pusher thread to complete
    EXPECT_TRUE(try_push_succeeded.load()) // Verify try_push in thread succeeded
        << "TryPush operation in the separate thread unexpectedly failed.";

    TypeParam result2;
    EXPECT_TRUE(this->queue.try_pop(result2, std::chrono::milliseconds(1))) // TryPop value2
        << "Failed to try_pop second item after pusher thread completed.";
    EXPECT_EQ(result1, value1) << "First try_popped item does not match.";
    EXPECT_EQ(result2, value2) << "Second try_popped item does not match.";
}

// Test Case 8: TryPop succeeds after some wait (another thread try_pushes)
// Simulates a consumer waiting for a producer to add items.
TYPED_TEST(TimeBoundedQueueTest, TryPopSucceedsAfterWait) {
    TypeParam value;
    if constexpr (std::is_same_v<TypeParam, int>) {
        value = 99;
    } else if constexpr (std::is_same_v<TypeParam, std::string>) {
        value = "lazy_item";
    } else if constexpr (std::is_same_v<TypeParam, MyData>) {
        value = {99, "lazy_data"};
    }

    this->queue.set_capacity(1); // Start empty

    TypeParam result;
    std::atomic<bool> try_pop_succeeded{false};
    std::thread popper_thread([&]() {
        // This try_pop will block until an item is available.
        // It uses a 200ms timeout. Its sleep interval is hardcoded to 5ms internally.
        try_pop_succeeded.store(this->queue.try_pop(result, std::chrono::milliseconds(200), 1));
    });

    // Wait a bit to ensure the popper thread has started and is blocked.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(this->queue.try_push(value, std::chrono::milliseconds(1))) // TryPush value from main thread
        << "Failed to try_push item to unblock popper thread.";

    popper_thread.join(); // Wait for the popper thread to complete
    EXPECT_TRUE(try_pop_succeeded.load()) // Verify try_pop in thread succeeded
        << "TryPop operation in the separate thread unexpectedly failed.";
    EXPECT_EQ(result, value) << "TryPopped value does not match try_pushed value.";
}

// Test Case 9: Concurrency - Multiple producers and consumers
// A more robust test involving several threads concurrently try_pushing and try_popping
// to stress the thread-safety and timeout mechanisms.
TYPED_TEST(TimeBoundedQueueTest, MultiThreadedProducerConsumer) {
    const int num_producers = 4;
    const int num_consumers = 4;
    const int items_per_producer = 50;
    const int total_items = num_producers * items_per_producer;
    const int queue_capacity = 10; // Moderate capacity to allow some buffering and contention

    this->queue.set_capacity(queue_capacity);

    // Prepare all items to be try_pushed in advance
    std::vector<TypeParam> expected_try_pushed_items(total_items);
    if constexpr (std::is_same_v<TypeParam, int>) {
        std::iota(expected_try_pushed_items.begin(), expected_try_pushed_items.end(), 0);
    } else if constexpr (std::is_same_v<TypeParam, std::string>) {
        for (int i = 0; i < total_items; ++i) {
            expected_try_pushed_items[i] = "item_" + std::to_string(i);
        }
    } else if constexpr (std::is_same_v<TypeParam, MyData>) {
        for (int i = 0; i < total_items; ++i) {
            expected_try_pushed_items[i] = {i, "data_" + std::to_string(i)};
        }
    }

    // Atomic counters for tracking operations and collected items
    std::vector<TypeParam> consumed_items_collector(total_items); // To collect all consumed items
    std::atomic<int> try_pushed_count{0};
    std::atomic<int> consumed_idx{0}; // Atomic index for storing into consumed_items_collector
    std::atomic<int> failed_try_pushes{0};
    std::atomic<int> failed_try_pops{0};

    std::vector<std::thread> producers;
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&, i]() {
            for (int j = 0; j < items_per_producer; ++j) {
                // Get item from our pre-generated list using a unique global index
                TypeParam item_to_try_push = expected_try_pushed_items[i * items_per_producer + j];
                // TryPush with a generous 1-second timeout and 1ms sleep interval
                if (this->queue.try_push(item_to_try_push, std::chrono::milliseconds(1000), 1)) {
                    try_pushed_count.fetch_add(1);
                } else {
                    failed_try_pushes.fetch_add(1);
                }
            }
        });
    }

    std::vector<std::thread> consumers;
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&]() {
            TypeParam result;
            // Consumer keeps trying to try_pop. Use a very long timeout to ensure it eventually gets items.
            while (true) {
                // TryPop with a 5-second timeout (its internal sleep is hardcoded to 5ms)
                if (this->queue.try_pop(result, std::chrono::milliseconds(5000))) {
                    int current_idx = consumed_idx.fetch_add(1);
                    if (current_idx < total_items) { // Ensure we don't write out of bounds
                        consumed_items_collector[current_idx] = result;
                    }
                } else {
                    failed_try_pops.fetch_add(1);
                    // If try_pop failed with a timeout, and all producers are done AND the queue is empty,
                    // it means there are no more items to expect. Break the loop.
                    if (try_pushed_count.load() == total_items && this->queue.empty()) {
                        break;
                    }
                    // Sleep briefly to avoid busy-waiting if queue is temporarily empty
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                // Break if we've successfully consumed all expected items
                if (consumed_idx.load() >= total_items) {
                    break;
                }
            }
        });
    }

    // Wait for all producer threads to complete their work
    for (auto& p : producers) p.join();

    // Wait for all consumer threads to complete their work.
    // Consumers might take longer if the queue is slow or becomes empty intermittently.
    for (auto& c : consumers) c.join();

    // Assertions for overall correctness
    EXPECT_EQ(try_pushed_count.load(), total_items)
        << "All items should have been try_pushed successfully by producers.";
    EXPECT_EQ(consumed_idx.load(), total_items)
        << "All items should have been consumed successfully by consumers.";
    EXPECT_EQ(failed_try_pushes.load(), 0)
        << "No try_pushes should have failed given ample timeout and consumer activity.";
    // Failed try_pops might legitimately occur if a consumer times out right when the queue
    // becomes empty for good, but `consumed_idx` should still reach `total_items`.
    // So, `failed_try_pops` could be > 0 here, but we prioritize `consumed_idx == total_items`.
    EXPECT_TRUE(this->queue.empty())
        << "Queue should be empty after all try_pushes and try_pops have completed.";

    // Verify that the set of consumed items is exactly the same as the set of try_pushed items.
    // Sorting is necessary because `tbb::concurrent_bounded_queue` does not guarantee order
    // for concurrent try_pushes/try_pops when capacity is > 1.
    std::sort(expected_try_pushed_items.begin(), expected_try_pushed_items.end());
    std::sort(consumed_items_collector.begin(), consumed_items_collector.end());
    EXPECT_EQ(expected_try_pushed_items, consumed_items_collector)
        << "The set of consumed items must exactly match the set of try_pushed items.";
}

// Test Case 10: Zero timeout for try_push/try_pop
// Verifies that operations with a zero timeout behave like non-blocking tbb::concurrent_bounded_queue<T>::try_push/tbb::concurrent_bounded_queue<T>::try_pop.
TYPED_TEST(TimeBoundedQueueTest, ZeroTimeout) {
    TypeParam value;
    if constexpr (std::is_same_v<TypeParam, int>) { value = 123; }
    else if constexpr (std::is_same_v<TypeParam, std::string>) { value = "zero_timeout"; }
    else if constexpr (std::is_same_v<TypeParam, MyData>) { value = {123, "zero_timeout_data"}; }

    this->queue.set_capacity(1);

    // TryPush with zero timeout when empty: should succeed immediately
    EXPECT_TRUE(this->queue.try_push(value, std::chrono::milliseconds(0), 0))
        << "TryPush with zero timeout to empty queue failed.";
    EXPECT_EQ(this->queue.size(), 1)
        << "Queue size incorrect after zero-timeout try_push.";

    // TryPop with zero timeout when item is present: should succeed immediately
    TypeParam result;
    EXPECT_TRUE(this->queue.try_pop(result, std::chrono::milliseconds(0), 0))
        << "TryPop with zero timeout from non-empty queue failed.";
    EXPECT_EQ(result, value)
        << "TryPopped value incorrect after zero-timeout try_pop.";
    EXPECT_EQ(this->queue.size(), 0)
        << "Queue size incorrect after zero-timeout try_pop.";

    // TryPop with zero timeout when empty: should fail immediately
    EXPECT_FALSE(this->queue.try_pop(result, std::chrono::milliseconds(0), 0))
        << "TryPop with zero timeout from empty queue unexpectedly succeeded.";

    // TryPush with zero timeout when full: should fail immediately
    EXPECT_TRUE(this->queue.try_push(value, std::chrono::milliseconds(0), 0)) // Fill it again
        << "Failed to refill queue for zero-timeout try_push when full test.";
    EXPECT_EQ(this->queue.size(), 1)
        << "Queue size incorrect after refilling for zero-timeout test.";
    EXPECT_FALSE(this->queue.try_push(value, std::chrono::milliseconds(0), 0)) // Try to try_push when full
        << "TryPush with zero timeout to full queue unexpectedly succeeded.";
}

// Test Case 11: TryPush sleep interval check
// Verifies that the `try_push` method respects the `sleepInterval` parameter,
// ensuring it waits in appropriate increments.
TYPED_TEST(TimeBoundedQueueTest, TryPushSleepInterval) {
    TypeParam value;
    if constexpr (std::is_same_v<TypeParam, int>) { value = 77; }
    else if constexpr (std::is_same_v<TypeParam, std::string>) { value = "sleep_test"; }
    else if constexpr (std::is_same_v<TypeParam, MyData>) { value = {77, "sleep_test_data"}; }

    this->queue.set_capacity(1);
    EXPECT_TRUE(this->queue.try_push(value, std::chrono::milliseconds(1))) // Fill queue
        << "Failed to initially fill queue for sleep interval test.";

    const unsigned int sleep_interval_ms = 10; // Use a distinct sleep interval
    const std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(30);

    auto start_time = std::chrono::steady_clock::now();
    // Attempt to try_push with a specified timeout and sleep interval
    EXPECT_FALSE(this->queue.try_push(value, timeout_ms, sleep_interval_ms))
        << "TryPush unexpectedly succeeded or did not time out as expected.";
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // Expecting to wait roughly `timeout_ms`.
    // With `sleep_interval_ms = 10` and `timeout_ms = 30`, it should perform about 3 sleeps
    // (e.g., checks at 0, 10, 20, 30ms approx).
    EXPECT_GE(elapsed_ms, 25) // Should be close to timeout (allowing for initial check)
        << "Elapsed time is less than expected timeout for try_push sleep interval test.";
    EXPECT_LT(elapsed_ms, 60) // Should not be excessively long (e.g., stuck)
        << "Elapsed time is excessively long for try_push sleep interval test.";
}

// Test Case 12: TryPop ignores sleepInterval parameter (critical observation)
// This test specifically verifies the behavior identified: the `try_pop` method's
// implementation hardcodes a 5ms sleep, disregarding the `sleepInterval` parameter.
TEST(TimeBoundedQueueIntTest, TryPopIgnoresSleepIntervalParameter) {
    TimeBoundedQueue<int> q; // Use a direct instance for non-parameterized test
    q.set_capacity(1);
    int result;

    // Try to try_pop from an empty queue with a specified sleep interval (e.g., 100ms)
    // that we expect to be ignored by the implementation.
    auto start_time = std::chrono::steady_clock::now();
    // Timeout is 20ms. If it respects 100ms sleep, it would time out very quickly or take >100ms.
    // If it uses hardcoded 5ms, it should take ~20ms (4 sleeps of 5ms).
    EXPECT_FALSE(q.try_pop(result, std::chrono::milliseconds(20), 100))
        << "TryPop unexpectedly succeeded or didn't time out.";
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // Assert that the elapsed time is consistent with the hardcoded 5ms sleep,
    // not the 100ms passed as a parameter.
    EXPECT_GE(elapsed_ms, 15) // Should be roughly 4*5ms = 20ms
        << "Elapsed time too short, suggesting try_pop didn't sleep properly.";
    EXPECT_LT(elapsed_ms, 40) // Should definitely be less than 100ms + buffer
        << "Elapsed time too long, suggesting try_pop respected the sleepInterval parameter.";
}


// Test Case 13: Stress test with max capacity and overflow attempts
// TryPushes a large number of items up to capacity, attempts to overflow, then try_pops all.
TYPED_TEST(TimeBoundedQueueTest, MaxCapacityStress) {
    const int max_items = 1000; // A reasonably large number of items
    this->queue.set_capacity(max_items);

    // Prepare values to try_push
    std::vector<TypeParam> try_pushed_values(max_items);
    for (int i = 0; i < max_items; ++i) {
        if constexpr (std::is_same_v<TypeParam, int>) { try_pushed_values[i] = i; }
        else if constexpr (std::is_same_v<TypeParam, std::string>) { try_pushed_values[i] = "stress_" + std::to_string(i); }
        else if constexpr (std::is_same_v<TypeParam, MyData>) { try_pushed_values[i] = {i, "stress_data_" + std::to_string(i)}; }

        // TryPush with a small timeout, expecting immediate success as queue is not full yet
        EXPECT_TRUE(this->queue.try_push(try_pushed_values[i], std::chrono::milliseconds(10)))
            << "Failed to try_push item " << i << " during capacity fill.";
    }
    EXPECT_EQ(this->queue.size(), max_items)
        << "Queue size mismatch after filling to capacity.";
    EXPECT_FALSE(this->queue.empty())
        << "Queue should not be empty after filling to capacity.";

    // Attempt to try_push one more item: should fail immediately with zero timeout
    TypeParam overflow_value;
    if constexpr (std::is_same_v<TypeParam, int>) { overflow_value = 9999; }
    else if constexpr (std::is_same_v<TypeParam, std::string>) { overflow_value = "overflow"; }
    else if constexpr (std::is_same_v<TypeParam, MyData>) { overflow_value = {9999, "overflow_data"}; }
    EXPECT_FALSE(this->queue.try_push(overflow_value, std::chrono::milliseconds(0)))
        << "TryPush unexpectedly succeeded when queue was full (overflow attempt).";

    // TryPop all items from the queue
    std::vector<TypeParam> try_popped_values(max_items);
    for (int i = 0; i < max_items; ++i) {
        // TryPop with a small timeout, expecting immediate success as items are present
        EXPECT_TRUE(this->queue.try_pop(try_popped_values[i], std::chrono::milliseconds(10)))
            << "Failed to try_pop item " << i << " during draining.";
    }
    EXPECT_EQ(this->queue.size(), 0)
        << "Queue size mismatch after draining.";
    EXPECT_TRUE(this->queue.empty())
        << "Queue should be empty after draining all elements.";

    // Verify content: sort both vectors and compare to account for non-guaranteed order
    std::sort(try_pushed_values.begin(), try_pushed_values.end());
    std::sort(try_popped_values.begin(), try_popped_values.end());
    EXPECT_EQ(try_pushed_values, try_popped_values)
        << "The set of try_popped items does not match the set of try_pushed items.";
}

// Test Case 14: Concurrent try_push/try_pop with very short timeouts
// This test aims to stress the queue under high contention with very short timeouts,
// where operations are likely to fail due to timing. It primarily checks for robustness
// (no crashes/deadlocks) and that successful operations produce correct results.
TYPED_TEST(TimeBoundedQueueTest, ConcurrentShortTimeouts) {
    const int num_threads = 2; // Keep low for focused contention
    const int items_per_thread = 50; // Total 100 items
    const int total_items = num_threads * items_per_thread;
    const int queue_capacity = 2; // Very small capacity to maximize contention

    this->queue.set_capacity(queue_capacity);

    std::atomic<int> try_push_successes{0};
    std::atomic<int> try_pop_successes{0};
    std::atomic<int> try_push_failures{0};
    std::atomic<int> try_pop_failures{0};
    std::atomic<int> produced_counter{0}; // To generate unique values for try_pushing
    std::vector<TypeParam> consumed_items_collector(total_items); // Collect all successfully try_popped items
    std::atomic<int> consumed_idx{0};

    std::vector<std::thread> threads;

    // Producer threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < items_per_thread; ++j) {
                TypeParam value_to_try_push;
                int current_val_idx = produced_counter.fetch_add(1); // Get a unique index
                // Initialize value based on type and unique index
                if constexpr (std::is_same_v<TypeParam, int>) { value_to_try_push = current_val_idx; }
                else if constexpr (std::is_same_v<TypeParam, std::string>) { value_to_try_push = "val_" + std::to_string(current_val_idx); }
                else if constexpr (std::is_same_v<TypeParam, MyData>) { value_to_try_push = {current_val_idx, "val_data_" + std::to_string(current_val_idx)}; }

                // Attempt to try_push with a very short timeout (10ms) and minimal sleep (1ms)
                if (this->queue.try_push(value_to_try_push, std::chrono::milliseconds(10), 1)) {
                    try_push_successes.fetch_add(1);
                } else {
                    try_push_failures.fetch_add(1);
                }
            }
        });
    }

    // Consumer threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            TypeParam result;
            // Consumer keeps trying until `try_pop_successes` reaches `total_items`
            while (try_pop_successes.load() < total_items) {
                // Attempt to try_pop with a very short timeout (10ms)
                if (this->queue.try_pop(result, std::chrono::milliseconds(10))) {
                    int current_idx = consumed_idx.fetch_add(1);
                    if (current_idx < total_items) {
                        consumed_items_collector[current_idx] = result; // Store successfully try_popped items
                    }
                    try_pop_successes.fetch_add(1);
                } else {
                    try_pop_failures.fetch_add(1);
                    // If producers are done and queue is empty, no more items will come.
                    // This allows consumers to exit cleanly if there's nothing left.
                    if (produced_counter.load() == total_items && this->queue.empty()) {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Briefly yield
                }
            }
        });
    }

    // Join all threads to ensure they complete
    for (auto& t : threads) t.join();

    // With short timeouts and high contention, it's expected that some try_pushes/try_pops might fail.
    // The primary goal of this test is robustness and ensuring the queue doesn't crash or deadlock.
    // We verify that the total number of operations (successes + failures) equals the intended
    // number of try_pushes, and that the number of successful try_pops is consistent.
    EXPECT_EQ(try_push_successes.load() + try_push_failures.load(), total_items)
        << "Total try_pushes (succeeded + failed) should match intended try_pushes.";
    EXPECT_LE(try_pop_successes.load(), total_items) // Number of try_pops shouldn't exceed total items
        << "Number of successful try_pops exceeds total try_pushed items.";

    EXPECT_TRUE(this->queue.empty())
        << "Queue should be empty or contain only a few residual items after stress test.";

    // Further verification: although order is not guaranteed, the set of *successfully*
    // try_pushed items should conceptually match the set of *successfully* try_popped items.
    // This part is complex to verify precisely with failed try_pushes, so we rely on the
    // fact that the queue becomes empty and no crashes occur.
    SUCCEED(); // If it reached here, the test passed its primary objective (robustness).
}

// Test Case 15: Destructor check
// This is a basic check to ensure that the destructor runs without crashing
// when the queue contains items. The underlying TBB queue is responsible for cleanup.
TEST(TimeBoundedQueueIntTest, DestructorCleansUp) {
    { // Scope to ensure the queue object is destructed
        TimeBoundedQueue<int> q;
        q.set_capacity(5);
        q.try_push(10, std::chrono::milliseconds(1));
        q.try_push(20, std::chrono::milliseconds(1));
        // Items 10, 20 are in the queue.
        // When `q` goes out of scope, its destructor (and TBB's) will be called.
        // We just ensure no crash or memory leak (by valgrind/asan if run separately).
    } // `q` is destructed here
    SUCCEED(); // If the program reaches here, it means no immediate crash occurred.
}
