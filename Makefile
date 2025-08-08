TARGET = mysh

CC = gcc

SRC_DIR = src
INC_DIR = include
TEST_DIR = tests

SRCS = $(SRC_DIR)/main.c \
	   $(SRC_DIR)/parser.c \
	   $(SRC_DIR)/builtin.c \
	   $(SRC_DIR)/jobs.c \
	   $(SRC_DIR)/signals.c\
	   $(SRC_DIR)/vars.c 

CFLAGS = -Wall -I$(INC_DIR)
DEBUG_CFLAGS = -Wall -I$(INC_DIR) -g -O0
TEST_CFLAGS = -Wall -Wextra -std=c99 -g -D_POSIX_C_SOURCE=200809L -I$(INC_DIR)

# Test executables
UNIT_TEST = $(TEST_DIR)/test_shell
INTEGRATION_TEST = $(TEST_DIR)/test_integrated

.PHONY: all clean debug unit-tests integration-tests full-tests test-pipes test-background test-redirection test-job-control debug-shell valgrind-shell strace-shell help

# Default build (release)
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

# Debug build
debug: $(SRCS)
	$(CC) $(DEBUG_CFLAGS) $(SRCS) -o $(TARGET)

# Test targets
# Build and run unit tests
unit-tests: $(UNIT_TEST)
	./$(UNIT_TEST)

$(UNIT_TEST): $(TEST_DIR)/test_shell.c
	$(CC) $(TEST_CFLAGS) -o $@ $<

# Build and run integration tests
integration-tests: $(INTEGRATION_TEST) $(TARGET)
	./$(INTEGRATION_TEST)

$(INTEGRATION_TEST): $(TEST_DIR)/test_integrated.c
	$(CC) $(TEST_CFLAGS) -o $@ $<

# Run all basic tests
all-tests: unit-tests integration-tests
	@echo "=== All Core Tests Completed ==="

# Run comprehensive tests (advanced scenarios)
comprehensive-tests: $(TARGET)
	@echo "=== Running Comprehensive Test Suite ==="
	./tests/test_comprehensive.sh

# Run all tests including comprehensive
full-tests: all-tests comprehensive-tests
	@echo "=== All Tests (Core + Comprehensive) Completed ==="

# Individual test categories (for debugging specific functionality)
test-pipes: $(INTEGRATION_TEST) $(TARGET)
	@echo "=== Testing Pipe Functionality ==="
	./$(INTEGRATION_TEST) | grep -E "(pipe|Pipeline)"

test-background: $(INTEGRATION_TEST) $(TARGET)
	@echo "=== Testing Background Job Functionality ==="
	./$(INTEGRATION_TEST) | grep -E "(background|Background)"

test-redirection: $(INTEGRATION_TEST) $(TARGET)
	@echo "=== Testing Redirection Functionality ==="
	./$(INTEGRATION_TEST) | grep -E "(redirection|Redirection)"

test-variables: $(INTEGRATION_TEST) $(TARGET)
	@echo "=== Testing Variable Functionality ==="
	./$(INTEGRATION_TEST) | grep -E "(variable|Variable)"

test-job-control: $(INTEGRATION_TEST) $(TARGET)
	@echo "=== Testing Job Control Functionality ==="
	./$(INTEGRATION_TEST) | grep -E "(job|Job)"

# Debugging targets
debug-shell: $(TARGET)
	gdb $(TARGET)

valgrind-shell: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

strace-shell: $(TARGET)
	strace -f -o shell_strace.log ./$(TARGET)

# Clean up
clean:
	rm -f $(TARGET)
	rm -f $(UNIT_TEST) $(INTEGRATION_TEST)
	rm -f $(SRC_DIR)/*.o
	rm -f *.o *.log *.txt *.sh *.tmp *.out *.app
	rm -f test_*.txt test_*.sh
	rm -f *var*.txt *var*.sh
	rm -f comprehensive_*.txt comprehensive_*.sh
	rm -f shell_strace.log

# Help target
help:
	@echo "Available targets:"
	@echo "  all              - Build the shell executable (default)"
	@echo "  debug            - Build the shell with debug symbols"
	@echo "  unit-tests       - Build and run unit tests"
	@echo "  integration-tests - Build and run integration tests"
	@echo "  full-tests       - Run both unit and integration tests"
	@echo "  test-pipes       - Test only pipe functionality"
	@echo "  test-background  - Test only background job functionality"
	@echo "  test-redirection - Test only redirection functionality"
	@echo "  test-job-control - Test only job control functionality"
	@echo "  debug-shell      - Start shell in GDB debugger"
	@echo "  valgrind-shell   - Run shell with Valgrind memory checking"
	@echo "  strace-shell     - Run shell with system call tracing"
	@echo "  clean            - Remove all generated files"
	@echo "  help             - Show this help message"