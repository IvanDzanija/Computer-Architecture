#include <OpenCL/cl.h>
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#if defined _MSC_VER
#pragma comment(lib, "OpenCL.lib")
#define NOTIFY_CONVENTION __stdcall
#else
#define NOTIFY_CONVENTION
#endif

#include <OpenCL/opencl.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// #include <unistd.h>

#define CL_CHECK(_expr)                                                        \
    do {                                                                       \
        cl_int _err = _expr;                                                   \
        if (_err == CL_SUCCESS)                                                \
            break;                                                             \
        fprintf(stderr, "OpenCL Error: '%s' returned %d!\n", #_expr,           \
                (int)_err);                                                    \
        abort();                                                               \
    } while (0)

void *CL_CHECK_ERR(void *_val) {
    if (_val != NULL) {
        return _val;
    }
    fprintf(stderr, "OpenCL Error\n");
    abort();
    return NULL;
}

void NOTIFY_CONVENTION pfn_notify(const char *errinfo, const void *private_info,
                                  size_t cb, void *user_data) {
    fprintf(stderr, "OpenCL Error (via pfn_notify): %s\n", errinfo);
}

int main(int argc, char **argv) {
    // OpenCL setup
    // Find platform
    cl_platform_id platform;
    CL_CHECK(clGetPlatformIDs(1, &platform, NULL));
    // Find device
    cl_device_id device;
    CL_CHECK(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL));
    // Create context
    cl_int _err = CL_INVALID_VALUE;
    cl_context context = (cl_context)CL_CHECK_ERR(
        clCreateContext(NULL, 1, &device, &pfn_notify, NULL, &_err));
    // Create command queue
    cl_command_queue queue =
        (cl_command_queue)CL_CHECK_ERR(clCreateCommandQueue(
            context, device, CL_QUEUE_PROFILING_ENABLE, &_err));

    // Create and build program
    const char *kernel_sgemm = " __kernel void sgemm(\n \
            const int N,\
            const int M,\
            const int K,\
            __global float* A,\
            __global float* B,\
            __global float* C) {\
            int row = get_global_id(1);\
            int col = get_global_id(0);\
            if (row < N && col < K) {\
                float sum = 0.0f;\
                for (int i = 0; i < M; ++i) {\
                    sum += A[row * M + i] * B[i * K + col];\
                }\
                C[row * K + col] = sum;\
            }\
        }\n";

    cl_program program = (cl_program)CL_CHECK_ERR(
        clCreateProgramWithSource(context, 1, &kernel_sgemm, NULL, &_err));

    // Build program
    if (clBuildProgram(program, 1, &device, "", NULL, NULL) != CL_SUCCESS) {
        char buffer[10240];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                              sizeof(buffer), buffer, NULL);
        fprintf(stderr, "CL Compilation failed:\n%s", buffer);
        abort();
    }

    // Create kernel
    cl_kernel kernel =
        (cl_kernel)CL_CHECK_ERR(clCreateKernel(program, "sgemm", &_err));

    // Events for measurement

    cl_event event_write_a, event_write_b, event_read;
    cl_event event_kernel;

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

        // Random initialization of matrices A and B
        for (int i = 0; i < N * M; ++i) {
            a[i] = (float)(rand() % 100) / 10.0;
        }
        for (int i = 0; i < M * K; ++i) {
            b[i] = (float)(rand() % 100) / 10.0;
        }
        for (int i = 0; i < N * K; ++i) {
            c[i] = 0.0f;
        }

        // Create buffers
        // Matrix A - Read only
        // CL_MEM_COPY_HOST_PTR -> copies data at the moment of buffer creation
        // (hard to measure time)
        cl_mem buffer_a = (cl_mem)CL_CHECK_ERR(
            clCreateBuffer(context, CL_MEM_READ_ONLY, //| CL_MEM_COPY_HOST_PTR,
                           sizeof(float) * N * M, NULL, &_err));
        CL_CHECK(clEnqueueWriteBuffer(queue, buffer_a, CL_FALSE, 0,
                                      sizeof(float) * N * M, a, 0, NULL,
                                      &event_write_a));
        // Matrix B - Read only
        cl_mem buffer_b = (cl_mem)CL_CHECK_ERR(
            clCreateBuffer(context, CL_MEM_READ_ONLY, //| CL_MEM_COPY_HOST_PTR,
                           sizeof(float) * M * K, NULL, &_err));
        CL_CHECK(clEnqueueWriteBuffer(queue, buffer_b, CL_FALSE, 0,
                                      sizeof(float) * M * K, b, 0, NULL,
                                      &event_write_b));

        // Matrix C - Write only
        cl_mem buffer_c = (cl_mem)CL_CHECK_ERR(clCreateBuffer(
            context, CL_MEM_WRITE_ONLY, sizeof(float) * N * K, NULL, &_err));

        // Set kernel arguments
        CL_CHECK(clSetKernelArg(kernel, 0, sizeof(int), &N));
        CL_CHECK(clSetKernelArg(kernel, 1, sizeof(int), &M));
        CL_CHECK(clSetKernelArg(kernel, 2, sizeof(int), &K));
        CL_CHECK(clSetKernelArg(kernel, 3, sizeof(cl_mem), &buffer_a));
        CL_CHECK(clSetKernelArg(kernel, 4, sizeof(cl_mem), &buffer_b));
        CL_CHECK(clSetKernelArg(kernel, 5, sizeof(cl_mem), &buffer_c));

        // Execute kernel
        size_t global_work_size[2] = {K, N};
        CL_CHECK(clEnqueueNDRangeKernel(queue, kernel, 2, NULL,
                                        global_work_size, NULL, 0, NULL,
                                        &event_kernel));

        // Read back result
        CL_CHECK(clEnqueueReadBuffer(queue, buffer_c, CL_TRUE, 0,
                                     sizeof(float) * N * K, c, 0, NULL,
                                     &event_read));

        // Asynchronous finish
        CL_CHECK(clFinish(queue));

        cl_ulong start, end;
        double time_write = 0, time_kernel = 0, time_read = 0; // ms

        // Write Matrix A time
        clGetEventProfilingInfo(event_write_a, CL_PROFILING_COMMAND_START,
                                sizeof(start), &start, NULL);
        clGetEventProfilingInfo(event_write_a, CL_PROFILING_COMMAND_END,
                                sizeof(end), &end, NULL);
        time_write += (double)(end - start) * 1e-6;

        // Write Matrix B time
        clGetEventProfilingInfo(event_write_b, CL_PROFILING_COMMAND_START,
                                sizeof(start), &start, NULL);
        clGetEventProfilingInfo(event_write_b, CL_PROFILING_COMMAND_END,
                                sizeof(end), &end, NULL);
        time_write += (double)(end - start) * 1e-6;

        // Kernel execution time
        clGetEventProfilingInfo(event_kernel, CL_PROFILING_COMMAND_START,
                                sizeof(start), &start, NULL);
        clGetEventProfilingInfo(event_kernel, CL_PROFILING_COMMAND_END,
                                sizeof(end), &end, NULL);
        time_kernel += (double)(end - start) * 1e-6;

        // Read Matrix C time
        clGetEventProfilingInfo(event_read, CL_PROFILING_COMMAND_START,
                                sizeof(start), &start, NULL);
        clGetEventProfilingInfo(event_read, CL_PROFILING_COMMAND_END,
                                sizeof(end), &end, NULL);
        time_read += (double)(end - start) * 1e-6;

        printf("\n--- PROFILIRANJE ---\n");
        printf("Prijenos RAM -> GPU: %f ms\n", time_write);
        printf("Izvodenje (Kernel):  %f ms\n", time_kernel);
        printf("Prijenos GPU -> RAM: %f ms\n", time_read);
        printf("Ukupno (bez setupa): %f ms\n",
               time_write + time_kernel + time_read);

        double size_A_GB =
            (double)(sizeof(float) * N * M) / (1024.0 * 1024.0 * 1024.0);
        double size_B_GB =
            (double)(sizeof(float) * M * K) / (1024.0 * 1024.0 * 1024.0);
        double size_C_GB =
            (double)(sizeof(float) * N * K) / (1024.0 * 1024.0 * 1024.0);

        double speed_write = (size_A_GB + size_B_GB) / (time_write / 1000.0);
        double speed_read = size_C_GB / (time_read / 1000.0);

        // ADD and MUL per element
        double total_flops = 2.0 * (double)N * (double)M * (double)K;
        double tflops = (total_flops / 1e12) / (time_kernel / 1000.0);
        double gflops = (total_flops / 1e9) / (time_kernel / 1000.0);

        printf("\n--- ANALIZA ZA N=%zu ---\n", N);
        printf("Brzina RAM -> GPU: %.2f GB/s\n", speed_write);
        printf("Brzina GPU -> RAM: %.2f GB/s\n", speed_read);
        printf("Performanse GPU:   %.4f TFLOPS (%.2f GFLOPS)\n", tflops,
               gflops);

        // Random print to prevent optimization
        printf("Random check: %f\n", c[(N / 2) * K + (K / 2)]);

        if (muls[i] == 1) {
            printf("Provjera tocnosti rezultata...\n");
            float *c_cpu = (float *)malloc(sizeof(float) * N * K);
            bool match = true;

            for (int r = 0; r < N; ++r) {
                for (int c_idx = 0; c_idx < K; ++c_idx) {
                    float sum = 0.0f;
                    for (int k_idx = 0; k_idx < M; ++k_idx) {
                        sum += a[r * M + k_idx] * b[k_idx * K + c_idx];
                    }
                    c_cpu[r * K + c_idx] = sum;

                    if (fabs(c_cpu[r * K + c_idx] - c[r * K + c_idx]) > 0.1f) {
                        match = false;
                    }
                }
            }

            if (match)
                printf("USPJEH! Rezultati se podudaraju.\n");
            else
                printf("POGREŠKA! Rezultati se razlikuju.\n");

            free(c_cpu);
        }

        // Cleanup
        CL_CHECK(clReleaseMemObject(buffer_a));
        CL_CHECK(clReleaseMemObject(buffer_b));
        CL_CHECK(clReleaseMemObject(buffer_c));
        free(a);
        free(b);
        free(c);
    }

    // Cleanup

    CL_CHECK(clReleaseKernel(kernel));
    CL_CHECK(clReleaseProgram(program));
    CL_CHECK(clReleaseContext(context));
    CL_CHECK(clReleaseCommandQueue(queue));

    //  CL_CHECK(clEnqueueWriteBuffer(queue, input_buffer, CL_TRUE, 0,
    //                                sizeof(int) * NUM_DATA, buf_in, 0, NULL,
    //                                NULL));
    // size_t global_work_size[1] = {NUM_DATA};
    // CL_CHECK(clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global_work_size,
    //                                 NULL, 0, NULL, NULL));
    // CL_CHECK(clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0,
    //                              sizeof(int) * NUM_DATA, buf_out, 0, NULL,
    //                              NULL));

    return 0;
}
