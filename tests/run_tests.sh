#!/bin/bash
# Test runner script for local testing (without Docker)
# For CI/CD, use GitHub Actions workflows

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Detect OS
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if [ -f /etc/debian_version ]; then
            echo "debian"
        elif [ -f /etc/redhat-release ]; then
            echo "centos"
        else
            echo "linux"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        echo "windows"
    else
        echo "unknown"
    fi
}

# Test configuration
SHELLS=("bash" "zsh" "csh" "tcsh" "sh" "dash" "ksh" "fish")

# Parse arguments
SHELL_FILTER=""
BUILD_ONLY=false
RUN_TESTS=true

usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -s, --shell SHELL     Test only specified shell (bash, zsh, csh, tcsh, sh, dash, ksh, fish)"
    echo "  -b, --build-only       Only build, don't run tests"
    echo "  -h, --help            Show this help message"
    echo ""
    echo "Note: This script is for local testing. For CI/CD, use GitHub Actions workflows."
    echo ""
    echo "Examples:"
    echo "  $0                    # Run all tests for available shells"
    echo "  $0 -s bash             # Test only bash shell"
    echo "  $0 -s zsh              # Test only zsh shell"
}

while [[ $# -gt 0 ]]; do
    case $1 in
        -s|--shell)
            SHELL_FILTER="$2"
            shift 2
            ;;
        -b|--build-only)
            BUILD_ONLY=true
            RUN_TESTS=false
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Filter shells
if [ -n "$SHELL_FILTER" ]; then
    SHELLS=("$SHELL_FILTER")
fi

# Check if shell is available
check_shell() {
    local shell=$1
    local shell_path
    
    case $shell in
        bash) shell_path="/bin/bash" ;;
        zsh) shell_path="/bin/zsh" ;;
        csh) shell_path="/bin/csh" ;;
        tcsh) shell_path="/bin/tcsh" ;;
        sh) shell_path="/bin/sh" ;;
        dash) shell_path="/bin/dash" ;;
        ksh) shell_path="/bin/ksh" ;;
        fish) shell_path="/usr/bin/fish" ;;
        *)
            return 1
            ;;
    esac
    
    [ -x "$shell_path" ] || command -v "$shell" >/dev/null 2>&1
}

# Build the project
build_project() {
    echo -e "${YELLOW}Building project...${NC}"
    cd "$PROJECT_ROOT"
    
    if ! make clean && make all; then
        echo -e "${RED}Build failed${NC}"
        return 1
    fi
    
    # Install binaries
    mkdir -p "$HOME/bin"
    cp setd "$HOME/bin/" 2>/dev/null || cp setd.exe "$HOME/bin/" 2>/dev/null || true
    cp mark "$HOME/bin/" 2>/dev/null || cp mark.exe "$HOME/bin/" 2>/dev/null || true
    chmod +x "$HOME/bin/setd" "$HOME/bin/mark" 2>/dev/null || true
    
    export PATH="$HOME/bin:${PATH}"
    export SETD_DIR="$HOME/bin"
    export MARK_DIR="$HOME/bin"
    
    echo -e "${GREEN}✓ Build complete${NC}"
}

# Run tests for a specific shell
run_test() {
    local shell=$1
    local test_script="$SCRIPT_DIR/test_${shell}.sh"
    
    if [ ! -f "$test_script" ]; then
        echo -e "${RED}ERROR: Test script not found: $test_script${NC}"
        return 1
    fi
    
    # Determine shell interpreter
    local shell_interpreter
    case $shell in
        bash) shell_interpreter="bash" ;;
        zsh) shell_interpreter="zsh" ;;
        csh) shell_interpreter="csh" ;;
        tcsh) shell_interpreter="tcsh" ;;
        sh) shell_interpreter="sh" ;;
        dash) shell_interpreter="dash" ;;
        ksh) shell_interpreter="ksh" ;;
        fish) shell_interpreter="fish" ;;
        *)
            echo -e "${RED}ERROR: Unknown shell: $shell${NC}"
            return 1
            ;;
    esac
    
    # Check if shell is available
    if ! check_shell "$shell"; then
        echo -e "${YELLOW}⚠ Shell $shell not available, skipping...${NC}"
        return 0
    fi
    
    echo -e "${YELLOW}Testing with $shell...${NC}"
    
    # Set environment
    export SETD_DIR="$HOME/bin"
    export MARK_DIR="$HOME/bin"
    export PATH="$HOME/bin:${PATH}"
    
    # Run the test
    if "$shell_interpreter" "$test_script"; then
        echo -e "${GREEN}✓ $shell: PASSED${NC}"
        return 0
    else
        echo -e "${RED}✗ $shell: FAILED${NC}"
        return 1
    fi
}

# Main execution
OS=$(detect_os)
echo "=== mark-setd Test Suite ==="
echo "Detected OS: $OS"
echo ""

# Build project
if ! build_project; then
    echo -e "${RED}Build failed. Exiting.${NC}"
    exit 1
fi

if [ "$BUILD_ONLY" = true ]; then
    echo -e "${GREEN}Build complete. Exiting (--build-only specified).${NC}"
    exit 0
fi

# Run tests
echo ""
echo "Running tests..."
echo ""

TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

for shell in "${SHELLS[@]}"; do
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    if run_test "$shell"; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
done

# Summary
echo ""
echo "=== Test Summary ==="
echo "Total tests: $TOTAL_TESTS"
echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
if [ $FAILED_TESTS -gt 0 ]; then
    echo -e "${RED}Failed: $FAILED_TESTS${NC}"
else
    echo "Failed: $FAILED_TESTS"
fi

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
fi
