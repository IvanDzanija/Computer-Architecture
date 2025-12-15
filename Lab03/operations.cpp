#include <bits/stdc++.h>
/*
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
 */

using u64 = uint64_t;
using i64 = int64_t;
using u32 = uint32_t;
using i32 = int32_t;
using u16 = uint16_t;
using i16 = int16_t;
using u8 = uint8_t;
using i8 = int8_t;

// --- Ažurirane globalne volatile konstante ---
volatile u64 final_sum = 0;
volatile int magic_i = 23;
volatile float magic_f = 23.0f;
volatile double magic_d = 23.0;
// ---------------------------------------------

struct Result {
    u64 accesses;
    double time;
    u64 sum;

    void print(const std::string &type, size_t size_bytes, char op) {
        double ns_per_access = (time * 1e9) / double(accesses);

        // Formatiramo rezultat u ns po operaciji i printamo unutar tablice
        printf("| %s (%zuB) | %c | %15.3f ns |\n", type.c_str(), sizeof(u8), op,
               ns_per_access);
    }
};

// ... (init_buf funkcije ostaju iste, koristit ce se u main funkciji) ...

void init_buf_c(u8 *buf, size_t n) {
    for (u64 i = 0; i < n; ++i) {
        buf[i] = 1;
        volatile u8 tmp = buf[i];
    }
}

void init_buf_s(u16 *buf, size_t n) {
    for (u64 i = 0; i < n; ++i) {
        buf[i] = 1;
        volatile u16 tmp = buf[i];
    }
}

void init_buf_i(u32 *buf, size_t n) {
    for (u64 i = 0; i < n; ++i) {
        buf[i] = 1;
        volatile u32 tmp = buf[i];
    }
}

void init_buf_f(float *buf, size_t n) {
    for (u64 i = 0; i < n; ++i) {
        buf[i] = 1;
        volatile float tmp = buf[i];
    }
}
void init_buf_d(double *buf, size_t n) {
    for (u64 i = 0; i < n; ++i) {
        buf[i] = 1;
        volatile double tmp = buf[i];
    }
}

// ----------------------------------------------------------------------------------
// --- GLAVNA IZMJENA: Funkcija measure s operacijom i magic konstantom ---
// ----------------------------------------------------------------------------------

template <typename T>
Result measure(volatile T *buf, size_t buf_size, u64 laps, char op_type) {
    u64 accs = 0;
    clock_t start = clock();

    T volatile magic_val;
    if constexpr (std::is_floating_point_v<T>) {
        if constexpr (std::is_same_v<T, float>)
            magic_val = (T)magic_f;
        else
            magic_val = (T)magic_d;
    } else {
        magic_val = (T)magic_i;
    }

    while (laps--) {
        for (size_t i = 0; i < buf_size; ++i) {
            // Glavna operacija: prisiljena da se izvrši zbog volatile
            switch (op_type) {
            case '+':
                buf[i] += magic_val;
                break;
            case '*':
                buf[i] *= magic_val;
                break;
            case '/':
                buf[i] /= magic_val;
                break;
            default:
                break;
            }
            ++accs;
        }
    }
    double t = double(clock() - start) / CLOCKS_PER_SEC;

    // Zbroj za sprječavanje eliminacije koda (kao i ranije)
    u64 sum = 0;
    for (size_t i = 0; i < buf_size; ++i)
        sum += (u64)buf[i];
    final_sum += sum;

    return {accs, t, sum};
}

// ----------------------------------------------------------------------------------
// --- NOVE FUNKCIJE ZA TABLIČNI ISPIS I TESTIRANJE ---
// ----------------------------------------------------------------------------------

void print_header() {
    std::cout << "\n## 📊 Performanse: Tipovi Podataka i Aritmeticke Operacije "
                 "(M2/ARM64)\n";
    std::cout << "-------------------------------------------------------------"
                 "----\n";
    std::cout << "| Tip (Size) | Operacija | Vrijeme po Pristupu (ns) |\n";
    std::cout << "|------------|-----------|--------------------------|\n";
}

