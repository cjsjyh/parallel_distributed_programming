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
#define TILE_SIZE 32
#define DATA_SIZE (4096 * 4096)
#define TILE_DIM 32

TIMER_T compute_time = 0;
TIMER_T device_time = 0;

int N;
float matrixA[4096 * 4096];
float matrixB[4096 * 4096];
float matrixC[4096 * 4096];



__global__ void matrixMultiplicationKernel(float* A, float* B, float* C) {
    int Row = blockIdx.y * blockDim.y + threadIdx.y;
    int Col = blockIdx.x * blockDim.x + threadIdx.x;

    float tmpSum = 0;

    if (Row < 4096 && Col < 4096) {
        // each thread computes one element of the block sub-matrix
        for (int i = 0; i < 4096; i++) {
            tmpSum += A[Row * 4096 + i] * B[i * 4096 + Col];
        }
    }
    C[Row * 4096 + Col] = tmpSum;
}

__global__ void matrixMultiplicationSharedKernel(float* A, float* B, float* C)
{
    __shared__ float sA[TILE_SIZE][TILE_SIZE];   // Tile size of 32x32 
    __shared__ float sB[TILE_SIZE][TILE_SIZE];

    int Row = blockDim.y * blockIdx.y + threadIdx.y;
    int Col = blockDim.x * blockIdx.x + threadIdx.x;
    float Cvalue = 0.0;
    sA[threadIdx.y][threadIdx.x] = 0.0;
    sB[threadIdx.y][threadIdx.x] = 0.0;

    for (int k = 0; k < (((4096 - 1) / TILE_SIZE) + 1); k++)
    {
        if ((Row < 4096) && (threadIdx.x + (k * TILE_SIZE)) < 4096){
            sA[threadIdx.y][threadIdx.x] = A[(Row * 4096) + threadIdx.x + (k * TILE_SIZE)];
        }
        else{
            sA[threadIdx.y][threadIdx.x] = 0.0;
        }
        if (Col < 4096 && (threadIdx.y + k * TILE_SIZE) < 4096){
            sB[threadIdx.y][threadIdx.x] = B[(threadIdx.y + k * TILE_SIZE) * 4096 + Col];
        }
        else{
            sB[threadIdx.y][threadIdx.x] = 0.0;
        }
        __syncthreads();

        for (int j = 0; j < TILE_SIZE; ++j){
            Cvalue += sA[threadIdx.y][j] * sB[j][threadIdx.x];
        }
    }
    if (Row < 4096 && Col < 4096){
        C[Row * 4096 + Col] = Cvalue;
    }
}

void init_matrix() {
    srand((unsigned)time(NULL));
    int random_num;
    for (int i = 0; i < 4096; i++) {
        for (int j = 0; j < 4096; j++) {
            random_num = (int)(((double)rand() / RAND_MAX) * 40.0f);
            matrixA[i*4096 + j] = random_num;
        }
    }

    for (int i = 0; i < 4096; i++) {
        for (int j = 0; j < 4096; j++) {
            random_num = (int)(((double)rand() / RAND_MAX) * 40.0f);
            matrixB[i * 4096 + j] = random_num;
        }
    }
}

cudaError_t MatrixMult_GPU()
{
    cudaError_t cudaStatus = cudaSetDevice(0);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
        return cudaStatus;
    }

    float* GPU_matrixA, *GPU_matrixB, *GPU_matrixC;
    size_t size;
    size = DATA_SIZE * sizeof(int);
    CUDA_CALL(cudaMalloc(&GPU_matrixA, size));
    CUDA_CALL(cudaMalloc(&GPU_matrixB, size));
    CUDA_CALL(cudaMalloc(&GPU_matrixC, size));
    CUDA_CALL(cudaMemcpy(GPU_matrixA, matrixA, size, cudaMemcpyHostToDevice));
    CUDA_CALL(cudaMemcpy(GPU_matrixB, matrixB, size, cudaMemcpyHostToDevice));
    
    // Global memory
    CHECK_TIME_INIT_GPU();
    CHECK_TIME_START_GPU();

    dim3 dimBlock(BLOCK_SIZE);
    dim3 dimGrid(DATA_SIZE / dimBlock.x);
    matrixMultiplicationKernel << < dimGrid, dimBlock >> > (GPU_matrixA, GPU_matrixB, GPU_matrixC);

    CUDA_CALL(cudaGetLastError());
    CUDA_CALL(cudaDeviceSynchronize());
    
    CHECK_TIME_END_GPU(device_time);
    CHECK_TIME_DEST_GPU();
    
    CUDA_CALL(cudaMemcpy(matrixC, GPU_matrixC, size, cudaMemcpyDeviceToHost));
    printf("Global GPU time = %.6f\n", device_time);


    // Shared memory
    CHECK_TIME_INIT_GPU();
    CHECK_TIME_START_GPU();

    dim3 dimBlockShared(TILE_SIZE, TILE_SIZE);
    dim3 dimGridShared(DATA_SIZE / 4096 / dimBlock.x, DATA_SIZE / 4096 / dimBlock.x);
    matrixMultiplicationSharedKernel << < dimGridShared, dimBlockShared >> > (GPU_matrixA, GPU_matrixB, GPU_matrixC);

    CUDA_CALL(cudaGetLastError());
    CUDA_CALL(cudaDeviceSynchronize());

    CHECK_TIME_END_GPU(device_time);
    CHECK_TIME_DEST_GPU();

    CUDA_CALL(cudaMemcpy(matrixC, GPU_matrixC, size, cudaMemcpyDeviceToHost));
    printf("Shared GPU time = %.6f\n", device_time);


    cudaFree(GPU_matrixA);
    cudaFree(GPU_matrixB);
    cudaFree(GPU_matrixC);

    
    return cudaStatus;
}

int main()
{
    init_matrix();
    MatrixMult_GPU();

    return 0;
}
