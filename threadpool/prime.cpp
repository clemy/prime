#include <cmath>
#include <pthread.h>
#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <iostream>
#include <typeinfo>

using namespace std;

using ValType = unsigned int;
static constexpr ValType kStart{ 1000001 };  // start must be odd
static constexpr ValType kCount{ 20971520 };
static constexpr ValType kThreads{ 8 };
static constexpr ValType kWorkItems{ 128 };

static constexpr ValType kEnd{ kStart + kCount }; // check if it fits into ValType
static_assert((kStart & 1) == 1);
static_assert(kCount % kWorkItems == 0);
static constexpr ValType kCountPerWorkItem{ kCount / kWorkItems };
static_assert((kCountPerWorkItem & 1) == 0); // must be even

static atomic<ValType> sWorkItemNr;
static_assert(sWorkItemNr.is_always_lock_free == true);

static array<bitset<kCountPerWorkItem / 2>, kWorkItems> s_results;

struct ThreadInfo {
    pthread_t handle;
    ValType prime_count;
};

// only works for odd numbers! because testing only with odd numbers!
static bool checkPrime(ValType p) {
    ValType max{ static_cast<ValType>(sqrt(p)) };
    for (ValType i{ 3 }; i <= max; i += 2) {
        if (p % i == 0) {
            return false;
        }
    }
    return true;
}

static void *searchPrimes(void *_thread_info) {
    auto thread_info{ static_cast<ThreadInfo *>(_thread_info) };
    ValType prime_count{ 0 };
    ValType work_item_nr;
    while ((work_item_nr = sWorkItemNr.fetch_add(1, memory_order_relaxed)) < kWorkItems) {
        const ValType start = kStart + work_item_nr * kCountPerWorkItem;
        const ValType end = start + kCountPerWorkItem;
        auto &result_field{ s_results[work_item_nr] };
        
        for (ValType i{ start }; i < end; i += 2) {
            if (checkPrime(i)) {
                result_field.set((i - start) / 2);
                ++prime_count;
            }
        }
    }
    thread_info->prime_count = prime_count;
    return NULL;
}

int main() {
    cerr << "sizeof(ValType) = " << sizeof(ValType) << " (" << typeid(ValType).name() << ")\n";
    cerr << "primes:\n";
    auto begin_time{ chrono::high_resolution_clock::now() };

    array<ThreadInfo, kThreads> threads;
    sWorkItemNr.store(0, memory_order_relaxed);
    for (auto &t : threads) {
        if (pthread_create(&t.handle, NULL, searchPrimes, &t) != 0)
            throw runtime_error("Thread creation failed");
    }
    ValType prime_count{ 0 };
    for (auto &t : threads) {
        pthread_join(t.handle, NULL);
        prime_count += t.prime_count;
    }

    auto mid_time{ chrono::high_resolution_clock::now() };

    for (ValType work_item_nr{ 0 }; work_item_nr < kWorkItems; ++work_item_nr) {
        auto &result_field{ s_results[work_item_nr] };
        for (ValType j{ 0 }; j < kCountPerWorkItem / 2; ++j) {
            if (result_field.test(j)) {
                cout << to_string(kStart + work_item_nr * kCountPerWorkItem + j * 2) + "\n";
            }
        }
    }

    auto end_time{ chrono::high_resolution_clock::now() };

    chrono::duration<double> calctime{ mid_time - begin_time };
    chrono::duration<double> runtime{ end_time - begin_time };
    cerr << "Time:     " << runtime.count() << " s\n";
    cerr << "TimeCalc: " << calctime.count() << " s\n";
    cerr << "Speed:    " << kCount / runtime.count() << " numbers/s\n";
    cerr << "Speed:    " << prime_count / runtime.count() << " primes/s\n";
    cerr << "Primes:   " << prime_count << "\n";
    cerr << "Count:    " << kCount << "\n";
    cerr << "Threads:  " << kThreads << "\n";
    return 0;
}
