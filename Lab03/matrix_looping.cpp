#include <bits/stdc++.h>
using u64 = uint64_t;
using i64 = int64_t;
using u32 = uint32_t;
using i32 = int32_t;
using u8 = uint8_t;
using i8 = int8_t;

template <typename T> class Matrix {
  private:
    size_t _rows, _cols, _size;
    volatile T *_data;

    size_t _index(size_t row, size_t col) { return row * _cols + col; }

  public:
    Matrix(size_t rows, size_t cols)
        : _rows(rows), _cols(cols), _size(rows * cols) {}

    Matrix(size_t rows, size_t cols, volatile T *data)
        : _rows(rows), _cols(cols), _size(rows * cols), _data(data) {}

    T at(size_t i, size_t j) { return _data[_index(i, j)]; }

    void set(size_t i, size_t j, T v) { _data[_index(i, j)] = v; }

    i64 sum_row_major() {
        i64 sum = 0;
        for (size_t i = 0; i < _rows; ++i)
            for (size_t j = 0; j < _cols; ++j)
                sum += at(i, j);
        return sum;
    }

    i64 sum_col_major() {
        i64 sum = 0;
        for (size_t j = 0; j < _cols; ++j)
            for (size_t i = 0; i < _rows; ++i)
                sum += at(i, j);
        return sum;
    }
};

signed main(void) {
    constexpr i32 b1 = 128, b2 = 128; // Cache line size;

    constexpr i32 s1_perf = 131072; // Performance core L1 cache size
    constexpr i32 s1_effi = 65536;  // Efficiency core L1 cache size

    constexpr i32 s2_perf = 16777216 / 2; // Performance core L2 cache size
    constexpr i32 s2_effi = 4194304;      // Efficiency core L2 cache size

    // M2 processor - Performance core(4)
    // M2 processor doesn't have L3 cache but rather shared SLC

    static constexpr size_t N = 128;
    static constexpr size_t M = s2_perf / sizeof(int);

    std::cout << "N=" << N << ", M=" << M << std::endl;

    alignas(128) volatile int *buffer =
        (volatile int *)aligned_alloc(4096, N * M * sizeof(int));

    Matrix<int> A(N, M, buffer);

    for (size_t i = 0; i < N; ++i)
        for (size_t j = 0; j < M; ++j)
            A.set(i, j, 1);

    clock_t t1 = clock();
    i64 s1 = A.sum_row_major();
    clock_t t2 = clock();

    clock_t t3 = clock();
    i64 s2 = A.sum_col_major();
    clock_t t4 = clock();

    double dt_row = double(t2 - t1) / CLOCKS_PER_SEC;
    double dt_col = double(t4 - t3) / CLOCKS_PER_SEC;

    std::cout << "Row-major sum: " << s1 << ", time = " << dt_row << " s\n";
    std::cout << "Col-major sum: " << s2 << ", time = " << dt_col << " s\n";
    std::cout << "Performance ratio (col/row): " << dt_col / dt_row << "\n";
    return 0;
}
