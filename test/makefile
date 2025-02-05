# Compiler and flags
CC      := gcc
CFLAGS  := -g -I..
#-Wall -Wextra -O2
# The NAND library source from the parent directory
LIB_SRC := ../nand.c

# Directories for intermediate and binary files in tests
BUILDDIR := build
BINDIR   := $(BUILDDIR)/bin

# Find all test source files (*.c) in this directory.
TEST_SRCS := $(wildcard *.c)
# For each test source file, produce an executable in the bin directory.
# (Directly substitute from %.c to $(BINDIR)/% so that, e.g., test_nand_basic.c becomes build/bin/test_nand_basic.)
TESTS := $(patsubst %.c, $(BINDIR)/%, $(TEST_SRCS))

# Default target: build all tests.
all: $(TESTS)

# Ensure the build directories exist.
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BINDIR): $(BUILDDIR)
	mkdir -p $(BINDIR)

# Pattern rule: compile each test source file and link with the NAND library into the bin directory.
$(BINDIR)/%: %.c $(LIB_SRC) | $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@

# Clean up all generated files in the test build directory.
clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean
