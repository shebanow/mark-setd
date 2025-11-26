#!/bin/ksh
# Test script for ksh shell

set -e

TEST_ROOT="/tmp/test_tree"
# ksh doesn't have .sh.file, use $0 instead
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Set environment
export SETD_DIR=/root/bin
export MARK_DIR=/root/bin
export PATH="/root/bin:${PATH}"

# Source SETD_BASH (ksh is mostly compatible with bash)
if [ -f "$PROJECT_ROOT/SETD_BASH" ]; then
    . "$PROJECT_ROOT/SETD_BASH"
else
    echo "ERROR: SETD_BASH not found"
    exit 1
fi

# Create test tree
"$SCRIPT_DIR/create_test_tree.sh" "$TEST_ROOT"

# Test results
PASSED=0
FAILED=0

test_cd() {
    target="$1"
    expected_dir="$2"
    test_name="$3"
    
    cd "$target" 2>/dev/null
    actual_dir=$(pwd)
    
    if [ "$actual_dir" = "$expected_dir" ]; then
        print "PASS: $test_name"
        ((PASSED++))
        return 0
    else
        print "FAIL: $test_name (expected: $expected_dir, got: $actual_dir)"
        ((FAILED++))
        return 1
    fi
}

test_cd_list() {
    test_name="$1"
    
    output=$(cd -l 2>&1)
    
    if [ -n "$output" ]; then
        print "PASS: $test_name"
        ((PASSED++))
        return 0
    else
        print "FAIL: $test_name (no output from cd -l)"
        ((FAILED++))
        return 1
    fi
}

test_cd_number() {
    num="$1"
    test_name="$2"
    
    cd "-$num" 2>/dev/null
    result=$?
    
    if [ $result -eq 0 ]; then
        print "PASS: $test_name"
        ((PASSED++))
        return 0
    else
        print "FAIL: $test_name (cd -$num failed)"
        ((FAILED++))
        return 1
    fi
}

print "=== Testing with ksh ==="
print ""

# Start from home
cd ~

# Test 1: Navigate to directory with spaces
test_cd "$TEST_ROOT/My Project" "$TEST_ROOT/My Project" "cd to directory with spaces"

# Test 2: Navigate to directory with dash
test_cd "$TEST_ROOT/dir-with-dash" "$TEST_ROOT/dir-with-dash" "cd to directory with dash"

# Test 3: Navigate to directory with dot
test_cd "$TEST_ROOT/dir.with.dot" "$TEST_ROOT/dir.with.dot" "cd to directory with dot"

# Test 4: Navigate to nested directory with spaces
test_cd "$TEST_ROOT/My Project/src" "$TEST_ROOT/My Project/src" "cd to nested directory with spaces"

# Test 5: Test cd -l (list history)
test_cd_list "cd -l shows history"

# Test 6: Test cd -1 (go back one)
cd "$TEST_ROOT/My Project"
cd "$TEST_ROOT/dir-with-dash"
# Need to wait a moment for history to update
sleep 0.1
test_cd_number 1 "cd -1 navigates back"

# Test 7: Test cd -2 (go back two)
cd "$TEST_ROOT/My Project"
cd "$TEST_ROOT/dir-with-dash"
cd "$TEST_ROOT/dir.with.dot"
# Need to wait a moment for history to update
sleep 0.1
test_cd_number 2 "cd -2 navigates back two"

# Test 8: Mark a directory and navigate to it
cd "$TEST_ROOT/My Project"
mark testproj
cd ~
cd testproj
test_cd "$TEST_ROOT/My Project" "$TEST_ROOT/My Project" "cd to marked directory"

print ""
print "=== Test Summary ==="
print "Passed: $PASSED"
print "Failed: $FAILED"

if [ $FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi

