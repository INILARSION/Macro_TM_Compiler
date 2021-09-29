cfiles := $(wildcard *.c)

compile: $(cfiles) $(hfiles)
	gcc -O2 -o macro_compiler $(cfiles)
