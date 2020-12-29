#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include<stdio.h>
#include<stdlib.h>
#include <math.h>
#include <Windows.h>
#include <time.h>
#include <assert.h>

#define CUDA_CALL(x) { const cudaError_t a = (x); if(a != cudaSuccess) { printf("\nCuda Error: %s (err_num=%d) at line:%d\n", cudaGetErrorString(a), a, __LINE__); cudaDeviceReset(); assert(0);}}
typedef float TIMER_T;

#define USE_CPU_TIMER 1
#define USE_GPU_TIMER 1

#if USE_CPU_TIMER == 1
__int64 start, freq, end;
#define CHECK_TIME_START { QueryPerformanceFrequency((LARGE_INTEGER*)&freq); QueryPerformanceCounter((LARGE_INTEGER*)&start); }
#define CHECK_TIME_END(a) { QueryPerformanceCounter((LARGE_INTEGER*)&end); a = (float)((float)(end - start) / (freq / 1000.0f)); }
#else
#define CHECK_TIME_START
#define CHECK_TIME_END(a)
#endif

#if USE_GPU_TIMER == 1
cudaEvent_t cuda_timer_start, cuda_timer_stop;
#define CUDA_STREAM_0 (0)

void create_device_timer()
{
    CUDA_CALL(cudaEventCreate(&cuda_timer_start));
    CUDA_CALL(cudaEventCreate(&cuda_timer_stop));
}

void destroy_device_timer()
{
    CUDA_CALL(cudaEventDestroy(cuda_timer_start));
    CUDA_CALL(cudaEventDestroy(cuda_timer_stop));
}

inline void start_device_timer()
{
    cudaEventRecord(cuda_timer_start, CUDA_STREAM_0);
}

inline TIMER_T stop_device_timer()
{
    TIMER_T ms;
    cudaEventRecord(cuda_timer_stop, CUDA_STREAM_0);
    cudaEventSynchronize(cuda_timer_stop);

    cudaEventElapsedTime(&ms, cuda_timer_start, cuda_timer_stop);
    return ms;
}

#define CHECK_TIME_INIT_GPU() { create_device_timer(); }
#define CHECK_TIME_START_GPU() { start_device_timer(); }
#define CHECK_TIME_END_GPU(a) { a = stop_device_timer(); }
#define CHECK_TIME_DEST_GPU() { destroy_device_timer(); }
#else
#define CHECK_TIME_INIT_GPU()
#define CHECK_TIME_START_GPU()
#define CHECK_TIME_END_GPU(a)
#define CHECK_TIME_DEST_GPU()
#endif

#define BLOCK_SIZE 64
#define DATA_SIZE 10000

TIMER_T compute_time = 0;
TIMER_T device_time = 0;

int N;
int numbers[DATA_SIZE];
int numbers_CPU[DATA_SIZE];

__global__ void reduction_GPU(int* data, int data_size) {
    int idx = blockDim.x * blockIdx.x + threadIdx.x;
    if (idx < data_size) {
        for (int stride = 1; stride < data_size; stride *= 2) {
            if (idx % (2 * stride) == 0) {
                int lhs = data[idx];
                int rhs = data[idx + stride];
                data[idx] = lhs < rhs ? rhs : lhs;
            }
            __syncthreads();
        }
    }
}

__global__ void reduction_no_diverge_GPU(int* data, int data_size) {
    int idx = blockDim.x * blockIdx.x + threadIdx.x;
    if (idx < data_size) {
        for (int stride = 1; stride < data_size; stride *= 2) {
            int index = 2 * stride * index;
            if (index < blockDim.x) {
                int lhs = data[idx];
                int rhs = data[idx + stride];
                data[idx] = lhs < rhs ? rhs : lhs;
            }
            __syncthreads();
        }
    }
}

#define MAX_CUDA_THREADS_PER_BLOCK 1024
__global__ void reduction_shared_no_diverge_GPU(int* data, int data_size) {
    int idx = blockDim.x * blockIdx.x + threadIdx.x;
    __shared__ float sdata[MAX_CUDA_THREADS_PER_BLOCK];
    if (idx < data_size) {

        /*copy to shared memory*/
        sdata[threadIdx.x] = data[idx];
        __syncthreads();

        for (int stride = 1; stride < blockDim.x; stride *= 2) {
            int index = 2 * stride * index;
            if (index < blockDim.x) {
                int lhs = sdata[threadIdx.x];
                int rhs = sdata[threadIdx.x + stride];
                sdata[threadIdx.x] = lhs < rhs ? rhs : lhs;
            }
            __syncthreads();
        }
    }
    if (idx == 0) data[0] = sdata[0];
}


int reduction_CPU() {
    for (int i = 1; i < DATA_SIZE; i *= 2) {
        for (int j = 0; j < DATA_SIZE; j += i*2) {
            if (j >= DATA_SIZE || j + i >= DATA_SIZE)
                break;
            if (numbers_CPU[j + i] > numbers_CPU[j])
                numbers_CPU[j] = numbers_CPU[j + i];
        }
    }
    //printf("CPU: %d\n", numbers_CPU[0]);
    return numbers_CPU[0];
}


void init_matrix() {
    srand((unsigned)time(NULL));
    int random_num;
    for (int i = 0; i < DATA_SIZE; i++) {
        random_num = (int)(((double)rand() / RAND_MAX) * 300.0f);
        numbers_CPU[i] = numbers[i] = random_num;
    }
}

cudaError_t reduction_GPU()
{
    cudaError_t cudaStatus = cudaSetDevice(0);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
        return cudaStatus;
    }

    int max_value;
    int* d_numbers;
    size_t size = DATA_SIZE * sizeof(int);
    CUDA_CALL(cudaMalloc(&d_numbers, size));
    
    dim3 dimBlock(BLOCK_SIZE);
    dim3 dimGrid(DATA_SIZE / dimBlock.x + 1);


    // with path-divergence
    CUDA_CALL(cudaMemcpy(d_numbers, numbers, size, cudaMemcpyHostToDevice));

    CHECK_TIME_INIT_GPU();
    CHECK_TIME_START_GPU();

    
    reduction_GPU << < dimGrid, dimBlock >> > (d_numbers, DATA_SIZE);


    CUDA_CALL(cudaGetLastError());
    CUDA_CALL(cudaDeviceSynchronize());
    
    CHECK_TIME_END_GPU(device_time);
    CHECK_TIME_DEST_GPU();
    
    CUDA_CALL(cudaMemcpy(&max_value, d_numbers, sizeof(int), cudaMemcpyDeviceToHost));
    //printf("\nGPU %d\n", max_value);
    printf("GPU time = %.6f\n", device_time);


    // without path-divergence
    CUDA_CALL(cudaMemcpy(d_numbers, numbers, size, cudaMemcpyHostToDevice));

    CHECK_TIME_INIT_GPU();
    CHECK_TIME_START_GPU();

    reduction_no_diverge_GPU << < dimGrid, dimBlock >> > (d_numbers, DATA_SIZE);

    CUDA_CALL(cudaGetLastError());
    CUDA_CALL(cudaDeviceSynchronize());

    CHECK_TIME_END_GPU(device_time);
    CHECK_TIME_DEST_GPU();

    CUDA_CALL(cudaMemcpy(&max_value, d_numbers, sizeof(int), cudaMemcpyDeviceToHost));
    //printf("\nGPU no divergence: %d\n", max_value);
    printf("GPU no divergence time = %.6f\n", device_time);

    // shared without path-divergence
    CUDA_CALL(cudaMemcpy(d_numbers, numbers, size, cudaMemcpyHostToDevice));

    CHECK_TIME_INIT_GPU();
    CHECK_TIME_START_GPU();

    reduction_shared_no_diverge_GPU << < dimGrid, dimBlock >> > (d_numbers, DATA_SIZE);

    CUDA_CALL(cudaGetLastError());
    CUDA_CALL(cudaDeviceSynchronize());

    CHECK_TIME_END_GPU(device_time);
    CHECK_TIME_DEST_GPU();

    CUDA_CALL(cudaMemcpy(&max_value, d_numbers, sizeof(int), cudaMemcpyDeviceToHost));
    //printf("\nGPU shared no divergence: %d\n", max_value);
    printf("GPU shared no divergence time = %.6f\n", device_time);


    cudaFree(d_numbers);

    
    return cudaStatus;
}

int main()
{
    init_matrix();
    CHECK_TIME_START;
    reduction_CPU();
    CHECK_TIME_END(compute_time);
    printf("CPU time = %.6f\n", compute_time);
    reduction_GPU();

    return 0;
}
