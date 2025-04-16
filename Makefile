# Compiler and flags
CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lm

# Detect operating system
OS := $(shell uname -s)

# Adjust settings based on OS
ifeq ($(OS),Darwin) # macOS
    CC = gcc
    CFLAGS += -std=c99
    LDFLAGS = -lm
else ifneq (,$(findstring MINGW,$(OS))) # Windows (MSYS2/MinGW)
    CFLAGS += -std=c99
    LDFLAGS = -lm
    EXEC = minecraft.exe
else # Assume Linux or other Unix-like (for completeness)
    CFLAGS += -std=c99
    LDFLAGS = -lm
endif

# Default executable name
EXEC ?= minecraft

# Source files
SRC = minecraft.c

# Object files
OBJ = $(SRC:.c=.o)

# Default target
all: $(EXEC)

# Link object files to create executable
$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJ) $(EXEC)

# Phony targets
.PHONY: all clean
