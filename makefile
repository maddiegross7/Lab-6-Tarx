CC = gcc 
INCLUDES = -I/home/makefile/cs360/include
CFLAGS = $(INCLUDES)
LIBDIR = /home/makefile/cs360/lib
LIBS = $(LIBDIR)/libfdr.a 

EXECUTABLES = bin/tarx

all: $(EXECUTABLES)

bin/tarx: src/tarx.c
	$(CC) $(CFLAGS) -o bin/tarx src/tarx.c $(LIBS)

clean:
	rm -f $(EXECUTABLES)
