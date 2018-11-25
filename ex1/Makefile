.PHONY:exe 

PROGRAM = toupper
SHELL := /bin/bash

sources = $(PROGRAM).c

VARIANTS = $(PROGRAM)_variant1
VARIANTS += $(PROGRAM)_variant2 
VARIANTS += $(PROGRAM)_variant3 
VARIANTS += $(PROGRAM)_variant4 


ICC = icc -fsource-asm -fno-alias -lm $(sources)
GCC = gcc -lm -march=native $(sources)
$(PROGRAM)_variant1 = $(GCC) -O0 
$(PROGRAM)_variant2 = $(GCC) -O1
$(PROGRAM)_variant3 = $(GCC) -O2
$(PROGRAM)_variant4 = $(GCC) -O3

PARAMS = 80000

#############################################################################################


exe: .autoclean $(VARIANTS) Makefile
	if test -a ./$(PROGRAM)_variant1; then ./$(PROGRAM)_variant1 $(PARAMS) > results1;else rm -f results1; fi
	if test -a ./$(PROGRAM)_variant2; then ./$(PROGRAM)_variant2 $(PARAMS) > results2;else rm -f results2;fi
	if test -a ./$(PROGRAM)_variant3; then ./$(PROGRAM)_variant3 $(PARAMS) > results3;else rm -f results3;fi
	if test -a ./$(PROGRAM)_variant4; then ./$(PROGRAM)_variant4 $(PARAMS) > results4;else rm -f results4;fi
	if test -a ./$(PROGRAM)_variant5; then ./$(PROGRAM)_variant5 $(PARAMS) > results5;else rm -f results5;fi
	if test -a ./$(PROGRAM)_variant6; then ./$(PROGRAM)_variant6 $(PARAMS) > results6;else rm -f results6;fi
	if test -a ./$(PROGRAM)_variant7; then ./$(PROGRAM)_variant7 $(PARAMS) > results7;else rm -f results7;fi
	if test -a ./$(PROGRAM)_variant8; then ./$(PROGRAM)_variant8 $(PARAMS) > results8;else rm -f results8;fi
	if test -a ./$(PROGRAM)_variant9; then ./$(PROGRAM)_variant9 $(PARAMS) > results9;else rm -f results9;fi
	if test -a ./results1; then cat results1 > results; else rm -f results; fi
	if test -a ./results2; then cat results2 >> results; fi
	if test -a ./results3; then cat results3 >> results; fi
	if test -a ./results4; then cat results4 >> results; fi
	if test -a ./results5; then cat results5 >> results; fi
	if test -a ./results6; then cat results6 >> results; fi
	if test -a ./results7; then cat results7 >> results; fi
	if test -a ./results8; then cat results8 >> results; fi
	if test -a ./results9; then cat results9 >> results; fi
	cat results;
	if test -a options.h; then rm options.h;fi

$(PROGRAM)_variant1: $(sources) Makefile
	echo '#define OPTS "VARIANT1: $($(PROGRAM)_variant1)"' > options.h 
	$($(PROGRAM)_variant1) -S -o $(PROGRAM)_variant1.s
	$($(PROGRAM)_variant1) -o $(PROGRAM)_variant1
$(PROGRAM)_variant2: $(sources) Makefile
	echo '#define OPTS "VARIANT2: $($(PROGRAM)_variant2)"' > options.h 
	$($(PROGRAM)_variant2) -S -o $(PROGRAM)_variant2.s
	$($(PROGRAM)_variant2) -o $(PROGRAM)_variant2
$(PROGRAM)_variant3: $(sources) Makefile
	echo '#define OPTS "VARIANT3: $($(PROGRAM)_variant3)"' > options.h 
	$($(PROGRAM)_variant3) -S -o $(PROGRAM)_variant3.s
	$($(PROGRAM)_variant3) -o $(PROGRAM)_variant3
$(PROGRAM)_variant4: $(sources) Makefile
	echo '#define OPTS "VARIANT4: $($(PROGRAM)_variant4)"' > options.h 
	$($(PROGRAM)_variant4) -S -o $(PROGRAM)_variant4.s
	$($(PROGRAM)_variant4) -o $(PROGRAM)_variant4
$(PROGRAM)_variant5: $(sources) Makefile
	echo '#define OPTS "VARIANT5: $($(PROGRAM)_variant5)"' > options.h 
	$($(PROGRAM)_variant5) -S  -o $(PROGRAM)_variant5.s
	$($(PROGRAM)_variant5) -o $(PROGRAM)_variant5
$(PROGRAM)_variant6: $(sources) Makefile
	echo '#define OPTS "VARIANT6: $($(PROGRAM)_variant6)"' > options.h 
	$($(PROGRAM)_variant6) -S -o $(PROGRAM)_variant6.s
	$($(PROGRAM)_variant6) -o $(PROGRAM)_variant6
$(PROGRAM)_variant7: $(sources) Makefile
	echo '#define OPTS "VARIANT7: $($(PROGRAM)_variant7)"' > options.h 
	$($(PROGRAM)_variant7) -S -o $(PROGRAM)_variant7.s
	$($(PROGRAM)_variant7) -o $(PROGRAM)_variant7
$(PROGRAM)_variant8: $(sources) Makefile
	echo '#define OPTS "VARIANT8: $($(PROGRAM)_variant8)"' > options.h 
	$($(PROGRAM)_variant8) -S -o $(PROGRAM)_variant8.s
	$($(PROGRAM)_variant8) -o $(PROGRAM)_variant8
$(PROGRAM)_variant9: $(sources) Makefile
	echo '#define OPTS "VARIANT9: $($(PROGRAM)_variant9)"' > options.h 
	$($(PROGRAM)_variant9) -S -o $(PROGRAM)_variant9.s
	$($(PROGRAM)_variant9) -o $(PROGRAM)_variant9

.autoclean: Makefile
	make clean
	touch .autoclean

.PHONY: clean
clean:
	rm -f *_variant* results*

