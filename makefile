# CC = gcc 
# INCLUDES = -I/home/mrjantz/cs360/include
# CFLAGS = $(INCLUDES)
# LIBDIR = /home/mrjantz/cs360/lib
# LIBS = $(LIBDIR)/libfdr.a 

# EXECUTABLES = bin/tarx

# all: $(EXECUTABLES)

# bin/tarx: src/tarx.c
# 	$(CC) $(CFLAGS) -o bin/tarx src/tarx.c $(LIBS)

# clean:
# 	rm -f $(EXECUTABLES)
CC = gcc
INCLUDES = -I/home/mrjantz/cs360/include
CFLAGS = -g $(INCLUDES)  # Add -g to enable debugging symbols
LIBDIR = /home/mrjantz/cs360/lib
LIBS = $(LIBDIR)/libfdr.a 

EXECUTABLES = bin/tarx

all: $(EXECUTABLES)

bin/tarx: src/tarx.c
	$(CC) $(CFLAGS) -o bin/tarx src/tarx.c $(LIBS)

run: bin/tarx
	# Run the program inside gdb to get detailed information about the segfault
	gdb --args ./bin/tarx < /home/mrjantz/cs360/labs/Lab-6-Tarx/Gradescript-Examples/001.tarc

clean:
	rm -f $(EXECUTABLES)
