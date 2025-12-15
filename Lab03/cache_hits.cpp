#include <bits/stdc++.h>
/*
 * Uvedimo sljedeće oznake:
 * s1 ... veličina priručne memorije L1;
 * b1 ... veličina linije priručne memorije L1;
 * s2, b2 ... analogno za priručnu memoriju L2;
 * s3, b3 ... analogno za priručnu memoriju L3.
 *
 * M2 processor (MacOS):
 * hw.perflevel0.l1icachesize: 19660
 * hw.perflevel0.l1dcachesize: 131072
 * hw.perflevel0.l2cachesize: 16777216
 * hw.perflevel1.l1icachesize: 131072
 * hw.perflevel1.l1dcachesize: 65536
 * hw.perflevel1.l2cachesize: 4194304
 *
 * Performance core clock: 3.49 GHz
 * s1 = 128 KiB
 * s2 = 4 MiB (shared)
 * s3 = 8MiB (shared with rest of system, eg. GPU)
 * b1 = b2 = b3 = 128
 */

using u64 = uint64_t;
using i64 = int64_t;
using u32 = uint32_t;
using i32 = int32_t;
using u8 = uint8_t;
using i8 = int8_t;

struct Result {
    u64 accesses;
    double time;
    u64 sum;

    void print(char task) {
        double ns_per_access = (time * 1e9) / double(accesses);
        double mb_per_s = (double(accesses) * 1.0) / (1024.0 * 1024.0) / time;

        std::cout << std::fixed << std::setprecision(3);
        std::cout << "Potprogram " << task << std::endl;
        std::cout << "Ukupno pristupa: " << accesses << std::endl;
        std::cout << "Ukupno vrijeme: " << time << " s" << std::endl;
        std::cout << "Vrijeme pristupa: " << ns_per_access << " ns"
                  << std::endl;
        std::cout << "Propusnost: " << mb_per_s << " MB/s" << std::endl;
        std::cout << std::endl;
    }
};

volatile u64 final_sum = 0;

void init_buf(u8 *buf, size_t n) {
    memset(buf, 0, n);
    for (i64 i = 0; i < n; ++i) {
        volatile u8 tmp = buf[i];
    }
}

Result measure_A(volatile u8 *buf, size_t buf_size, u64 laps) {
    u64 accs = 0;
    clock_t start = clock();
    while (laps--) {
        for (size_t i = 0; i < buf_size; ++i) {
            buf[i] += 1;
            ++accs;
        }
    }
    double t = double(clock() - start) / CLOCKS_PER_SEC;
    u64 sum = 0;
    for (size_t i = 0; i < buf_size; ++i)
        sum += buf[i];
    final_sum += sum;
    return {accs, t, sum};
}

Result measure_sparse(volatile u8 *buf, size_t buf_size, size_t stride,
                      u64 laps) {
    u64 accs = 0;
    clock_t start = clock();

    while (laps--) {
        for (size_t i = 0; i < buf_size; i += stride) {
            buf[i] += 1;
            ++accs;
        }
    }
    double t = double(clock() - start) / CLOCKS_PER_SEC;
    u64 sum = 0;
    for (size_t i = 0; i < buf_size; ++i)
        sum += buf[i];
    final_sum += sum;
    return {accs, t, sum};
}

int main() {
    // M2 processor - Performance core(4)
    // M2 processor doesn't have L3 cache but rather shared SLC
    constexpr size_t s1 = 131072;       // L1 cache size
    constexpr size_t s2 = 16777216 / 2; // L2 cache size (shared)
    // Division by 2 is heuristic, taking half of total L2 shared cache
    constexpr size_t s3 = 32 * 1024 * 1024; // SLC (system shared)
    constexpr size_t b1 = 128;              // L1 line size
    constexpr size_t b2 = 128;              // L2 line size
    constexpr size_t b3 = 128;              // L3 line size
    constexpr size_t delta = 13;

    constexpr size_t bufA_size = s1;
    constexpr size_t bufB_size = 2 * s1 * delta;
    constexpr size_t bufC_size = 2 * s2 * delta;
    constexpr size_t bufD_size = 2 * s3 * delta;

    constexpr size_t strideB = b1 * delta;
    constexpr size_t strideC = b2 * delta;
    constexpr size_t strideD = b3 * delta;

    constexpr u64 laps = 1000;

    alignas(128) static u8 bufA[bufA_size];
    alignas(128) static u8 bufB[bufB_size];
    alignas(128) static u8 bufC[bufC_size];
    alignas(128) static u8 bufD[bufD_size];

    init_buf(bufA, bufA_size);
    init_buf(bufB, bufB_size);
    init_buf(bufC, bufC_size);
    init_buf(bufD, bufD_size);

    Result rA = measure_A(bufA, bufA_size, laps * 20);
    Result rB = measure_sparse(bufB, bufB_size, strideB, laps * 500);
    Result rC = measure_sparse(bufC, bufC_size, strideC, laps * 5);
    Result rD = measure_sparse(bufD, bufD_size, strideD, laps);

    rA.print('A');
    rB.print('B');
    rC.print('C');
    rD.print('D');

    double tA = rA.time / double(rA.accesses);
    double tB = rB.time / double(rB.accesses);
    double tC = rC.time / double(rC.accesses);
    double tD = rD.time / double(rD.accesses);

    std::cout << "Ratios: " << std::endl;
    std::cout << "t(L2)/t(L1) = " << tB / tA << std::endl;
    std::cout << "t(L3)/t(L2) = " << tC / tB << std::endl;
    std::cout << "t(RAM)/t(L3) = " << tD / tC << std::endl;
    std::cout << std::endl;
    std::cout << final_sum << std::endl;

    return 0;
}
