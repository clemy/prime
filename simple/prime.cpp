#include <cmath>
#include <chrono>
#include <iostream>

using namespace std;

constexpr int kStart{ 1000000 };
constexpr int kCount{ 20971520 };

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

int searchPrimes() {
    constexpr int start = (kStart | 1); // start must be odd => set bit 0
    constexpr int end = kStart + kCount;
    int prime_count{ 0 };

    for (int i{ start }; i < end; i += 2) {
        if (checkPrime(i)) {
            cout << to_string(i) + "\n";
            ++prime_count;
        }
    }
    return prime_count;
}

int main() {
    cerr << "primes:\n";
    auto begin_time{ chrono::high_resolution_clock::now() };
    auto prime_count{ searchPrimes() };
    auto end_time{ chrono::high_resolution_clock::now() };
    chrono::duration<double> runtime{ end_time - begin_time };
    cerr << "Time:   " << runtime.count() << " s\n";
    cerr << "Speed:  " << kCount / runtime.count() << " numbers/s\n";
    cerr << "Speed:  " << prime_count / runtime.count() << " primes/s\n";
    cerr << "Primes: " << prime_count << "\n";
    cerr << "Count:  " << kCount << "\n";
    cerr << "Threads: 1 (single-threaded)\n";
    return 0;
}
