all: cache asm

cache: ucache.c
	gcc ucache.c -O2 -Werror -o cache

asm: ucache.c
	gcc ucache.c -O2 -Werror -S -masm=intel -o cache_asm.s

gpu: gpu.cu
	nvcc gpu.cu -O2 -o gpu

