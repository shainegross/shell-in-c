#!/bin/bash

# Integration Tests for mysh shell
# Tests shell binary with real commands

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0

# Test helper functions
run_test() {
    local test_name="$1"
    local command="$2"
    local expected_output="$3"
    local description="$4"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    echo -e "${YELLOW}Test $TESTS_RUN: $test_name${NC}"
    echo "Command: $command"
    echo "Expected: $expected_output"
    
    # Run the command through our shell
    raw_output=$(echo "$command" | timeout 5s ./mysh 2>/dev/null)
    
    # Extract just the command output, filtering out prompts
    actual_output=$(echo "$raw_output" | sed 's/mysh> //g' | grep -v "^$" | head -1)
    
    if [ "$actual_output" = "$expected_output" ]; then
        echo -e "${GREEN}✓ PASS${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ FAIL${NC}"
        echo "  Got: '$actual_output'"
    fi
    echo
}

run_test_file() {
    local test_name="$1"
    local command="$2"
    local expected_file="$3"
    local description="$4"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    echo -e "${YELLOW}Test $TESTS_RUN: $test_name${NC}"
    echo "Command: $command"
    echo "Expected file: $expected_file"
    
    # Clean up any existing output file
    rm -f "$expected_file"
    
    # Run the command through our shell
    echo "$command" | timeout 5s ./mysh >/dev/null 2>&1
    
    if [ -f "$expected_file" ] && [ -s "$expected_file" ]; then
        echo -e "${GREEN}✓ PASS - File created successfully${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo "File contents: $(cat "$expected_file")"
    else
        echo -e "${RED}✗ FAIL - File not created or empty${NC}"
    fi
    echo
}

# Setup
echo "========================================="
echo "Integration Tests for mysh Shell"
echo "========================================="
echo

# Check if shell binary exists
if [ ! -f "./mysh" ]; then
    echo -e "${RED}Error: mysh binary not found. Run 'make' first.${NC}"
    exit 1
fi

# Create test files
echo "test content" > test_input.txt
echo "hello world" > test_data.txt
echo -e "line1\nline2\nline3" > test_lines.txt

echo "Starting tests..."
echo

# Test 1: Built-in commands
echo "exit" | timeout 2s ./mysh >/dev/null 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Exit command works${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}✗ Exit command failed${NC}"
fi
TESTS_RUN=$((TESTS_RUN + 1))
echo

# Test 2: pwd command
run_test "PWD Command" "pwd" "$(pwd)" "Current working directory"

# Test 3: Simple command
run_test "Echo Command" "echo hello" "hello" "Basic command execution"

# Test 4: Command with arguments
run_test "Echo with Args" "echo hello world" "hello world" "Command with multiple arguments"

# Test 5: Input redirection
run_test "Input Redirection" "cat < test_input.txt" "test content" "Reading from file"

# Test 6: Output redirection
run_test_file "Output Redirection" "echo 'output test' > test_output.txt" "test_output.txt" "Writing to file"

# Test 7: Append redirection
echo "first line" > test_append.txt
run_test_file "Append Redirection" "echo 'second line' >> test_append.txt" "test_append.txt" "Appending to file"

# Test 8: Simple pipe
if command -v wc >/dev/null 2>&1; then
    run_test "Simple Pipe" "echo 'one two three' | wc -w" "3" "Basic pipe functionality"
fi

# Test 9: Pipe with file input
if command -v wc >/dev/null 2>&1; then
    run_test "Pipe with Input" "cat test_lines.txt | wc -l" "3" "Pipe with file input"
fi

# Test 10: Multiple pipes
if command -v grep >/dev/null 2>&1 && command -v wc >/dev/null 2>&1; then
    echo -e "apple\nbanana\napricot\ncherry" > test_fruits.txt
    run_test "Multiple Pipes" "cat test_fruits.txt | grep a | wc -l" "3" "Multiple command pipeline"
fi

# Test 11: Complex redirection + pipe
if command -v grep >/dev/null 2>&1; then
    run_test_file "Pipe to File" "echo -e 'line1\nline2\nline3' | grep line > test_grep_output.txt" "test_grep_output.txt" "Pipe output to file"
fi

# Test 12: cd command
mkdir -p test_dir
current_dir=$(pwd)
echo "cd test_dir && pwd" | timeout 2s ./mysh 2>/dev/null | grep -q "test_dir"
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ CD command works${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}✗ CD command failed${NC}"
fi
TESTS_RUN=$((TESTS_RUN + 1))
echo

# Test background jobs
echo -e "${YELLOW}Test: Background Job Creation${NC}"
TESTS_RUN=$((TESTS_RUN + 1))

# Run a background job and check for job notification
output=$(echo "sleep 1 &" | timeout 3s ./mysh 2>&1)
if echo "$output" | grep -q "\[.*\].*[0-9]\+"; then
    echo -e "${GREEN}✓ PASS - Background job notification displayed${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
    echo "Output: $output"
else
    echo -e "${RED}✗ FAIL - No background job notification${NC}"
    echo "Output: $output"
fi
echo

# Test background job detection
echo -e "${YELLOW}Test: Background Process Detection${NC}"
TESTS_RUN=$((TESTS_RUN + 1))

# Test that shell doesn't wait for background jobs
start_time=$(date +%s)
echo "sleep 2 &" | timeout 5s ./mysh >/dev/null 2>&1
end_time=$(date +%s)
duration=$((end_time - start_time))

if [ $duration -lt 2 ]; then
    echo -e "${GREEN}✓ PASS - Shell returned immediately for background job${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
    echo "Duration: ${duration}s (should be < 2s)"
else
    echo -e "${RED}✗ FAIL - Shell waited for background job${NC}"
    echo "Duration: ${duration}s (should be < 2s)"
fi
echo

# Test foreground vs background behavior
echo -e "${YELLOW}Test: Foreground vs Background Job Behavior${NC}"
TESTS_RUN=$((TESTS_RUN + 1))

# Foreground should block
start_time=$(date +%s)
echo "sleep 1" | timeout 3s ./mysh >/dev/null 2>&1
end_time=$(date +%s)
fg_duration=$((end_time - start_time))

# Background should not block
start_time=$(date +%s)
echo "sleep 1 &" | timeout 3s ./mysh >/dev/null 2>&1
end_time=$(date +%s)
bg_duration=$((end_time - start_time))

if [ $fg_duration -ge 1 ] && [ $bg_duration -lt 1 ]; then
    echo -e "${GREEN}✓ PASS - Foreground blocks, background doesn't${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
    echo "Foreground duration: ${fg_duration}s, Background duration: ${bg_duration}s"
else
    echo -e "${RED}✗ FAIL - Incorrect blocking behavior${NC}"
    echo "Foreground duration: ${fg_duration}s, Background duration: ${bg_duration}s"
fi
echo

# Cleanup
rm -f test_input.txt test_data.txt test_lines.txt test_output.txt test_append.txt test_fruits.txt test_grep_output.txt
rm -rf test_dir

# Results
echo "========================================="
echo "Test Results"
echo "========================================="
echo "Tests run: $TESTS_RUN"
echo "Tests passed: $TESTS_PASSED"
echo "Tests failed: $((TESTS_RUN - TESTS_PASSED))"

if [ $TESTS_PASSED -eq $TESTS_RUN ]; then
    echo -e "${GREEN}All tests PASSED! 🎉${NC}"
    exit 0
else
    echo -e "${RED}Some tests FAILED! 😞${NC}"
    exit 1
fi
