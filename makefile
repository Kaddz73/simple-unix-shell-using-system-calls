# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Source file and object file
SRC = dragonshell.c
OBJ = dragonshell.o

# Main target that links the object file to create the executable
dragonshell: $(OBJ)
	$(CC) $(CFLAGS) -o dragonshell $(OBJ)

# Compile target to produce the object file
compile: $(SRC)
	$(CC) $(CFLAGS) -c $(SRC)

# Clean target to remove the object file and executable
clean:
	rm -f $(OBJ) dragonshell

.PHONY: clean compile