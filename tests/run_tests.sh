#!/bin/bash

# Shell Test Runner Script
# This script runs comprehensive tests for the shell implementation

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SHELL_DIR="/home/shainegross/shell-in-c"
TEST_DIR="$(pwd)"
SHELL_EXEC="./mysh"
LOG_FILE="test_results.log"

# Function to print colored output
print_status() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

print_header() {
    echo
    echo "=================================="
    echo "$1"
    echo "=================================="
}

# Function to run a test and capture results
run_test() {
    local test_name=$1
    local test_command=$2
    
    print_status $BLUE "Running: $test_name"
    
    if eval "$test_command" >> "$LOG_FILE" 2>&1; then
        print_status $GREEN "✓ PASSED: $test_name"
        return 0
    else
        print_status $RED "✗ FAILED: $test_name"
        return 1
    fi
}

# Initialize log file
echo "Shell Test Results - $(date)" > "$LOG_FILE"
echo "======================================" >> "$LOG_FILE"

# Test counters
total_tests=0
passed_tests=0
failed_tests=0

print_header "SHELL TESTING SUITE"
print_status $YELLOW "Starting comprehensive shell tests..."

# Check if shell executable exists
if [ ! -f "$SHELL_EXEC" ]; then
    print_status $RED "ERROR: Shell executable '$SHELL_EXEC' not found!"
    print_status $YELLOW "Please compile your shell first."
    exit 1
fi

print_status $GREEN "Found shell executable: $SHELL_EXEC"

# 1. Unit Tests
print_header "UNIT TESTS"
if [ -f "./test_shell" ]; then
    ((total_tests++))
    if run_test "Unit Tests" "./test_shell"; then
        ((passed_tests++))
    else
        ((failed_tests++))
    fi
else
    print_status $YELLOW "Unit test executable not found. Compile with: make unit-tests"
fi

# 2. Basic Command Tests
print_header "BASIC COMMAND TESTS"

basic_commands=(
    "echo 'Hello World'"
    "ls"
    "pwd"
    "date"
    "whoami"
)

for cmd in "${basic_commands[@]}"; do
    ((total_tests++))
    test_name="Basic Command: $cmd"
    test_script="timeout 5 $SHELL_EXEC << EOF
$cmd
exit
EOF"
    
    if run_test "$test_name" "$test_script"; then
        ((passed_tests++))
    else
        ((failed_tests++))
    fi
done

# 3. Pipe Tests
print_header "PIPELINE TESTS"

pipe_commands=(
    "echo 'hello world' | grep hello"
    "echo 'test data' | wc -w"
    "ls | grep test || echo 'no test files'"
    "echo -e 'line1\\nline2\\nline3' | wc -l"
    "echo 'multiple words here' | grep words | wc -c"
    "echo 'a b c d e' | tr ' ' '\\n' | wc -l"
    "echo 'hello world' | cat | cat | grep hello"
    "echo 'data' | cat | grep data | wc -w | cat"
)

for cmd in "${pipe_commands[@]}"; do
    ((total_tests++))
    test_name="Pipeline: $cmd"
    test_script="timeout 10 $SHELL_EXEC << EOF
$cmd
exit
EOF"
    
    if run_test "$test_name" "$test_script"; then
        ((passed_tests++))
    else
        ((failed_tests++))
    fi
done

# 4. Background Job Tests
print_header "BACKGROUND JOB TESTS"

# Create background test script
cat > bg_test_temp.sh << 'EOF'
#!/bin/bash
timeout 15 ./mysh << 'SHELL_EOF'
sleep 2 &
jobs
echo "Foreground command"
sleep 1 &
jobs
sleep 4
jobs
exit
SHELL_EOF
EOF

chmod +x bg_test_temp.sh

((total_tests++))
if run_test "Background Jobs" "./bg_test_temp.sh"; then
    ((passed_tests++))
else
    ((failed_tests++))
fi

rm -f bg_test_temp.sh

# 5. Mixed Foreground/Background Tests
print_header "MIXED FG/BG TESTS"

