#!/bin/bash

# Comprehensive Shell Testing Suite
# Advanced edge cases, performance tests, and complex scenarios
# Run this when you need exhaustive testing beyond basic integration

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SHELL_EXEC="./mysh"
TESTS_RUN=0
TESTS_PASSED=0

# Test helper functions
run_comprehensive_test() {
    local test_name="$1"
    local test_function="$2"
    
    echo -e "${BLUE}Running: ${test_name}...${NC}"
    TESTS_RUN=$((TESTS_RUN + 1))
    
    if $test_function; then
        echo -e "${GREEN}PASSED${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}FAILED${NC}"
    fi
    echo
}

# Cleanup function
cleanup_comprehensive() {
    rm -f *.tmp *.log stress_*.txt perf_*.txt env_*.txt
    rm -f comprehensive_*.sh comprehensive_*.txt
}

# Trap to ensure cleanup on exit
trap cleanup_comprehensive EXIT

# Check if shell exists
if [ ! -f "$SHELL_EXEC" ]; then
    echo -e "${RED}Error: $SHELL_EXEC not found${NC}"
    exit 1
fi

echo "=== Comprehensive Shell Testing Suite ==="
echo "This suite tests advanced features and edge cases"
echo

# Test 1: Complex Variable Expansion
test_complex_variables() {
    timeout 10 $SHELL_EXEC << 'EOF' > comprehensive_vars.txt 2>&1
export VAR1=hello
export VAR2=world
set LOCAL1=test
echo $VAR1_$VAR2
echo $(VAR1)_$(VAR2)
echo prefix_$(LOCAL1)_suffix
echo \$VAR1 should not expand
echo $UNDEFINED should be empty: end
exit
EOF
    
    # Check outputs
    grep -q "hello_world" comprehensive_vars.txt && \
    grep -q "prefix_test_suffix" comprehensive_vars.txt && \
    grep -q "\$VAR1 should not expand" comprehensive_vars.txt && \
    grep -q "should be empty: end" comprehensive_vars.txt
}

# Test 2: Stress Test - Many Pipes
test_pipe_stress() {
    timeout 15 $SHELL_EXEC << 'EOF' > comprehensive_pipes.txt 2>&1
echo "1 2 3 4 5 6 7 8 9 10" | tr ' ' '\n' | grep -E "[0-9]" | sort -n | head -5 | tail -3 | wc -l
exit
EOF
    
    grep -q "3" comprehensive_pipes.txt
}

# Test 3: Complex Redirection with Variables
test_complex_redirection() {
    timeout 10 $SHELL_EXEC << 'EOF' > comprehensive_redir.txt 2>&1
export OUTFILE=test_complex.out
export APPENDFILE=test_complex.app
echo "first line" > $(OUTFILE)
echo "append line" >> $(APPENDFILE)
cat $(OUTFILE) $(APPENDFILE)
rm -f $(OUTFILE) $(APPENDFILE)
exit
EOF
    
    grep -q "first line" comprehensive_redir.txt && \
    grep -q "append line" comprehensive_redir.txt
}

# Test 4: Job Control Edge Cases
test_job_control_advanced() {
    timeout 15 $SHELL_EXEC << 'EOF' > comprehensive_jobs.txt 2>&1
echo "background test" &
jobs
fg %1 || true
jobs
exit
EOF
    
    grep -q "background test" comprehensive_jobs.txt
}

# Test 5: Performance Test - Many Variables
test_variable_performance() {
    timeout 20 $SHELL_EXEC << 'EOF' > comprehensive_perf.txt 2>&1
export VAR1=test1
export VAR2=test2
export VAR3=test3
set LOCAL1=local1
set LOCAL2=local2
for i in 1 2 3 4 5; do
    echo "Iteration $i: $(VAR1) $(VAR2) $(LOCAL1)"
done 2>/dev/null || echo "For loop not supported, testing basic expansion"
echo $(VAR1)$(VAR2)$(VAR3)$(LOCAL1)$(LOCAL2)
exit
EOF
    
    # Should have either for loop output or basic expansion
    grep -q "test1\|local1" comprehensive_perf.txt
}

# Run all comprehensive tests
echo -e "${YELLOW}=== Advanced Variable Tests ===${NC}"
run_comprehensive_test "Complex variable expansion" test_complex_variables

echo -e "${YELLOW}=== Performance Tests ===${NC}"
run_comprehensive_test "Pipe stress test" test_pipe_stress
run_comprehensive_test "Variable performance" test_variable_performance

echo -e "${YELLOW}=== Advanced Feature Tests ===${NC}"
run_comprehensive_test "Complex redirection" test_complex_redirection
run_comprehensive_test "Job control edge cases" test_job_control_advanced

# Results
echo "=== Comprehensive Test Results ==="
echo -e "${GREEN}Passed: $TESTS_PASSED${NC}"
echo -e "${RED}Failed: $((TESTS_RUN - TESTS_PASSED))${NC}"
echo -e "${BLUE}Total: $TESTS_RUN${NC}"

if [ $TESTS_PASSED -eq $TESTS_RUN ]; then
    echo -e "${GREEN}🎉 All comprehensive tests passed!${NC}"
    exit 0
else
    echo -e "${RED}❌ Some comprehensive tests failed${NC}"
    exit 1
fi
