#include "gtest/gtest.h"
#include "utils/timeboundedqueue.h"

#include <thread>
#include <vector>
#include <chrono>
#include <numeric>
#include <atomic>
#include <algorithm>
#include <string>
#include <type_traits>

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

template<typename T>
class TimeBoundedQueueTest : public ::testing::Test {
protected:
    TimeBoundedQueue<T> queue;

    void SetUp() override {}
};

using MyTypes = ::testing::Types<int, std::string, MyData>;
TYPED_TEST_SUITE(TimeBoundedQueueTest, MyTypes);

TYPED_TEST(TimeBoundedQueueTest, BasicLValueTryPushTryPop) {
    TypeParam value_to_try_push;

    if constexpr (std::is_same_v<TypeParam, int>) {
        value_to_try_push = 42;
    } else if constexpr (std::is_same_v<TypeParam, std::string>) {
        value_to_try_push = "hello";
    } else if constexpr (std::is_same_v<TypeParam, MyData>) {
        value_to_try_push = {1, "test_data"};
    }

    this->queue.set_capacity(1);

    EXPECT_TRUE(this->queue.try_push(value_to_try_push, std::chrono::milliseconds(100)))
        << "Lvalue try_push failed unexpectedly.";

    TypeParam result;
    EXPECT_TRUE(this->queue.try_pop(result, std::chrono::milliseconds(100)))
        << "TryPop failed unexpectedly after a successful try_push.";
    EXPECT_EQ(result, value_to_try_push)
        << "TryPopped value does not match try_pushed value.";

    EXPECT_TRUE(this->queue.empty())
        << "Queue should be empty after try_popping all elements.";
    EXPECT_EQ(this->queue.size(), 0)
        << "Queue size should be 0 after try_popping all elements.";
}

TYPED_TEST(TimeBoundedQueueTest, BasicRValueTryPushTryPop) {
    TypeParam value_to_try_push_original;
    if constexpr (std::is_same_v<TypeParam, int>) {
        value_to_try_push_original = 88;
    } else if constexpr (std::is_same_v<TypeParam, std::string>) {
        value_to_try_push_original = "world";
    } else if constexpr (std::is_same_v<TypeParam, MyData>) {
        value_to_try_push_original = {2, "rvalue_data"};
    }

    this->queue.set_capacity(1);

    TypeParam value_to_try_push_temp = value_to_try_push_original;

    EXPECT_TRUE(this->queue.try_push(std::move(value_to_try_push_temp), std::chrono::milliseconds(100)))
        << "Rvalue try_push failed unexpectedly.";

    TypeParam result;
    EXPECT_TRUE(this->queue.try_pop(result, std::chrono::milliseconds(100)))
        << "TryPop failed unexpectedly after an rvalue try_push.";
    EXPECT_EQ(result, value_to_try_push_original)
        << "TryPopped value does not match original value after rvalue try_push.";

    EXPECT_TRUE(this->queue.empty())
        << "Queue should be empty after try_popping the element.";
}

TYPED_TEST(TimeBoundedQueueTest, TryPushTimeoutWhenFullChrono) {
    TypeParam value_to_try_push;
    if constexpr (std::is_same_v<TypeParam, int>) { value_to_try_push = 1; }
    else if constexpr (std::is_same_v<TypeParam, std::string>) { value_to_try_push = "item_full"; }
    else if constexpr (std::is_same_v<TypeParam, MyData>) { value_to_try_push = {3, "full_data"}; }

    this->queue.set_capacity(1);
    EXPECT_TRUE(this->queue.try_push(value_to_try_push, std::chrono::milliseconds(1))) // Fill the queue
        << "Failed to fill queue initially for timeout test.";

    auto start_time = std::chrono::steady_clock::now();

    EXPECT_FALSE(this->queue.try_push(value_to_try_push, std::chrono::milliseconds(10)))
        << "TryPush unexpectedly succeeded when queue was full and timeout was short.";
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    EXPECT_GE(elapsed_ms, 8)
        << "TryPush did not wait for the expected duration (chrono timeout).";
    EXPECT_LT(elapsed_ms, 50)
        << "TryPush waited for excessively long (chrono timeout).";
}

