#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

float a_ram[1024][1024];

void calc_ram_to_ram_speed(void) {
    // Calculate RAM to RAM speed
    float *test_ram = (float *)malloc(sizeof(float) * 1024 * 1024);
    clock_t ram_start = clock();
    memcpy(test_ram, a_ram, sizeof(float) * 1024 * 1024);
    clock_t ram_end = clock();
    double ram_dur = (double)(ram_end - ram_start) / CLOCKS_PER_SEC;
    double ram_gb =
        (double)(sizeof(float) * 1024 * 1024) / (1024 * 1024 * 1024);
    printf("Brzina RAM -> RAM: %.2f GB/s\n", ram_gb / ram_dur);
    free(test_ram);
}

int main(void) {
    calc_ram_to_ram_speed();

    size_t size = 128;
    size_t muls[] = {1, 2, 4, 8, 16};

    for (size_t i = 0; i < sizeof(muls) / sizeof(muls[0]); i++) {
        const size_t N = size * muls[i];
        const size_t M = size * muls[i];
        const size_t K = size * muls[i];
        float *a = (float *)malloc(sizeof(float) * N * M);
        float *b = (float *)malloc(sizeof(float) * M * K);
        float *c = (float *)malloc(sizeof(float) * N * K);

        srand((unsigned int)time(NULL));
        for (int i = 0; i < N * M; ++i) {
            a[i] = (float)(rand() % 100) / 10.0;
        }
        for (int i = 0; i < M * K; ++i) {
            b[i] = (float)(rand() % 100) / 10.0;
        }
        for (int i = 0; i < N * K; ++i) {
            c[i] = 0.0f;
        }

        clock_t begin = clock();
        // Naive GEMM implementation
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < K; ++j) {
                for (int l = 0; l < M; ++l) {
                    c[i * K + j] += a[i * M + l] * b[l * K + j];
                }
            }
        }

        printf("\n--- ANALIZA ZA N=%zu ---\n", N);
        printf("Random check: %f\n", c[(N / 2) * K + (K / 2)]);

        clock_t end = clock();
        double dur = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("Time taken: %.3f ms\n", dur * 1000);

        double total_flops = 2.0 * (double)N * (double)M * (double)K;
        double tflops = (total_flops / 1e12) / (dur);
        double gflops = (total_flops / 1e9) / (dur);

        printf("Performance CPU:   %.4f TFLOPS (%.2f GFLOPS)\n", tflops,
               gflops);
    }

    return 0;
}
