#include <pthread.h>
#include <arm_neon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>

int debug = 0;
float *results;
float *ratios;
unsigned long   *sizes;

int no_sz = 1, no_ratio =1, no_version=1;
unsigned NumThreads=1;

// This is not really the way to be done.
// Rather, one should use the arch-specific high performance counters!
static inline float gettime(void) {
    struct timespec ts = {0};
    int err = clock_gettime(CLOCK_MONOTONIC, &ts);
    (void)err;
    return (float)ts.tv_sec + (float)ts.tv_nsec / 1000000000.0f;
}


static void toupper_simple(char * text, size_t n) {
    if (!text)
    {
        return;
    }
    char c;
    while((c = *text))
    {
        int cond = (c >= 'a' && c <= 'z');
        c = c - cond*0x20U;
        *text = c;
        ++text;
    }
}

struct ThreadData
{
	size_t elemsPerThread;
	char* start;
};

static void* toUpperPthread(void* threadArg)
{
	int counter = 0;
	printf("%d\n", ++counter);

	// Convert argument to data type
	struct ThreadData *data = (struct ThreadData *) threadArg;

	// Get end point of the partition
	int8x8_t* start = data->start;

	printf("%d\n", ++counter);

	// Perform to upper on thread's partition
	uint8x8_t as;
	uint8_t temp = 97;
	as = vmov_n_u8 (temp);
	uint8x8_t zs = vmov_n_u8 ((uint8_t)'z');
	uint8x8_t dif = vmov_n_u8 ((uint8_t)0x20);

	printf("%d\n", ++counter);

	size_t i = 0;
	size_t sz = data->elemsPerThread / 16;
	while(i < sz)
	{
		uint8x8_t c = *(uint8x8_t*)(start + i);
		uint8x8_t biggerCond = vcge_u8(c, as);
		uint8x8_t lessCond = vcle_u8(c, as);
		uint8x8_t cond = vand_u8(biggerCond, lessCond);
		c = vmls_u8(c, cond, dif);
		*(uint8x8_t*)(start+i) = c;
		++i;
    }

    printf("%d\n", ++counter);

    return NULL;
}

static void toupper_optimised(char * text, size_t n)
{
	if (!text)
	{
		return;
	}

	int counter = 0;

	// Set up for multi-threads
	pthread_t threads[64];
	struct ThreadData threadDataArray[64];

	size_t elemsPerThread = n / NumThreads;

	printf("%d\n", ++counter);

	size_t i = 0;
	for (i = 0; i < NumThreads; ++i)
	{
		threadDataArray[i].start = text + i * elemsPerThread;
		threadDataArray[i].elemsPerThread = elemsPerThread;
	}

	printf("%d\n", ++counter);

	// Do the multi-thread
	int rc;	long t;
	for (t = 0; t < NumThreads; t++)
	{
		rc = pthread_create(&threads[t], NULL,
				toUpperPthread, (void *) &threadDataArray[t]);
	}

	// toUpperPthread((void*)threadDataArray);

	printf("%d\n", ++counter);

	// Sync threads
	void* status;
	for (t = 0; t < NumThreads; ++t)
	{
		rc = pthread_join(threads[t], &status);

   		if (rc)
   		{
        	printf("ERROR; return code from pthread_join() is %d\n", rc);
          	exit(-1);
        }
	}

	// Handle the suffix
	size_t suffixStart = elemsPerThread * NumThreads;
	size_t suffixLen = n % NumThreads;

	for ( i = 0; i < suffixLen; i++ )
	{
		char c = text[suffixStart + i];
		int cond = (c >= 'a' && c <= 'z');
		c = c - cond * 0x20U;
		text[suffixStart + i] = c;
	}
}


/*****************************************************************/


// align at 16byte boundaries
void* mymalloc(unsigned long int size, void **original)
{
     void* addr = malloc(size+32);
     *original = addr;
     return (void*)((unsigned long int)addr /16*16+16);
}

char createChar(int ratio){
	char isLower = rand()%100;

	// upper case=0, lower case=1
	if(isLower < ratio)
		isLower =0;
	else
		isLower = 1;

	char letter = rand()%26+1; // a,A=1; b,B=2; ...

	return 0x40 + isLower*0x20 + letter;

}

char * init(unsigned long int sz, int ratio, char **original){
    int i=0;
    char *text = (char *) mymalloc(sz+1, (void **) original);
    srand(1);// ensures that all strings are identical
    for(i=0;i<sz;i++){
			char c = createChar(ratio);
			text[i]=c;
	  }
    text[i] = '\0';
    return text;
}



/*
 * ******************* Run the different versions **************
 */

typedef void (*toupperfunc)(char *text, size_t n);

void run_toupper(int size, int ratio, int version, toupperfunc f, const char* name)
{
   float start, stop;
		int index;

		index =  ratio;
		index += size*no_ratio;
		index += version*no_sz*no_ratio;

    char *textOriginalPtr;
    char *text = init(sizes[size], ratios[ratio], &textOriginalPtr);

    if(debug) printf("Before: %.40s...\n",text);

    start = gettime();
    (*f)(text, sizes[size]);
    stop = gettime();
    results[index] = stop-start;

    if(debug) printf("After:  %.40s...\n", text);
    free(textOriginalPtr);
}

struct _toupperversion {
    const char* name;
    toupperfunc func;
} toupperversion[] = {
    { "simple",    toupper_simple },
    { "optimised", toupper_optimised },
    { 0,0 }
};


void run(int size, int ratio)
{
	int v;
	for(v=0; toupperversion[v].func !=0; v++) {
		run_toupper(size, ratio, v, toupperversion[v].func, toupperversion[v].name);
	}

}

void printresults(){
	int i,j,k,index;
//	printf("%s\n", OPTS);

	for(j=0;j<no_sz;j++){
		for(k=0;k<no_ratio;k++){
			printf("Size: %ld \tRatio: %lf \tRunning time:", sizes[j], ratios[k]);
			for(i=0;i<no_version;i++){
				index =  k;
				index += j*no_ratio;
				index += i*no_sz*no_ratio;
				printf("\t%s: %lf", toupperversion[i].name, results[index]);
			}
			printf("\n");
		}
	}
}

int main(int argc, char* argv[])
{
    unsigned long int min_sz=800000, max_sz = 0, step_sz = 10000;
		int min_ratio=50, max_ratio = 0, step_ratio = 1;
		int arg,i,j,v;
		int no_exp;

		for(arg = 1;arg<argc;arg++){
			if(0==strcmp("-d",argv[arg])){
				debug = 1;
			}
			if(0==strcmp("-l",argv[arg])){
					min_sz = atoi(argv[arg+1]);
					if(arg+2>=argc) break;
					if(0==strcmp("-r",argv[arg+2])) break;
					if(0==strcmp("-d",argv[arg+2])) break;
					max_sz = atoi(argv[arg+2]);
					step_sz = atoi(argv[arg+3]);
			}
			if(0==strcmp("-r",argv[arg])){
					min_ratio = atoi(argv[arg+1]);
					if(arg+2>=argc) break;
					if(0==strcmp("-l",argv[arg+2])) break;
					if(0==strcmp("-d",argv[arg+2])) break;
					max_ratio = atoi(argv[arg+2]);
					step_ratio = atoi(argv[arg+3]);
			}
            if (0==strcmp("-t",argv[arg])){
                NumThreads = atoi(argv[arg+1]);
            }

		}
    for(v=0; toupperversion[v].func !=0; v++)
		no_version=v+1;

		if(0==max_sz)  no_sz =1;
		else no_sz = (max_sz-min_sz)/step_sz+1;

		if(0==max_ratio)  no_ratio =1;
		else no_ratio = (max_ratio-min_ratio)/step_ratio+1;

		no_exp = v*no_sz*no_ratio;
		results = (float *)malloc(sizeof(float[no_exp]));
		ratios = (float *)malloc(sizeof(float[no_ratio]));
		sizes = (long *)malloc(sizeof(long[no_sz]));

		for(i=0;i<no_sz;i++)
			sizes[i] = min_sz + i*step_sz;

		for(i=0;i<no_ratio;i++)
		{
			ratios[i] = min_ratio + i * step_ratio;
		}

		for(i=0;i<no_sz;i++)
			for(j=0;j<no_ratio;j++)
				run(i,j);

		printresults();

    if (debug)
    {
        char *originalU;
        char *u = init(sizes[0], 0, &originalU);
        char *v = (char*)malloc(sizes[0]);
        memcpy(v, u, sizes[0]);
        toupper_simple(u, sizes[0]);
        toupper_optimised(v, sizes[0]);
        printf("Verify\n");
        unsigned long int j;
        for (j = 0; j < sizes[0]; ++j)
        {
            if (u[j] != v[j])
            {
                printf("Error at %lu. Expected %c, Result %c\n", j, u[j], v[j]);
            }
        }
    }
    return 0;
}
