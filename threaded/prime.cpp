#include <cmath>
#include <pthread.h>
#include <array>
#include <chrono>
#include <iostream>

using namespace std;

constexpr int kStart{ 1000000 };
constexpr int kCount{ 20971520 };
constexpr int kThreads{ 8 };

static_assert(kCount % kThreads == 0);

struct ThreadInfo {
    pthread_t handle;
    int start_value;
    int prime_count;
};

// only works for odd numbers! because testing only with odd numbers!
bool checkPrime(int p) {    
    int max{ static_cast<int>(sqrt(p)) };    
    for (int i{ 3 }; i <= max; i += 2) {
        if (p % i == 0) {
            return false;
        }
    }
    return true;
}

void *searchPrimes(void *_thread_info) {
    auto thread_info{ static_cast<ThreadInfo *>(_thread_info) };
    const int start = (thread_info->start_value | 1); // start must be odd => set bit 0
    constexpr int count = kCount / kThreads;
    const int end = start + count;
    int prime_count{ 0 };

    for (int i{ start }; i < end; i += 2) {
        if (checkPrime(i)) {
            cout << to_string(i) + "\n"; // be aware of race conditions: combine string first
            ++prime_count;
        }
    }
    thread_info->prime_count = prime_count;
    return NULL;
}

int main() {
    cerr << "primes:\n";
    auto begin_time{ chrono::high_resolution_clock::now() };


    array<ThreadInfo, kThreads> threads;
    auto start{ kStart };
    for (auto &t : threads) {
        t.start_value = start;
        start += kCount / kThreads;
        if (pthread_create(&t.handle, NULL, searchPrimes, &t) != 0)
            throw runtime_error("Thread creation failed");
    }
    auto prime_count{ 0 };
    for (auto &t : threads) {
        pthread_join(t.handle, NULL);
        prime_count += t.prime_count;
    }

    auto end_time{ chrono::high_resolution_clock::now() };
    chrono::duration<double> runtime{ end_time - begin_time };
    cerr << "Time:    " << runtime.count() << " s\n";
    cerr << "Speed:   " << kCount / runtime.count() << " numbers/s\n";
    cerr << "Speed:   " << prime_count / runtime.count() << " primes/s\n";
    cerr << "Primes:  " << prime_count << "\n";
    cerr << "Count:   " << kCount << "\n";
    cerr << "Threads: " << kThreads << "\n";
    return 0;
}
