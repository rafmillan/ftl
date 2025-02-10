# Compiler and flags
CC      := gcc
CFLAGS  := -I..

#-Wall -Wextra -O2
# Additional libraries (link these after the object files)
LIBS    := -lm

# Gather all library .c files from the parent directory,
# excluding main.c (which is the application entry point)
LIB_SRCS := $(filter-out ../main.c, $(wildcard ../*.c))

# Directories for intermediate and binary files in tests
BUILDDIR := build
BINDIR   := $(BUILDDIR)/bin

# Find all test source files (*.c) in this directory.
TEST_SRCS := $(wildcard *.c)
# For each test source file, produce an executable in the bin directory.
# For example, test_ecc_basic.c becomes build/bin/test_ecc_basic.
TESTS := $(patsubst %.c,$(BINDIR)/%,$(TEST_SRCS))

# Default target: build all tests.
all: $(TESTS)

# Ensure the build directories exist.
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BINDIR): $(BUILDDIR)
	mkdir -p $(BINDIR)

# Pattern rule: compile each test source file and link with all library sources.
# Notice that $(LIBS) is appended at the end.
$(BINDIR)/%: %.c $(LIB_SRCS) | $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# Clean up all generated files in the test build directory.
clean:
	rm -rf $(BUILDDIR)

.PHONY: all clean
