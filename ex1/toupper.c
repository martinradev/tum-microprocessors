#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <stdint.h>
#include <xmmintrin.h>
#include <smmintrin.h>
#include "options.h"
#include <assert.h>


int debug = 0;
double *results;
double *ratios;
unsigned long   *sizes;

int no_sz = 1, no_ratio =1, no_version=1;



static inline
double gettime(void) {
    // to be implemented
    struct timeval tv = {0};
    int res = gettimeofday(&tv, NULL);
    (void)res;
    return ((double)tv.tv_sec + (double)tv.tv_usec / 1000000.0);
}


static void toupper_simple(char * text) {
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

static inline uintptr_t UpAlignAddr(uintptr_t v, uint32_t alignment)
{
    return (v + alignment - 1U) & ~((uintptr_t)alignment - 1U);
}

static void toupper_optimised(char * text) {
    if (!text)
    {
        return;
    }
    //assert(UpAlignAddr(13U, 16U) == 16U);
    //assert(UpAlignAddr(33U, 16U) == 48U);

    // Align pointer to be divisible by 16 to guarantee that we can use aligned vector loads.
    uintptr_t textPtrAsUint = (uintptr_t)(text);
    uintptr_t textPtrAsUintAligned = UpAlignAddr(textPtrAsUint, 16U);
    char *textEnd = (char*)(textPtrAsUintAligned);
    char c;
    while (text != textEnd && (c = *text))
    {
        int cond = (c >= 'a' && c <= 'z');
        c = c - cond * 0x20U;
        *text = c;
        ++text;
    }

    // Now go over the array using vector instructions
    __m128i *head = (__m128i*)(textPtrAsUintAligned);
    const __m128i lowerBound = _mm_set1_epi8('a'-1);
    const __m128i upperBound = _mm_set1_epi8('z'+1);
    const __m128i ending = _mm_set1_epi8('\0');
    const __m128i sub = _mm_set1_epi8(0x20U);
    while(1)
    {
        __m128i v = *head;
        int canEnd = _mm_cmpistrs(v, v, _SIDD_UBYTE_OPS);
        if (canEnd)
        {
            // Contains a \0 character.
            break;
        }
        __m128i gt = _mm_cmpgt_epi8(v, lowerBound);
        __m128i lt = _mm_cmplt_epi8(v, upperBound);
        __m128i cond = _mm_and_si128(lt, gt);
        __m128i toLower = _mm_sub_epi8(v, sub);
        toLower = _mm_blendv_epi8(v, toLower, cond);
        *head = toLower;
        ++head;
    }

    // Handle suffix
    text = (char*)head;
    while ((c = *text))
    {
        int cond = (c >= 'a' && c <= 'z');
        c = c - cond * 0x20U;
        *text = c;
        ++text;
    }
}


/*****************************************************************/


// align at 16byte boundaries
void* mymalloc(unsigned long int size)
{
     void* addr = malloc(size+32);
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

char * init(unsigned long int sz, int ratio){
    int i=0;
    char *text = (char *) mymalloc(sz+1);
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

typedef void (*toupperfunc)(char *text);

void run_toupper(int size, int ratio, int version, toupperfunc f, const char* name)
{
   double start, stop;
		int index;

		index =  ratio;
		index += size*no_ratio;
		index += version*no_sz*no_ratio;

    char *text = init(sizes[size], ratios[ratio]);


    if(debug) printf("Before: %.40s...\n",text);

    start = gettime();
    (*f)(text);
    stop = gettime();
    results[index] = stop-start;

    if(debug) printf("After:  %.40s...\n",text);
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
	printf("%s\n", OPTS);

	for(j=0;j<no_sz;j++){
		for(k=0;k<no_ratio;k++){
			printf("Size: %ld \tRatio: %f \tRunning time:", sizes[j], ratios[k]);
			for(i=0;i<no_version;i++){
				index =  k;
				index += j*no_ratio;
				index += i*no_sz*no_ratio;
				printf("\t%s: %f", toupperversion[i].name, results[index]);
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

		}
    for(v=0; toupperversion[v].func !=0; v++)
		no_version=v+1;
		if(0==max_sz)  no_sz =1;
		else no_sz = (max_sz-min_sz)/step_sz+1;
		if(0==max_ratio)  no_ratio =1;
		else no_ratio = (max_ratio-min_ratio)/step_ratio+1;
		no_exp = v*no_sz*no_ratio;
		results = (double *)malloc(sizeof(double[no_exp]));
		ratios = (double *)malloc(sizeof(double[no_ratio]));
		sizes = (long *)malloc(sizeof(long[no_sz]));

		for(i=0;i<no_sz;i++)
			sizes[i] = min_sz + i*step_sz;
		for(i=0;i<no_ratio;i++)
			ratios[i] = min_ratio + i*step_ratio;

		for(i=0;i<no_sz;i++)
			for(j=0;j<no_ratio;j++)
				run(i,j);

		printresults();
    return 0;
}