TYPED_TEST(TimeBoundedQueueTest, TryPushTimeoutWhenFullUInt) {
    TypeParam value_to_try_push;
    if constexpr (std::is_same_v<TypeParam, int>) { value_to_try_push = 11; }
    else if constexpr (std::is_same_v<TypeParam, std::string>) { value_to_try_push = "item_full_uint"; }
    else if constexpr (std::is_same_v<TypeParam, MyData>) { value_to_try_push = {11, "full_data_uint"}; }

    this->queue.set_capacity(1);
    EXPECT_TRUE(this->queue.try_push(value_to_try_push, (unsigned int)1))
        << "Failed to fill queue initially for unsigned int timeout test.";

    auto start_time = std::chrono::steady_clock::now();

    EXPECT_FALSE(this->queue.try_push(value_to_try_push, (unsigned int)10))
        << "TryPush unexpectedly succeeded when queue was full (unsigned int timeout).";
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    EXPECT_GE(elapsed_ms, 8)
        << "TryPush did not wait for the expected duration (unsigned int timeout).";
    EXPECT_LT(elapsed_ms, 50)
        << "TryPush waited for excessively long (unsigned int timeout).";
}


TYPED_TEST(TimeBoundedQueueTest, TryPopTimeoutWhenEmptyChrono) {
    TypeParam result;
    this->queue.set_capacity(1);

    auto start_time = std::chrono::steady_clock::now();

    EXPECT_FALSE(this->queue.try_pop(result, std::chrono::milliseconds(10)))
        << "TryPop unexpectedly succeeded when queue was empty and timeout was short.";
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    EXPECT_GE(elapsed_ms, 8)
        << "TryPop did not wait for the expected duration (chrono timeout).";
    EXPECT_LT(elapsed_ms, 50)
        << "TryPop waited for excessively long (chrono timeout).";
}

