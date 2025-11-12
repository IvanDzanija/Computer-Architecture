// #include <cblas.h>
#include <iostream>
#include <time.h>

extern "C" int potprogram_asm(int a, int b, int c);
extern "C" int igra_registrima(void);
extern "C" int zbrajanje_asm(int n);
extern "C" void zbrajanje_vektora_x87(float const *A, float const *B, int count,
                                      float *R);
extern "C" void zbrajanje_vektora_sse(float const *A, float const *B, int count,
                                      float *R);

int potprogram_c(int a, int b, int c) { return (a + b) * c; }

int zbrajanje(int n) {
    int ans = 0;
    for (int i = 0; i < n; ++i) {
        ans += i;
    }
    return ans;
}

void zbrajanje_vektora(float const *A, float const *B, int count, float *R) {
    for (int i = 0; i < count; ++i) {
        R[i] = A[i] + B[i];
    }
}

void zbrajanje_vektora_duff(float const *A, float const *B, int count,
                            float *R) {
    if (count <= 0) {
        return;
    }
    int n = (count + 7) / 8;
    switch (count % 8) {
    case 0:
        do {
            *R++ = *A++ + *B++;
        case 7:
            *R++ = *A++ + *B++;
        case 6:
            *R++ = *A++ + *B++;
        case 5:
            *R++ = *A++ + *B++;
        case 4:
            *R++ = *A++ + *B++;
        case 3:
            *R++ = *A++ + *B++;
        case 2:
            *R++ = *A++ + *B++;
        case 1:
            *R++ = *A++ + *B++;
        } while (--n > 0);
    }
}

const int size = 1024 * 1024 * 16;
__attribute__((aligned(32))) float a[size], b[size], r[size];

int main() {
    // 9A
    // int asm_rez = potprogram_asm(3, 5, 6);
    // int c_rez = potprogram_c(3, 5, 6);
    // std::cout << "ASM: " << asm_rez << std::endl;
    // std::cout << "C++: " << c_rez << std::endl;

    // 9B
    // int reg = igra_registrima();
    // std::cout << "9B: eax = " << reg << std::endl;

    // 9C
    int n;
    std::cin >> n;
    int corr = n * (n - 1) / 2;
    std::cout << "Correct ans: " << corr << std::endl;
    std::cout << "ASM Impl: " << zbrajanje_asm(n) << std::endl;
    std::cout << "C Impl: " << zbrajanje(n) << std::endl;

    // 9D
    //     std::cout << "A: ";
    //   for (int i = 0; i < size; ++i) {
    //       a[i] = i, b[i] = i * 2, r[i] = 0;
    //              std::cout << a[i] << ' ';
    //   }
    //     std::cout << std::endl << "B: ";
    //     for (int i = 0; i < size; ++i){
    //        std::cout << b[i] << ' ';
    //     }
    //    std::cout << std::endl;

    //  clock_t start = clock();
    //  zbrajanje_vektora(a, b, size, r);
    //  std::cout << "C impl: ";
    //  float seconds = (float)(clock() - start) / CLOCKS_PER_SEC;
    //  printf("%.4f\n", seconds);
    //  for (int i = 0; i < size; ++i) {
    //         std::cout << r[i] << ' ';
    //      r[i] = 0;
    //  }

    //  start = clock();
    //  zbrajanje_vektora_x87(a, b, size, r);
    //  seconds = (float)(clock() - start) / CLOCKS_PER_SEC;
    //  std::cout << std::endl << "x87 impl: ";
    //  printf("%.4f\n", seconds);
    //  for (int i = 0; i < size; ++i) {
    //             std::cout << r[i] << ' ';
    //      r[i] = 0;
    //  }

    //  start = clock();
    //  zbrajanje_vektora_sse(a, b, size, r);
    //  seconds = (float)(clock() - start) / CLOCKS_PER_SEC;
    //  std::cout << std::endl << "SSE impl: ";
    //  printf("%.4f\n", seconds);
    //  for (int i = 0; i < size; ++i) {
    //             std::cout << r[i] << ' ';
    //      r[i] = 0;
    //  }
    //  std::cout << std::endl;

    //  start = clock();
    //  zbrajanje_vektora_duff(a, b, size, r);
    //  seconds = (float)(clock() - start) / CLOCKS_PER_SEC;
    //  std::cout << "Duff impl: ";
    //  printf("%.4f\n", seconds);
    //  for (int i = 0; i < size; ++i) {
    //            std::cout << r[i] << ' ';
    //      r[i] = 0;
    //  }
    //  std::cout << std::endl;

    //  start = clock();
    //   cblas_saxpy(size, 1, a, 1, b, 1);
    //  seconds = (float)(clock() - start) / CLOCKS_PER_SEC;
    //  std::cout << "Duff impl: ";
    //  printf("%.4f\n", seconds);

    //  return 0;
}
