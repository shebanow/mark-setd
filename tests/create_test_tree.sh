#!/bin/bash
# Create a test directory tree with spaces and special characters

TEST_ROOT="${1:-/tmp/test_tree}"

# Remove existing test tree
rm -rf "$TEST_ROOT"

# Create base structure
mkdir -p "$TEST_ROOT"

# Create directories with spaces
mkdir -p "$TEST_ROOT/My Project"
mkdir -p "$TEST_ROOT/My Project/src"
mkdir -p "$TEST_ROOT/My Project/docs"
mkdir -p "$TEST_ROOT/My Project/tests"
mkdir -p "$TEST_ROOT/My Project/tests/unit tests"
mkdir -p "$TEST_ROOT/My Project/tests/integration tests"

# Create directories with special characters
mkdir -p "$TEST_ROOT/dir-with-dash"
mkdir -p "$TEST_ROOT/dir_with_underscore"
mkdir -p "$TEST_ROOT/dir.with.dot"
mkdir -p "$TEST_ROOT/dir@with@at"
mkdir -p "$TEST_ROOT/dir#with#hash"
mkdir -p "$TEST_ROOT/dir\$with\$dollar"
mkdir -p "$TEST_ROOT/dir%with%percent"
mkdir -p "$TEST_ROOT/dir&with&ampersand"
mkdir -p "$TEST_ROOT/dir*with*star"
mkdir -p "$TEST_ROOT/dir(with)parens"
mkdir -p "$TEST_ROOT/dir[with]brackets"
mkdir -p "$TEST_ROOT/dir{with}braces"
mkdir -p "$TEST_ROOT/dir~with~tilde"

# Create nested structures with mixed characters
mkdir -p "$TEST_ROOT/mixed/My Project/src"
mkdir -p "$TEST_ROOT/mixed/dir-with-dash/sub-dir"
mkdir -p "$TEST_ROOT/mixed/dir.with.dot/nested.dir"

# Create directories with unicode characters (if supported)
mkdir -p "$TEST_ROOT/测试目录"
mkdir -p "$TEST_ROOT/тестовая-директория"
mkdir -p "$TEST_ROOT/テストディレクトリ"

# Create some files for variety
touch "$TEST_ROOT/My Project/README.md"
touch "$TEST_ROOT/My Project/src/main.cpp"
touch "$TEST_ROOT/dir-with-dash/file.txt"
touch "$TEST_ROOT/dir.with.dot/config.ini"

echo "Test tree created at: $TEST_ROOT"
echo "Total directories: $(find "$TEST_ROOT" -type d | wc -l)"
echo "Total files: $(find "$TEST_ROOT" -type f | wc -l)"

