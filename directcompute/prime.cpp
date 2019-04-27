#include <chrono>
#include <iostream>
#include <fstream>
#include "ComputeDevice.h"

using namespace std;

// values must match those from shader source
static constexpr UINT kStart{ 1000001 };  // start must be odd
//static constexpr UINT kCount{ 50331648 };
static constexpr UINT kCount{ 20971520 };
static constexpr UINT kThreads{ 64 };
static constexpr UINT kThreadGroups{ 256 };

static constexpr UINT kEnd{ kStart + kCount }; // check if it fits into ValType
static_assert((kStart & 1) == 1);
static_assert(kCount % kThreads == 0);
static constexpr UINT kCountPerThread{ kCount / (kThreads * kThreadGroups) };
static_assert((kCountPerThread % 64) == 0);

int main() {
    UINT prime_count{ 0 };

    cerr << "primes:\n";
    auto begin_time{ chrono::high_resolution_clock::now() };

    ComputeDevice comp;
    try {
        cerr << "Init\n";
        comp.Init(L"prime.hlsl", "main", kCount / 64);
    } catch (const std::runtime_error &ex) {
        cerr << ex.what() << endl;
        throw;
    } catch (const winrt::hresult_error &ex) {
        wcerr << ex.code() << ": " << ex.message().c_str() << endl;
        wcerr << comp.GetError() << endl;
        throw;
    }

    try {
        cerr << "Run\n";
        comp.Run(kThreadGroups);
    } catch (const std::runtime_error &ex) {
        cerr << ex.what() << endl;
        throw;
    } catch (const winrt::hresult_error &ex) {
        wcerr << ex.code() << ": " << ex.message().c_str() << endl;
        wcerr << comp.GetError() << endl;
        throw;
    }

    UINT *out;
    try {
        cerr << "GetOutput\n";
        out = comp.GetOutput();
    } catch (const std::runtime_error &ex) {
        cerr << ex.what() << endl;
        throw;
    } catch (const winrt::hresult_error &ex) {
        wcerr << ex.code() << ": " << ex.message().c_str() << endl;
        wcerr << comp.GetError() << endl;
        throw;
    }

    auto mid_time{ chrono::high_resolution_clock::now() };

    ofstream out_strm("primes.txt");
    for (UINT i = 0; i < kCount / 64; ++i)
        for (UINT j = 0; j < 32; ++j)
            if (out[i] & (1 << j)) {
                out_strm << to_string(kStart + (i * 32 + j) * 2) + "\n";
                ++prime_count;
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
    cerr << "Threads:  " << kThreads << " (gpu shader threads)\n";
    cerr << "Groups:   " << kThreadGroups << " (gpu shader thread groups)\n";
    return 0;
}