template <typename T>
void run_test_case(const std::string &type_name, volatile T *bufA, size_t sizeA,
                   volatile T *bufB, size_t sizeB, volatile T *bufC,
                   size_t sizeC, u64 lapsA, u64 lapsB, u64 lapsC) {

    std::vector<char> ops = {'+', '*', '/'};
    std::vector<std::pair<volatile T *, size_t>> buffers = {
        {bufA, sizeA}, {bufB, sizeB}, {bufC, sizeC}};
    std::vector<u64> current_laps = {lapsA, lapsB, lapsC};

    for (size_t i = 0; i < buffers.size(); ++i) {
        volatile T *buf = buffers[i].first;
        size_t size = buffers[i].second;
        u64 laps = current_laps[i];

        // Moramo ponovno inicijalizirati buffer za svaku operaciju da bi bio
        // cist
        for (char op : ops) {

            // Re-inicijalizacija buffer-a (MORA BITI PRIJE MJERENJA)
            for (size_t j = 0; j < size; ++j)
                buf[j] = (T)1;

            Result r = measure(buf, size, laps, op);

            // Ispis rezultata u red
            printf("| %s (%zuB) | %c | %15.3f ns |\n", type_name.c_str(),
                   sizeof(T), op, (r.time * 1e9) / double(r.accesses));
        }
    }
}

int main() {
    std::cout << std::fixed << std::setprecision(3);

    // M2 cache sizes
    constexpr size_t s1 = 131072;       // L1 cache size
    constexpr size_t s2 = 16777216 / 2; // L2 cache size (heuristic)

    // Veličine spremnika u bajtovima
    constexpr size_t bufA_size_bytes = s1 / 8; // 16 KiB
    constexpr size_t bufB_size_bytes = s2 / 8; // 1 MiB
    constexpr size_t bufC_size_bytes = 2 * s2; // 16 MiB

    // Broj elemenata i polja se računa automatski (vaš originalni kod)

    // Ponavljanja (Laps) su prilagođena velicinama spremnika
    static constexpr u64 laps = 100;          // Osnovna vrijednost
    static constexpr u64 lapsC = laps;        // 16 MB
    static constexpr u64 lapsB = laps * 10;   // 1 MB
    static constexpr u64 lapsA = laps * 1000; // 16 KB

    // --- ALOKACIJA I INICIJALIZACIJA (VAŠ ORIGINALNI KOD) ---

    alignas(128) static u16 SbufA[bufA_size_bytes / sizeof(u16)];
    alignas(128) static u16 SbufB[bufB_size_bytes / sizeof(u16)];
    alignas(128) static u16 SbufC[bufC_size_bytes / sizeof(u16)];
    // Nema potrebe za init_buf_* u main-u, jer cemo ih inicijalizirati unutar
    // run_test_case

    alignas(128) static u8 CbufA[bufA_size_bytes];
    alignas(128) static u8 CbufB[bufB_size_bytes];
    alignas(128) static u8 CbufC[bufC_size_bytes];

    alignas(128) static u32 IbufA[bufA_size_bytes / sizeof(u32)];
    alignas(128) static u32 IbufB[bufB_size_bytes / sizeof(u32)];
    alignas(128) static u32 IbufC[bufC_size_bytes / sizeof(u32)];

    alignas(128) static float FbufA[bufA_size_bytes / sizeof(float)];
    alignas(128) static float FbufB[bufB_size_bytes / sizeof(float)];
    alignas(128) static float FbufC[bufC_size_bytes / sizeof(float)];

    alignas(128) static double DbufA[bufA_size_bytes / sizeof(double)];
    alignas(128) static double DbufB[bufB_size_bytes / sizeof(double)];
    alignas(128) static double DbufC[bufC_size_bytes / sizeof(double)];

    // --- POKRETANJE TESTOVA I ISPIS TABLICE ---
    print_header();

    // Tip: char (u8)
    run_test_case<u8>("char (A)", CbufA, bufA_size_bytes, CbufB,
                      bufB_size_bytes, CbufC, bufC_size_bytes, lapsA, lapsB,
                      lapsC);

    // Tip: short (u16)
    run_test_case<u16>("short (B)", SbufA, bufA_size_bytes / 2, SbufB,
                       bufB_size_bytes / 2, SbufC, bufC_size_bytes / 2, lapsA,
                       lapsB, lapsC);

    // Tip: int (u32)
    run_test_case<u32>("int (C)", IbufA, bufA_size_bytes / 4, IbufB,
                       bufB_size_bytes / 4, IbufC, bufC_size_bytes / 4, lapsA,
                       lapsB, lapsC);

    // Tip: float (float)
    run_test_case<float>("float (D)", FbufA, bufA_size_bytes / 4, FbufB,
                         bufB_size_bytes / 4, FbufC, bufC_size_bytes / 4, lapsA,
                         lapsB, lapsC);

    // Tip: double (double)
    run_test_case<double>("double (E)", DbufA, bufA_size_bytes / 8, DbufB,
                          bufB_size_bytes / 8, DbufC, bufC_size_bytes / 8,
                          lapsA, lapsB, lapsC);

    std::cout << "-------------------------------------------------------------"
                 "----\n";
    std::cout << "Final Checksum (za sprecavanje eliminacije koda): "
              << final_sum << std::endl;
    return 0;
}
