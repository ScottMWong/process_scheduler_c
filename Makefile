
#Compiler flags
CFLAGS=-Wall

#Link math.h
# Thanks to https://stackoverflow.com/questions/42769853/issue-with-makefile-for-including-math-h for giving an explicit answer for this
LIBS += -lm

#Make and clean command definitions

allocate: allocate.o
	gcc $(CFLAGS) -o allocate allocate.o $(LIBS)

allocate.o: allocate.c allocate.h
	gcc $(CFLAGS) -c allocate.c

.PHONY: clean

clean:
	rm -f *.o allocate