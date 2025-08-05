#!/bin/bash

# Quick verification script for specific shell features
# Use this for rapid testing during development

SHELL_EXEC="./mysh"

if [ ! -f "$SHELL_EXEC" ]; then
    echo "Error: $SHELL_EXEC not found"
    exit 1
fi

echo "=== Quick Shell Feature Tests ==="

# Test 1: Simple pipe
echo "Test 1: Simple pipe"
timeout 5 $SHELL_EXEC << 'EOF'
echo "hello world" | grep hello
exit
EOF
echo "---"

# Test 2: Background job
echo "Test 2: Background job"
timeout 8 $SHELL_EXEC << 'EOF'
sleep 2 &
jobs
echo "This should appear immediately"
sleep 3
jobs
exit
EOF
echo "---"

# Test 3: Double pipe
echo "Test 3: Double pipe"
timeout 5 $SHELL_EXEC << 'EOF'
echo "one two three" | wc -w | cat
exit
EOF
echo "---"

# Test 4: Background pipeline
echo "Test 4: Background pipeline"
timeout 8 $SHELL_EXEC << 'EOF'
echo "background test" | grep test | wc -w &
jobs
echo "Foreground continues"
sleep 3
jobs
exit
EOF
echo "---"

# Test 5: Triple pipe
echo "Test 5: Triple pipe"
timeout 5 $SHELL_EXEC << 'EOF'
echo -e "line1\nline2\nline3" | grep line | wc -l | cat
exit
EOF
echo "---"

echo "=== Quick tests completed ==="
