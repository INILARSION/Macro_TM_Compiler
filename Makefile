cfiles := $(wildcard *.c)
hfiles := $(wildcard *.h)

compile: $(cfiles) $(hfiles)
	gcc -O2 -o macro_compiler $(cfiles) $(hfiles)
