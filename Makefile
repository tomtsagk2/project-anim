# Files
HEADERS = $(wildcard src/*.h)
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

# Project Data
name = project
version = 0.0.0
OUT = bin/${name}

# Compiler arguments
CC = gcc
CFLAGS = -Wall -Wextra -pedantic

### Rules ###
${OUT}: ${OBJ}
	-mkdir bin/
	$(CC) $(CFLAGS) -o $@ $(OBJ)

clean:
	@echo Cleaning ...
	-rm -f ${OUT} ${OBJ}
	-rm -r bin/

.PHONY : clean

# Re-compile on header change
${OBJ}: ${HEADERS}

# How to compile c files
.c.o:
	${CC} ${CFLAGS} -c -o ${<:.c=.o} $<
