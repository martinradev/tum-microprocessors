all: cache asm 

cache: ucache.c
	g++ ucache.c -O2 -Werror -o cache

asm: ucache.c
	g++ ucache.c -O2 -Werror -S -masm=intel -o cache_asm.s

