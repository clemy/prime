
// values must match those from main source
#define kStart 1000001
#define kCount 20971520
#define kThreads 64
#define kThreadGroups 256

#define kCountPerThread (kCount / (kThreads * kThreadGroups))

bool checkPrime(uint p) {
    uint max = sqrt(p) + 1;
    for (uint i = 3; i <= max; i += 2) {
        if (p % i == 0) {
            return false;
        }
    }
    return true;
}

RWByteAddressBuffer BufferOut : register(u0);
[numthreads(kThreads, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    int tocheck = kStart + DTid.x * kCountPerThread;
    for (uint i = 0; i < kCountPerThread / 64; ++i) {
        uint bitset = 0;
        for (uint j = 0; j < 32; ++j) {
            if (checkPrime(tocheck)) {
                bitset |= 1 << j;
            }
            tocheck += 2;
        }
        BufferOut.Store(DTid.x * (kCountPerThread / 16) + i * 4, bitset);
    }
}