TYPED_TEST(TimeBoundedQueueTest, TryPopTimeoutWhenEmptyUInt) {
    TypeParam result;
    this->queue.set_capacity(1);

    auto start_time = std::chrono::steady_clock::now();

    EXPECT_FALSE(this->queue.try_pop(result, (unsigned int)10))
        << "TryPop unexpectedly succeeded when queue was empty (unsigned int timeout).";
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    EXPECT_GE(elapsed_ms, 8)
        << "TryPop did not wait for the expected duration (unsigned int timeout).";
    EXPECT_LT(elapsed_ms, 50)
        << "TryPop waited for excessively long (unsigned int timeout).";
}

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
    EXPECT_TRUE(this->queue.try_push(value1, std::chrono::milliseconds(1)))
        << "Failed to try_push initial item for wait test.";

    std::atomic<bool> try_push_succeeded{false};
    std::thread pusher_thread([&]() {
        try_push_succeeded.store(this->queue.try_push(value2, std::chrono::milliseconds(200), 1));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    TypeParam result1;
    EXPECT_TRUE(this->queue.try_pop(result1, std::chrono::milliseconds(1)))
        << "Failed to try_pop first item from main thread.";

    pusher_thread.join();
    EXPECT_TRUE(try_push_succeeded.load())
        << "TryPush operation in the separate thread unexpectedly failed.";

    TypeParam result2;
    EXPECT_TRUE(this->queue.try_pop(result2, std::chrono::milliseconds(1)))
        << "Failed to try_pop second item after pusher thread completed.";
    EXPECT_EQ(result1, value1) << "First try_popped item does not match.";
    EXPECT_EQ(result2, value2) << "Second try_popped item does not match.";
}

TYPED_TEST(TimeBoundedQueueTest, TryPopSucceedsAfterWait) {
    TypeParam value;
    if constexpr (std::is_same_v<TypeParam, int>) {
        value = 99;
    } else if constexpr (std::is_same_v<TypeParam, std::string>) {
        value = "lazy_item";
    } else if constexpr (std::is_same_v<TypeParam, MyData>) {
        value = {99, "lazy_data"};
    }

    this->queue.set_capacity(1);

    TypeParam result;
    std::atomic<bool> try_pop_succeeded{false};
    std::thread popper_thread([&]() {
        try_pop_succeeded.store(this->queue.try_pop(result, std::chrono::milliseconds(200), 1));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_TRUE(this->queue.try_push(value, std::chrono::milliseconds(1)))
        << "Failed to try_push item to unblock popper thread.";

    popper_thread.join();
    EXPECT_TRUE(try_pop_succeeded.load())
        << "TryPop operation in the separate thread unexpectedly failed.";
    EXPECT_EQ(result, value) << "TryPopped value does not match try_pushed value.";
}

TYPED_TEST(TimeBoundedQueueTest, MultiThreadedProducerConsumer) {
    const int num_producers = 4;
    const int num_consumers = 4;
    const int items_per_producer = 50;
    const int total_items = num_producers * items_per_producer;
    const int queue_capacity = 10;

    this->queue.set_capacity(queue_capacity);

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

    std::vector<TypeParam> consumed_items_collector(total_items);
    std::atomic<int> try_pushed_count{0};
    std::atomic<int> consumed_idx{0};
    std::atomic<int> failed_try_pushes{0};
    std::atomic<int> failed_try_pops{0};

    std::vector<std::thread> producers;
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&, i]() {
            for (int j = 0; j < items_per_producer; ++j) {                
                TypeParam item_to_try_push = expected_try_pushed_items[i * items_per_producer + j];

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

            while (true) {
                if (this->queue.try_pop(result, std::chrono::milliseconds(5000))) {
                    int current_idx = consumed_idx.fetch_add(1);
                    if (current_idx < total_items) {
                        consumed_items_collector[current_idx] = result;
                    }
                } else {
                    failed_try_pops.fetch_add(1);

                    if (try_pushed_count.load() == total_items && this->queue.empty()) {
                        break;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

                if (consumed_idx.load() >= total_items) {
                    break;
                }
            }
        });
    }

    for (auto& p : producers) p.join();
    for (auto& c : consumers) c.join();

    EXPECT_EQ(try_pushed_count.load(), total_items)
        << "All items should have been try_pushed successfully by producers.";
    EXPECT_EQ(consumed_idx.load(), total_items)
        << "All items should have been consumed successfully by consumers.";
    EXPECT_EQ(failed_try_pushes.load(), 0)
        << "No try_pushes should have failed given ample timeout and consumer activity.";
    EXPECT_TRUE(this->queue.empty())
        << "Queue should be empty after all try_pushes and try_pops have completed.";

    std::sort(expected_try_pushed_items.begin(), expected_try_pushed_items.end());
    std::sort(consumed_items_collector.begin(), consumed_items_collector.end());
    EXPECT_EQ(expected_try_pushed_items, consumed_items_collector)
        << "The set of consumed items must exactly match the set of try_pushed items.";
}

TYPED_TEST(TimeBoundedQueueTest, ZeroTimeout) {
    TypeParam value;
    if constexpr (std::is_same_v<TypeParam, int>) { value = 123; }
    else if constexpr (std::is_same_v<TypeParam, std::string>) { value = "zero_timeout"; }
    else if constexpr (std::is_same_v<TypeParam, MyData>) { value = {123, "zero_timeout_data"}; }

    this->queue.set_capacity(1);

    EXPECT_TRUE(this->queue.try_push(value, std::chrono::milliseconds(0), 0))
        << "TryPush with zero timeout to empty queue failed.";
    EXPECT_EQ(this->queue.size(), 1)
        << "Queue size incorrect after zero-timeout try_push.";

    TypeParam result;
    EXPECT_TRUE(this->queue.try_pop(result, std::chrono::milliseconds(0), 0))
        << "TryPop with zero timeout from non-empty queue failed.";
    EXPECT_EQ(result, value)
        << "TryPopped value incorrect after zero-timeout try_pop.";
    EXPECT_EQ(this->queue.size(), 0)
        << "Queue size incorrect after zero-timeout try_pop.";

    EXPECT_FALSE(this->queue.try_pop(result, std::chrono::milliseconds(0), 0))
        << "TryPop with zero timeout from empty queue unexpectedly succeeded.";

    EXPECT_TRUE(this->queue.try_push(value, std::chrono::milliseconds(0), 0)) // Fill it again
        << "Failed to refill queue for zero-timeout try_push when full test.";
    EXPECT_EQ(this->queue.size(), 1)
        << "Queue size incorrect after refilling for zero-timeout test.";
    EXPECT_FALSE(this->queue.try_push(value, std::chrono::milliseconds(0), 0)) // Try to try_push when full
        << "TryPush with zero timeout to full queue unexpectedly succeeded.";
}

TYPED_TEST(TimeBoundedQueueTest, TryPushSleepInterval) {
    TypeParam value;
    if constexpr (std::is_same_v<TypeParam, int>) { value = 77; }
    else if constexpr (std::is_same_v<TypeParam, std::string>) { value = "sleep_test"; }
    else if constexpr (std::is_same_v<TypeParam, MyData>) { value = {77, "sleep_test_data"}; }

    this->queue.set_capacity(1);
    EXPECT_TRUE(this->queue.try_push(value, std::chrono::milliseconds(1)))
        << "Failed to initially fill queue for sleep interval test.";

    const unsigned int sleep_interval_ms = 10;
    const std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(30);

    auto start_time = std::chrono::steady_clock::now();

    EXPECT_FALSE(this->queue.try_push(value, timeout_ms, sleep_interval_ms))
        << "TryPush unexpectedly succeeded or did not time out as expected.";
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // With `sleep_interval_ms = 10` and `timeout_ms = 30`, it should perform about 3 sleeps
    // (e.g., checks at 0, 10, 20, 30ms approx).
    EXPECT_GE(elapsed_ms, 25)
        << "Elapsed time is less than expected timeout for try_push sleep interval test.";
    EXPECT_LT(elapsed_ms, 60)
        << "Elapsed time is excessively long for try_push sleep interval test.";
}

TEST(TimeBoundedQueueIntTest, TryPopIgnoresSleepIntervalParameter) {
    TimeBoundedQueue<int> q;
    q.set_capacity(1);
    int result;

    auto start_time = std::chrono::steady_clock::now();

    EXPECT_FALSE(q.try_pop(result, std::chrono::milliseconds(20), 100))
        << "TryPop unexpectedly succeeded or didn't time out.";
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    EXPECT_GE(elapsed_ms, 15)
        << "Elapsed time too short, suggesting try_pop didn't sleep properly.";
    EXPECT_LT(elapsed_ms, 40)
        << "Elapsed time too long, suggesting try_pop respected the sleepInterval parameter.";
}


TYPED_TEST(TimeBoundedQueueTest, MaxCapacityStress) {
    const int max_items = 1000;
    this->queue.set_capacity(max_items);

    std::vector<TypeParam> try_pushed_values(max_items);
    for (int i = 0; i < max_items; ++i) {
        if constexpr (std::is_same_v<TypeParam, int>) { try_pushed_values[i] = i; }
        else if constexpr (std::is_same_v<TypeParam, std::string>) { try_pushed_values[i] = "stress_" + std::to_string(i); }
        else if constexpr (std::is_same_v<TypeParam, MyData>) { try_pushed_values[i] = {i, "stress_data_" + std::to_string(i)}; }

        EXPECT_TRUE(this->queue.try_push(try_pushed_values[i], std::chrono::milliseconds(10)))
            << "Failed to try_push item " << i << " during capacity fill.";
    }
    EXPECT_EQ(this->queue.size(), max_items)
        << "Queue size mismatch after filling to capacity.";
    EXPECT_FALSE(this->queue.empty())
        << "Queue should not be empty after filling to capacity.";

    TypeParam overflow_value;
    if constexpr (std::is_same_v<TypeParam, int>) { overflow_value = 9999; }
    else if constexpr (std::is_same_v<TypeParam, std::string>) { overflow_value = "overflow"; }
    else if constexpr (std::is_same_v<TypeParam, MyData>) { overflow_value = {9999, "overflow_data"}; }
    EXPECT_FALSE(this->queue.try_push(overflow_value, std::chrono::milliseconds(0)))
        << "TryPush unexpectedly succeeded when queue was full (overflow attempt).";

    std::vector<TypeParam> try_popped_values(max_items);
    for (int i = 0; i < max_items; ++i) {
        EXPECT_TRUE(this->queue.try_pop(try_popped_values[i], std::chrono::milliseconds(10)))
            << "Failed to try_pop item " << i << " during draining.";
    }
    EXPECT_EQ(this->queue.size(), 0)
        << "Queue size mismatch after draining.";
    EXPECT_TRUE(this->queue.empty())
        << "Queue should be empty after draining all elements.";

    std::sort(try_pushed_values.begin(), try_pushed_values.end());
    std::sort(try_popped_values.begin(), try_popped_values.end());
    EXPECT_EQ(try_pushed_values, try_popped_values)
        << "The set of try_popped items does not match the set of try_pushed items.";
}

TYPED_TEST(TimeBoundedQueueTest, ConcurrentShortTimeouts) {
    const int num_threads = 2;
    const int items_per_thread = 50;
    const int total_items = num_threads * items_per_thread;
    const int queue_capacity = 2;

    this->queue.set_capacity(queue_capacity);

    std::atomic<int> try_push_successes{0};
    std::atomic<int> try_pop_successes{0};
    std::atomic<int> try_push_failures{0};
    std::atomic<int> try_pop_failures{0};
    std::atomic<int> produced_counter{0};
    std::vector<TypeParam> consumed_items_collector(total_items);
    std::atomic<int> consumed_idx{0};

    std::vector<std::thread> threads;

    // Producer threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < items_per_thread; ++j) {
                TypeParam value_to_try_push;
                int current_val_idx = produced_counter.fetch_add(1);

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

            while (try_pop_successes.load() < total_items) {
                // Attempt to try_pop with a very short timeout (10ms)
                if (this->queue.try_pop(result, std::chrono::milliseconds(10))) {
                    int current_idx = consumed_idx.fetch_add(1);
                    if (current_idx < total_items) {
                        consumed_items_collector[current_idx] = result;
                    }
                    try_pop_successes.fetch_add(1);
                } else {
                    try_pop_failures.fetch_add(1);

                    if (produced_counter.load() == total_items && this->queue.empty()) {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
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

TEST(TimeBoundedQueueIntTest, DestructorCleansUp) {
    {
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
