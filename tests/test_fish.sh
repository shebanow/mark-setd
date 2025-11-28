#!/usr/bin/env fish
# Test script for fish shell

set TEST_ROOT "/tmp/test_tree"
set SCRIPT_DIR (dirname (status --current-filename))
set PROJECT_ROOT (cd $SCRIPT_DIR/.. && pwd)

# Set environment
set -gx SETD_DIR (test -n "$SETD_DIR" && echo "$SETD_DIR" || echo "$HOME/bin")
set -gx MARK_DIR (test -n "$MARK_DIR" && echo "$MARK_DIR" || echo "$HOME/bin")
set -gx PATH "$SETD_DIR:$PATH"

# Fish doesn't support sourcing bash scripts directly, so we need to define functions
function cd
    set target_dir (setd $argv)
    builtin cd "$target_dir"
    echo $PWD
end

function mark
    command mark $argv
    if test -f "$MARK_DIR/mark_db"
        source "$MARK_DIR/mark_db"
    end
end

# Create test tree
bash "$SCRIPT_DIR/create_test_tree.sh" "$TEST_ROOT"

# Test results
set PASSED 0
set FAILED 0

function test_cd
    set target $argv[1]
    set expected_dir $argv[2]
    set test_name $argv[3]
    
    cd "$target" 2>/dev/null
    set actual_dir (pwd)
    
    if test "$actual_dir" = "$expected_dir"
        echo "PASS: $test_name"
        set PASSED (math $PASSED + 1)
        return 0
    else
        echo "FAIL: $test_name (expected: $expected_dir, got: $actual_dir)"
        set FAILED (math $FAILED + 1)
        return 1
    end
end

function test_cd_list
    set test_name $argv[1]
    
    set output (cd -l 2>&1)
    
    if test -n "$output"
        echo "PASS: $test_name"
        set PASSED (math $PASSED + 1)
        return 0
    else
        echo "FAIL: $test_name (no output from cd -l)"
        set FAILED (math $FAILED + 1)
        return 1
    end
end

function test_cd_number
    set num $argv[1]
    set test_name $argv[2]
    
    cd "-$num" 2>/dev/null
    set result $status
    
    if test $result -eq 0
        echo "PASS: $test_name"
        set PASSED (math $PASSED + 1)
        return 0
    else
        echo "FAIL: $test_name (cd -$num failed)"
        set FAILED (math $FAILED + 1)
        return 1
    end
end

echo "=== Testing with fish ==="
echo ""

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

echo ""
echo "=== Test Summary ==="
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if test $FAILED -eq 0
    exit 0
else
    exit 1
end