mixed_commands=(
    "sleep 1 & echo 'immediate'; sleep 2"
    "echo 'first' & echo 'second'; sleep 2"
    "sleep 1 | cat & echo 'pipeline bg'; sleep 2"
)

for cmd in "${mixed_commands[@]}"; do
    ((total_tests++))
    test_name="Mixed FG/BG: $cmd"
    test_script="timeout 10 $SHELL_EXEC << EOF
$cmd
exit
EOF"
    
    if run_test "$test_name" "$test_script"; then
        ((passed_tests++))
    else
        ((failed_tests++))
    fi
done

# 6. Redirection Tests
print_header "REDIRECTION TESTS"

# Create test input file
echo "test input data" > test_input_temp.txt

redirection_commands=(
    "echo 'output test' > test_out_temp.txt && cat test_out_temp.txt"
    "echo 'append1' > test_append_temp.txt && echo 'append2' >> test_append_temp.txt && cat test_append_temp.txt"
    "cat < test_input_temp.txt"
    "wc -w < test_input_temp.txt"
)

for cmd in "${redirection_commands[@]}"; do
    ((total_tests++))
    test_name="Redirection: $cmd"
    test_script="timeout 10 $SHELL_EXEC << EOF
$cmd
exit
EOF"
    
    if run_test "$test_name" "$test_script"; then
        ((passed_tests++))
    else
        ((failed_tests++))
    fi
done

# Cleanup temporary files
rm -f test_*_temp.txt

# 7. Stress Tests
print_header "STRESS TESTS"

# Multiple background jobs
((total_tests++))
stress_test_script="timeout 20 $SHELL_EXEC << EOF
sleep 1 &
sleep 2 &
echo 'test' | cat &
sleep 1 | wc -w &
jobs
sleep 5
jobs
exit
EOF"

if run_test "Multiple Background Jobs" "$stress_test_script"; then
    ((passed_tests++))
else
    ((failed_tests++))
fi

# Complex pipeline
((total_tests++))
complex_pipe_script="timeout 15 $SHELL_EXEC << EOF
echo -e 'apple\\nbanana\\ncherry\\ndate\\napricot' | grep a | sort | wc -l
exit
EOF"

if run_test "Complex Pipeline" "$complex_pipe_script"; then
    ((passed_tests++))
else
    ((failed_tests++))
fi

# 8. Error Handling Tests
print_header "ERROR HANDLING TESTS"

error_commands=(
    "nonexistent_command_12345"
    "ls /nonexistent/path/12345 || echo 'handled'"
    "cat /dev/null/impossible 2>/dev/null || echo 'error handled'"
)

for cmd in "${error_commands[@]}"; do
    ((total_tests++))
    test_name="Error Handling: $cmd"
    test_script="timeout 10 $SHELL_EXEC << EOF
$cmd
exit
EOF"
    
    # For error tests, we just want to make sure the shell doesn't crash
    if run_test "$test_name" "$test_script"; then
        ((passed_tests++))
    else
        ((failed_tests++))
    fi
done

# Final Results
print_header "TEST RESULTS"
echo
print_status $BLUE "Total Tests: $total_tests"
print_status $GREEN "Passed: $passed_tests"
print_status $RED "Failed: $failed_tests"

if [ $failed_tests -eq 0 ]; then
    print_status $GREEN "🎉 ALL TESTS PASSED! 🎉"
    echo
    print_status $GREEN "Your shell implementation is working correctly!"
    exit_code=0
else
    print_status $RED "❌ Some tests failed"
    echo
    print_status $YELLOW "Check the log file '$LOG_FILE' for detailed results"
    print_status $YELLOW "Failed tests may indicate issues with:"
    print_status $YELLOW "  - Terminal control (SIGTTOU handling)"
    print_status $YELLOW "  - Process group management"
    print_status $YELLOW "  - Signal handling"
    print_status $YELLOW "  - Pipe implementation"
    print_status $YELLOW "  - Job control"
    exit_code=1
fi

echo
print_status $BLUE "Test log saved to: $LOG_FILE"
echo

exit $exit_code
