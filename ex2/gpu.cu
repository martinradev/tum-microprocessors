#include <stdio.h>
#include <inttypes.h>
#include <cuda.h>
#include <stdlib.h>
#include <string.h>

__global__ void warmup(uint8_t *arr, size_t n)
{
    uint32_t tid = threadIdx.x + blockIdx.x * blockDim.x;
    arr[tid] = 1U;
}

__global__ void test(uint8_t *arr, size_t n, size_t stride, uint64_t *timer)
{
    size_t i;
    size_t j;
    uint64_t sumDeltas = 0;
    uint64_t numReads = 0;
    for (j = 0U; j < 1U; ++j)
    {
        for (i = 0U; i < n; i += stride)
        {
            uint64_t t1 = clock64();
            arr[i] += 1U;
            uint64_t t2 = clock64();
            sumDeltas += (t2-t1);
            ++numReads;
        }
    }
    sumDeltas /= numReads;
    *timer = sumDeltas;
}

int main(void)
{
    float ms;
    size_t arraySize = 1024U * 512U;
    uint8_t *gpuBuffer = NULL;
    cudaError_t err = cudaMalloc(&gpuBuffer, arraySize);
    if (err != cudaSuccess)
    {
        printf("Failed to alloc gpu mem\n");
        return -1;
    }
    uint64_t *gpuClock;
    err = cudaMalloc(&gpuClock, sizeof(*gpuClock));
    if (err != cudaSuccess)
    {
        printf("Failed to alloc clock timer\n");
        return -1;
    }
    uint64_t gpuTimerOnCpu = 0U;
    cudaMemcpy(gpuClock, &gpuTimerOnCpu, sizeof(*gpuClock), cudaMemcpyHostToDevice);

    cudaEvent_t startEvent, endEvent;
    err = cudaEventCreate(&startEvent);
    if (err != cudaSuccess)
    {
        printf("Failed to create start event\n");
    }
    err = cudaEventCreate(&endEvent);
    if (err != cudaSuccess)
    {
        printf("Failed to create end event\n");
    }

    {
        // warm-up gpu buffer
        size_t threadBlockSize = 512U;
        size_t numBlocks = (arraySize + threadBlockSize - 1U) / threadBlockSize;
        printf("Launch warm-up kernel. Num blocks: %lu, Block size: %lu\n", numBlocks, threadBlockSize);
        warmup<<<dim3(numBlocks,1,1), dim3(threadBlockSize,1,1)>>>(gpuBuffer, arraySize); 
    }

    printf("Warm up gpu cache line kernel\n");
    err = cudaEventRecord(startEvent);
    test<<<dim3(1,1,1), dim3(1,1,1)>>>(gpuBuffer, arraySize, 1U, gpuClock);
    err = cudaEventRecord(endEvent);
    err = cudaEventSynchronize(endEvent);
    err = cudaEventElapsedTime(&ms, startEvent, endEvent);
    printf("Warm-up took %f\n", ms);

    FILE *out = fopen("gpu_cache_line_size_data.txt", "w+");
    const size_t maxStrideSize = 2048U;
    for (size_t i = 1; i < maxStrideSize; ++i)
    {
        double totalms = .0f;
        for (size_t q = 0U; q < 64U; ++q)
        {
            test<<<dim3(1,1,1), dim3(1,1,1)>>>(gpuBuffer, arraySize, i, gpuClock);
            cudaMemcpy(&gpuTimerOnCpu, gpuClock, sizeof(*gpuClock), cudaMemcpyDeviceToHost);
            totalms += (double)gpuTimerOnCpu;
        }
        fprintf(out, "%lu %f\n", i, i*totalms/(arraySize * 64U));
        printf("Done %lu/%lu\n", i, maxStrideSize);
    }
    fclose(out);

    cudaEventDestroy(endEvent);
    cudaEventDestroy(startEvent);
    cudaFree(gpuBuffer);

    return 0;
}
