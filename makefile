# Compiler and flags
CC      := gcc
CFLAGS  := -g #-Wall -Wextra -O2

# Directories for intermediate and binary files
BUILDDIR := build
BINDIR   := $(BUILDDIR)/bin

# Source files
SRCS    := main.c nand.c

# Object files will be stored in the build directory
OBJS    := $(patsubst %.c,$(BUILDDIR)/%.o,$(SRCS))

# Default target: build the main binary
all: $(BINDIR)/main

# Ensure the build directories exist
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BINDIR): $(BUILDDIR)
	mkdir -p $(BINDIR)

# Link object files to create the final executable in the bin directory
$(BINDIR)/main: $(OBJS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Compile .c files into .o files placed in the build directory.
$(BUILDDIR)/%.o: %.c nand.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build tests by invoking the Makefile in the test/ directory.
tests:
	$(MAKE) -C test

# Clean up all generated files
clean:
	rm -rf $(BUILDDIR)
	rm -rf test/$(BUILDDIR)

.PHONY: all tests clean
