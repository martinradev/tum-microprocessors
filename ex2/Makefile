all: cache asm

cache: ucache.cpp
	g++ ucache.cpp -O2 -Werror -std=c++11 -o cache

asm: ucache.cpp
	gcc ucache.cpp -O2 -Werror -S -masm=intel --std=c++11 -o cache_asm.s

gpu: gpu.cu
	nvcc gpu.cu -O2 -o gpu

