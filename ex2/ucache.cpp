#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <random>
#include <array>

#ifdef __linux__ 
#include <sys/mman.h>
#else
#include <windows.h>
#define MAP_FAILED (NULL)
#endif


void *mapMemory(size_t size, bool isExecutable, bool useBigPages)
{
#ifdef __linux__
	return mmap(NULL, size, PROT_READ|PROT_WRITE|(isExecutable ? PROT_EXEC : 0), MAP_PRIVATE|MAP_POPULATE|MAP_ANONYMOUS|(useBigPages?MAP_HUGETLB:0), -1, 0U);
#else
	void *ptr = VirtualAlloc(NULL, size, MEM_COMMIT|MEM_RESERVE|(useBigPages?MEM_LARGE_PAGES:0), (isExecutable?PAGE_EXECUTE_READWRITE:PAGE_READWRITE));
	return ptr;
#endif
}

void unmapMemory(void *ptr, size_t size)
{
#ifdef __linux__
	munmap(ptr, size);
#else 
	VirtualFree(ptr, size, MEM_RELEASE);
#endif
}

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

#define MMAX(a,b) ((a) < (b) ? (b) : (a)) 

// Call to get the number of elapsed cycles since start of processor.
static u64 __rdtscp_start(void)
{
    u32 high, low;
    asm volatile(".intel_syntax noprefix\n\t"
                 "cpuid\n\t" /* is this necessary here? */
                 "rdtsc\n\t"
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
    const size_t buffer_size = 1024U * 1024U * 4U;
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

    const size_t numSteps = 256;

    {
        // Warm-up
        const size_t kNumIterations = 1024U * 1024U * 8U;
            u64 time1 = __rdtscp_start();
            j = 0U;
            for (i = 0U; i < kNumIterations; ++i)
            {
                u64 ptr = (u64)(buffer + j);
                // Read a dummy value. The compiler cannot optimize-out this inlined assembly.
                asm volatile(".intel_syntax noprefix\n\t"
                             "mov %0, BYTE [%1]\n\t"
                             ".att_syntax prefix\n\t"
                             : "=r"(tmp)
                             : "r"(ptr));
                j = (j + step) % buffer_size;
            }
            u64 time2 = __rdtscp_end();
 
    }

    for (step = 1U; step < numSteps; step += 1U)
    { 
        const size_t kNumIterations = 1024U * 1024U * 8U;
            u64 time1 = __rdtscp_start();
            j = 0U;
            for (i = 0U; i < kNumIterations; ++i)
            {
                u64 ptr = (u64)(buffer + j);
                // Read a dummy value. The compiler cannot optimize-out this inlined assembly.
                asm volatile(".intel_syntax noprefix\n\t"
                             "mov %0, BYTE [%1]\n\t"
                             ".att_syntax prefix\n\t"
                             : "=r"(tmp)
                             : "r"(ptr));
                j = (j + step) % buffer_size;
            }
            u64 time2 = __rdtscp_end();
        double cyclesPerAccess = (double)(time2-time1) / kNumIterations;
        fprintf(inp, "%u %lf\n", (u32)step, cyclesPerAccess);
        printf("Done: %u/%u\n", (u32)step, (u32)numSteps);
    }

    fclose(inp);
    free(buffer);
}

// Use this 64-byte sized node for a static linked list. Make sure this struct is as big as a cache line.
// Generating the random sequence for accesses on the go didn't prove to be effective.
// It seemed like the hardware prefetcher could always read data ahead of time.
// In literature this is called "P-chasing"
typedef struct BlockDecl
{
    struct BlockDecl *next;
    u8 padding[56U];
} Block;

void generateRandomSequence(Block *blocks, size_t numBlocks)
{
    std::vector<size_t> seq(numBlocks-1);
    for (size_t i = 1; i < numBlocks; ++i)
    {
        seq[i-1] = i;
    }
    std::shuffle(seq.begin(), seq.end(), std::default_random_engine(111U));
    seq.push_back(0);
    size_t prevAddr = 0U;
    for (size_t i = 0U; i < numBlocks; ++i)
    {
        size_t nextAddr = seq[i];
        blocks[prevAddr].next = &blocks[nextAddr];
        prevAddr = nextAddr;
    }
}

static void generateInstructionCacheSizeData(void)
{
    const size_t kMaxBlocks = 256*1024;
    FILE *inp = fopen("icache_size_data.txt", "w+");
    Block *blocks = (Block*)mapMemory(sizeof(Block) * kMaxBlocks, true, false);
    if (blocks == (Block*)-1)
    {
        printf("Failed to alloc memory");
        return;
    }
    /*
        dec eax ; 0xFF 0xC8
        jne ... relative address to the block ... ; We know this because we are generating the code :) ; 0x0F 0x85 ... and 32-bit relative offset ....
        ret ; 0xC3
    */


    for (size_t j = 64U; j < kMaxBlocks; j += 64)
    {
        size_t prevAddr = 0U;
        std::vector<size_t> seq(j-1);
        for (size_t i = 1; i < j; ++i)
        {
            seq[i-1] = i;
        }
        std::shuffle(seq.begin(), seq.end(), std::default_random_engine(111U));
        seq.push_back(0);
        for (size_t i = 0; i < j; ++i)
        {
            size_t nextAddr = seq[i];
            u64 nextBlockVA = (u64)(&blocks[nextAddr]);
            u8 *block = (u8*)&blocks[prevAddr];
            // Determine relative offset to next block
            u64 offset = nextBlockVA - (u64)block;
            u32 offsetU32 = (u32)(offset & 0xFFFFFFFFU);
            u32 offsetU32Adj = offsetU32 - 8U; // 8 bytes since dec and jne take 8 bytes
            // dec eax
            block[0] = 0xFFU;
            block[1] = 0xC8U;
            // jne to next block
            block[2] = 0x0FU;
            block[3] = 0x85U;
            memcpy(&block[4], &offsetU32Adj, 4U);
            // ret
            block[8] = 0xC3U;
            prevAddr = nextAddr;
        }
        // Now add some code to go into the generated code
        u32 kMaxAccesses = kMaxBlocks;
        u64 t1 = __rdtscp_start();
        asm volatile(".intel_syntax noprefix\n\t"
                     "mov eax, %0\n\t"
                     "call %1\n\t"
                     ".att_syntax prefix\n\t"
                     : /* no output */
                     : "r"(kMaxAccesses), "r"((u64)blocks) 
                     : "eax", "flags");
        u64 t2 = __rdtscp_end();
        fprintf(inp, "%llu %llu\n", (unsigned long long)j*sizeof(Block), (unsigned long long)(t2-t1)/kMaxAccesses);
        printf("Done: %lu/%lu\n", j, kMaxBlocks);
    }
    fclose(inp);    
    unmapMemory(blocks, sizeof(Block) * kMaxBlocks);
}

static void generateCacheSizeData(const char *fileName, bool useBigPages)
{
    // This should be 500kb.
    // Here we use mmap to hopefully guarantee that the pages are locked(pinned).
    // Ideally we should be using big pages (HUGEPAGE_TLB) but that doesn't seem to be supported on my linux.
    // The L1-TLB can hold only info for 160 kb of data!
    const size_t kMaxBlocks = 12*1024U*1024U;
    Block *blocks = (Block*)mapMemory(sizeof(Block) * kMaxBlocks, false, useBigPages);
    if (blocks == MAP_FAILED)
    {
        printf("Failed to allocate: %s\n", strerror(errno));
        return;
    }

    FILE *inp = fopen(fileName, "w+");
    for (size_t j = 3U; j < kMaxBlocks; j = (j*15U)/11U)
    {
        generateRandomSequence(blocks, j);
        Block *b = NULL;
        u64 avg = 0U;
        size_t kMaxAccesses = kMaxBlocks;
        size_t numSamples = 4U;
        for (size_t q = 0U; q < numSamples; ++q)
        {
            // Measure.
            u64 t1 = __rdtscp_start();
            b = &blocks[0];
            for (size_t i = 0U; i < kMaxAccesses; ++i)
            {
                b = b->next;
            }
            u64 t2 = __rdtscp_end();
            u64 delta = t2-t1;
            avg += delta;
        }
        avg /= (numSamples*kMaxAccesses);
        // No need to divide by the number of accesses because the number of accesses are always the same!
        fprintf(inp, "%llu %llu\n", (unsigned long long)j*sizeof(Block), (unsigned long long)avg);
        printf("Done: %llu/%llu\n", (unsigned long long)j, (unsigned long long)kMaxBlocks);
        if (b == NULL)
        {
            // Add this as a dependency so that the compiler cannot optimize out the loop above
            exit(1);
        }
    }
    fclose(inp);
}

void printInfoFromCpuid(void)
{
    // Unfortunately, my cpu doesn't have support for the extended features, so I cannot
    // get the cache size through cpuid.
    // Also, it seems to be necessary to process the returned data.
    u32 eax = 0U;
    u32 ecx = 0U;
    u32 edx = 0U;
    u32 ebx = 0U;
    asm volatile(".intel_syntax noprefix\n\t"
                 "mov eax, 0x1\n\t"
                 "cpuid\n\t"
                 "mov %1, ebx\n\t"
                 ".att_syntax prefix\n\t"
                 : "=r"(eax), "=r"(ebx), "=r"(edx), "=r"(ecx)
                 : /* no input */
                 : "%rax", "%rdx", "%rbx");
    u8 clFlushSize = (u8)((ebx>>8U) & 0xFFU);
    printf("CLFLUSH size: %u\nCache line size: %u\n", clFlushSize, clFlushSize * 8U);
}

size_t log2int(size_t pow2number)
{
    return __builtin_ctz(pow2number);
}

void generateL1CacheAssociativityData(size_t cacheLineSize, size_t l1Size)
{
    size_t numCacheLinesToAlloc = 1024U * 1024U * 8U;
    Block *blocks = (Block*)mapMemory(sizeof(Block) * numCacheLinesToAlloc, false, false);
    FILE *out = fopen("l1_assoc_data.txt", "w+");
    if (blocks == (Block*)-1)
    {
        printf("Failed to alloc memory\n");
        return;
    }
    uintptr_t va = (uintptr_t)blocks;    
    size_t numLinesInCache = l1Size / cacheLineSize;
    for (size_t i = 2; i <= 32; i*=2)
    {
        size_t indexPow2 = numLinesInCache / (1U << log2int(i));        
        size_t setIndex = (va >> log2int(cacheLineSize)) & (indexPow2-1U);
        const size_t numIterations = 256U * 1024U * 1024U;
        u64 sample1 = 0U;
        {
            // Generate sequence.
            Block *cBlock = blocks;
            for (size_t j = 0U; j < i-1; ++j)
            {
                cBlock->next = cBlock + (indexPow2);
                cBlock = cBlock->next;
            }
            cBlock->next = blocks;
            cBlock = blocks;
            u64 t1 = __rdtscp_start();
            for (size_t j = 0U; j < numIterations; ++j)
            {
                cBlock = cBlock->next;
            }
            u64 t2 = __rdtscp_end();
            sample1 = t2-t1;
            if (cBlock == NULL)
            {
                // Just a dependency.
                exit(1);
            }
        }
        u64 sample2 = 0U;
        {
            // Generate sequence.
            Block *cBlock = blocks;
            for (size_t j = 0U; j < i*2-1; ++j)
            {
                cBlock->next = cBlock + (indexPow2);
                cBlock = cBlock->next;
            }
            cBlock->next = blocks;
            cBlock = blocks;
            u64 t1 = __rdtscp_start();
            for (size_t j = 0U; j < numIterations; ++j)
            {
                cBlock = cBlock->next;
            }
            u64 t2 = __rdtscp_end();
            sample2 = t2-t1;
            if (cBlock == NULL)
            {
                // Just a dependency.
                exit(1);
            }
        }

        fprintf(out, "%lu %lf %lu %lu\n", i, (double)sample2 / sample1, sample1, sample2);
    }
    fclose(out);
}

template<size_t T>
struct SizedBlock
{
    SizedBlock<T> *next;
    u8 padding[T-sizeof(SizedBlock*)];
};

template<size_t SZ>
void generateSizedRandomSequence(SizedBlock<SZ> *blocks, size_t numBlocks, size_t setIndexBits)
{
    std::vector<size_t> seq(numBlocks-1);
    for (size_t i = 0; i < numBlocks-1; ++i)
    {
        seq[i] = i+1;
    }
    std::shuffle(seq.begin(), seq.end(), std::default_random_engine(111U));
    size_t setIndex = 1U;
    SizedBlock<SZ> *prevBlock = blocks;
    for (size_t i = 0U; i < numBlocks - 1; ++i)
    {
        size_t nextAddr = seq[i];
        SizedBlock<64> *line = (SizedBlock<64>*)&blocks[nextAddr];    
        line += setIndex;
        SizedBlock<SZ> *nextBlock = (SizedBlock<SZ>*)(line);
        prevBlock->next = nextBlock;
        prevBlock = nextBlock;
        setIndex = (setIndex + 1U) % (1U << setIndexBits);
    }
    prevBlock->next = blocks;
    prevBlock = blocks;
    for (size_t i = 0U; i < numBlocks; ++i)
    {
        prevBlock = prevBlock->next;
    }
}

template<size_t PageSize>
void measureL1dtlb(size_t setIndexBits, const char *fileName, int tlbFlags)
{
    using PageBlock = SizedBlock<PageSize>;
    size_t numBlocksToAlloc = 2048;
    PageBlock *blocks = (PageBlock*)mapMemory(sizeof(PageBlock) * numBlocksToAlloc, false, tlbFlags);
    if (blocks == (PageBlock*)-1)
    {
        printf("Failed to alloc memory\n");
        return;
    }
    FILE *out = fopen(fileName, "w+");
    for (size_t sz = 516U; sz < 517U; ++sz)
    {
        generateSizedRandomSequence<PageSize>(blocks, sz, setIndexBits);
        const u64 numIterations = 8U * 1024U * 1024U;
        PageBlock *cBlock = blocks;
        for (u64 i = 0; i < numIterations; ++i)
        {
            cBlock = cBlock->next;
        }
        u64 t1 = __rdtscp_start();
        for (u64 i = 0; i < numIterations; ++i)
        {
            cBlock = cBlock->next;
        }
        u64 t2 = __rdtscp_end();
        fprintf(out, "%lu %lu\n", sz, (t2-t1) / numIterations);
        if (!cBlock)
        {
            exit(1);
        }
    }

    fclose(out);
}

template<size_t PageSize>
static void generateInstructionTLBSizeData(size_t setIndexBits, const size_t kMaxBlocks, const char *fileName, int tlbFlags)
{
    using PageBlock = SizedBlock<PageSize>;
    FILE *inp = fopen(fileName, "w+");
    PageBlock *blocks = (PageBlock*)mapMemory(sizeof(PageBlock) * kMaxBlocks, true, tlbFlags);
    if (blocks == (PageBlock*)-1)
    {
        printf("Failed to alloc memory");
        return;
    }
    /*
        dec eax ; 0xFF 0xC8
        jne ... relative address to the block ... ; We know this because we are generating the code :) ; 0x0F 0x85 ... and 32-bit relative offset ....
        ret ; 0xC3
    */


    for (size_t j = 2U; j < 256; ++j)
    {
        std::vector<size_t> seq(j-1);
        for (size_t i = 0; i < j-1; ++i)
        {
            seq[i] = i+1;
        }
        std::shuffle(seq.begin(), seq.end(), std::default_random_engine(111U));
        
        size_t setIndex = 1U;
        PageBlock *prevBlock = blocks;
        for (size_t i = 0; i < j-1; ++i)
        {
            size_t nextAddr = seq[i];
            SizedBlock<64> *line = (SizedBlock<64>*)&blocks[nextAddr];
            line += setIndex;
            PageBlock *nextBlock = (PageBlock*)line;

            u64 nextBlockVA = (u64)(nextBlock);
            u8 *block = (u8*)prevBlock;
            // Determine relative offset to next block
            u64 offset = nextBlockVA - (u64)block;
            u32 offsetU32 = (u32)(offset & 0xFFFFFFFFU);
            u32 offsetU32Adj = offsetU32 - 8U; // 8 bytes since dec and jne take 8 bytes
            // dec eax
            block[0] = 0xFFU;
            block[1] = 0xC8U;
            // jne to next block
            block[2] = 0x0FU;
            block[3] = 0x85U;
            memcpy(&block[4], &offsetU32Adj, 4U);
            // ret
            block[8] = 0xC3U;

            prevBlock = nextBlock;
            setIndex = (setIndex + 1U) % (1U << setIndexBits);
        }
        {
            u64 nextBlockVA = (u64)(blocks);
            u8 *block = (u8*)prevBlock;
            // Determine relative offset to next block
            u64 offset = nextBlockVA - (u64)block;
            u32 offsetU32 = (u32)(offset & 0xFFFFFFFFU);
            u32 offsetU32Adj = offsetU32 - 8U; // 8 bytes since dec and jne take 8 bytes
            // dec eax
            block[0] = 0xFFU;
            block[1] = 0xC8U;
            // jne to next block
            block[2] = 0x0FU;
            block[3] = 0x85U;
            memcpy(&block[4], &offsetU32Adj, 4U);
            // ret
            block[8] = 0xC3U;               
        }
        // Now add some code to go into the generated code
        u32 kMaxAccesses = 8 * 1024 * 1024;
        // Warm-up
        asm volatile(".intel_syntax noprefix\n\t"
                     "mov eax, %0\n\t"
                     "call %1\n\t"
                     ".att_syntax prefix\n\t"
                     : /* no output */
                     : "r"(kMaxAccesses), "r"((u64)blocks) 
                     : "eax", "flags");

        u64 t1 = __rdtscp_start();
        asm volatile(".intel_syntax noprefix\n\t"
                     "mov eax, %0\n\t"
                     "call %1\n\t"
                     ".att_syntax prefix\n\t"
                     : /* no output */
                     : "r"(kMaxAccesses), "r"((u64)blocks) 
                     : "eax", "flags");
        u64 t2 = __rdtscp_end();
        fprintf(inp, "%llu %llu\n", (unsigned long long)j, (unsigned long long)(t2-t1)/kMaxAccesses);
    }
    fclose(inp);    
    unmapMemory(blocks, sizeof(Block) * kMaxBlocks);
}

typedef enum RunTypeDecl
{
    RunAll,
    RunCacheLine,
    RunCacheSize,  
    RunCacheSize2mb,
    RunICacheSize,
    RunCpuidInfo,
    RunCpuL1Assoc,
    RunCpuL2Assoc,
    RunCpuL3Assoc,
    RunDTLB,
    RunDTLB2MB,
    RunITLB,
    RunITLB2MB,
} RunType;

int main(int argc, char *argv[])
{
    RunType rt = RunAll;
    for (size_t i = 1U; i < argc; ++i)
    {
        if (strcmp("-all", argv[i]) == 0)
        {
            rt = RunAll;
        }
        else if (strcmp("-line", argv[i]) == 0)
        {
            rt = RunCacheLine;
        }
        else if (strcmp("-size", argv[i]) == 0)
        {
            rt = RunCacheSize;
        }
        else if (strcmp("-size2mb", argv[i]) == 0)
        {
            rt = RunCacheSize2mb;
        }
        else if (strcmp("-isize", argv[i]) == 0)
        {
            rt = RunICacheSize;
        }
        else if (strcmp("-cpuid", argv[i]) == 0)
        {
            rt = RunCpuidInfo;
        }
        else if (strcmp("-l1assoc", argv[i]) == 0)
        {
            rt = RunCpuL1Assoc;
        }
        else if (strcmp("-l2assoc", argv[i]) == 0)
        {
            rt = RunCpuL2Assoc;
        }
        else if (strcmp("-l3assoc", argv[i]) == 0)
        {
            rt = RunCpuL3Assoc;
        }
        else if(strcmp("-dtlb", argv[i]) == 0)
        {
            rt = RunDTLB;
        }
        else if(strcmp("-itlb", argv[i]) == 0)
        {
            rt = RunITLB;
        }
        else if(strcmp("-dtlb2mb", argv[i]) == 0)
        {
            rt = RunDTLB2MB;
        }
        else if(strcmp("-itlb2mb", argv[i]) == 0)
        {
            rt = RunITLB2MB;
        }
        else
        {
            return -1;
        }
    }
    switch(rt)
    {
    case RunCacheLine:
        generateCacheLineData();
        break;
    case RunCacheSize:
        generateCacheSizeData("cache_size_data.txt", false);
        break;
    case RunCacheSize2mb:
        generateCacheSizeData("cache_size_data_2mb.txt", true);
        break;
    case RunCpuidInfo:
        printInfoFromCpuid();
        break;
    case RunCpuL1Assoc:
        generateL1CacheAssociativityData(64U /* line size */, 32U * 1024U /*l1 cache size*/);
        break;
    case RunCpuL2Assoc:
        break;
    case RunCpuL3Assoc:
        //generateCacheAssociativityData(64U, 12U * 1024U * 1024U);
        break;
    case RunICacheSize:
        generateInstructionCacheSizeData();
        break;
    case RunDTLB:    
        // This has to be changed. It's hardcoded for Sandy Bridge :)
        measureL1dtlb<4096U>(6U, "l1_dtlb_size.txt", 0);
        break;
    case RunDTLB2MB:
        measureL1dtlb<1024U*1024U*2U>(6U, "l1_dtlb2mb_size.txt", true);
        break;
    case RunITLB:
        generateInstructionTLBSizeData<4096U>(6U, 256U*1024U, "itlb_size_data.txt", 0);
        break;
    case RunITLB2MB:
        generateInstructionTLBSizeData<2U*1024U*1024U>(6U, 1024U, "itlb2mb_size_data.txt", true);
        break;
    default:
        break;
    }
    return 0;
}
