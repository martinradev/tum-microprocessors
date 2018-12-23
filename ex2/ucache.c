#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <emmintrin.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

// Call to get the number of elapsed cycles since start of processor.
static u64 __rdtscp_start(void)
{
    u32 high, low;
    asm volatile(".intel_syntax noprefix\n\t"
                 "mfence\n\t" /* is this necessary here? */
                 "cpuid\n\t" /* is this necessary here? */
                 "rdtscp\n\t"
                 "mov %0, edx\n\t"
                 "mov %1, eax\n\t"
                 ".att_syntax prefix\n\t"
        : "=r" (high), "=r" (low)
        :
        : "%rax", "%rbx", "%rdx", "%rcx");
    return (((u64)high) << 32U) | (u64)low;
}

// Call to get the number of elapsed cycles since start of processor.
static u64 __rdtscp_end(void)
{
    u32 high, low;
    asm volatile(".intel_syntax noprefix\n\t"
                 "rdtscp\n\t"
                 "mov %0, edx\n\t"
                 "mov %1, eax\n\t"
                 "cpuid\n\t"
                 "lfence\n\t"
                 ".att_syntax prefix\n\t"
        : "=r" (high), "=r" (low)
        :
        : "%rax", "%rbx", "%rdx", "%rcx");
    return (((u64)high) << 32U) | (u64)low;
}

// Iterate over the array using different stride lengths.
// The expectation is that for any two strides which are smaller than the cache line size
// we should expect a significant difference in elapsed cycles/byte as we have to issue more reads per read cache line.
// Whenever the stride is bigger than the cache line, we are always doing one issue per cache line, so there will be no
// difference in cycles / byte.
static void generateCacheLineData(void)
{
    const size_t buffer_size = 128U * 1024U * 1024U;
    u8 *buffer = NULL;
    size_t step;
    size_t i;
    size_t j;
    u8 tmp;
    // TODO: we could use mmap here too.
    buffer = (u8*)malloc(buffer_size);
    for (i = 0U; i < buffer_size; ++i)
    {
        buffer[i] = (i)&0xFFU;
    }
    FILE *inp = fopen("cache_line_data.txt", "w+");

    for (step = 1U; step < 128U; ++step)
    { 
        const size_t kNumIterations = 256;
        u64 avg = 0U;
        for (i = 0U; i < kNumIterations; ++i)
        {
            u8 *ptr = buffer;
            u64 time1 = __rdtscp_start();
            for (j = 0U; j < buffer_size; j += step, ptr += step)
            {
                // Read a dummy value. The compiler cannot optimize-out this inlined assembly.
                asm volatile(".intel_syntax noprefix\n\t"
                             "mov %0, BYTE PTR [%1]\n\t"
                             ".att_syntax prefix\n\t"
                             : "=r"(tmp)
                             : "r"(ptr));
            }
            u64 time2 = __rdtscp_end();
            avg += (time2 - time1);
        }
        avg /= kNumIterations;
        u64 cyclesPerAccess = step * avg / buffer_size;
        fprintf(inp, "%u %llu\n", (u32)step, (unsigned long long)(cyclesPerAccess));
    }

    fclose(inp);
    free(buffer);
}

// Use this 64-byte sized node for a static linked list.
// Generating the random sequence for accesses on the go didn't prove to be effective.
// It seemed like the hardware prefetcher could always read data ahead of time.
typedef struct BlockDecl
{
    struct BlockDecl *next;
    u8 value;
    u8 padding[55U];
} Block;

void generateRandomSequence(Block *blocks, size_t numBlocks)
{
    size_t prevAddr = 0U;
    for (size_t i = 0U; i < numBlocks; ++i)
    {
        size_t nextAddr = (prevAddr + numBlocks - 1U) % numBlocks;
        blocks[prevAddr].next = &blocks[nextAddr];
        prevAddr = nextAddr;
    }
}

static void generateCacheSizeData(void)
{
    // This should be 500kb.
    // Here we use mmap to hopefully guarantee that the pages are locked(pinned).
    // Ideally we should be using big pages (HUGEPAGE_TLB) but that doesn't seem to be supported on my linux.
    // The L1-TLB can hold only info for 160 kb of data!
    const size_t kMaxBlocks = 5U*1024U;
    Block *blocks = (Block*)mmap(NULL, sizeof(Block) * kMaxBlocks, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_LOCKED|MAP_POPULATE|MAP_ANONYMOUS, -1, 0U);
    if (blocks == MAP_FAILED)
    {
        printf("Failed to allocate: %s\n", strerror(errno));
        return;
    }

    FILE *inp = fopen("cache_line_data.txt", "w+");
    for (size_t j = 64U; j < kMaxBlocks; j += 64U)
    {
        generateRandomSequence(blocks, j);
        Block *b = NULL;
        u64 avg = 0U;
        size_t kMaxAccesses = 2*1024U*1024U;
        size_t numSamples = 8U;
        for (size_t q = 0U; q < numSamples; ++q)
        {
            b = &blocks[0];
            // Make sure that the caches are flushed and invalidated before benchmarking.
            // This quite costly!
            for (size_t i = 0U; i < kMaxAccesses; ++i)
            {
                Block *next = b->next;
                _mm_clflush(next);
                b = next;
            }
            // Measure.
            u64 t1 = __rdtscp_start();
            b = &blocks[0];
            for (size_t i = 0U; i < kMaxAccesses; ++i)
            {
                b->value = (u8)(i&0xFFU);
                b = b->next;
            }
            u64 t2 = __rdtscp_end();
            u64 delta = t2-t1;
            avg += delta;
        }
        avg /= numSamples;
        // No need to divide by the number of accesses because the number of accesses are always the same!
        fprintf(inp, "%llu %llu\n", (unsigned long long)j*sizeof(Block), (unsigned long long)avg);
    }
    fclose(inp);
}

int main(void)
{
    generateCacheLineData();
    generateCacheSizeData();
    return 0;
}