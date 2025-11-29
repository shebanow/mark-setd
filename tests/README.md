# mark-setd Test Suite

This directory contains a comprehensive test suite for the mark-setd utilities, testing across multiple operating systems and shell environments.

## Overview

The test suite validates the `mark` and `setd` utilities across:

- **Operating Systems**: Ubuntu (multiple versions), macOS (multiple versions), and Windows (via GitHub Actions)
- **Shells**: bash, zsh, csh, tcsh, sh, dash, ksh, and fish
- **Test Scenarios**: Directory navigation with spaces, special characters, history navigation, and mark functionality

## Prerequisites

- Make (for using Makefile targets)
- Bash (for running test scripts)
- C++ compiler (g++ or clang++)
- Shell interpreters for the shells you want to test

## Quick Start

### Run All Tests Locally

```bash
make test
# or
cd tests && ./run_tests.sh
```

### Test Specific Shell

```bash
make test-bash
make test-zsh
make test-csh
make test-tcsh
make test-sh
make test-dash
make test-ksh
make test-fish
```

Or directly:

```bash
cd tests && ./run_tests.sh -s bash
cd tests && ./run_tests.sh -s zsh
```

### Build Only

```bash
make test-build
# or
cd tests && ./run_tests.sh --build-only
```

### CI/CD Testing

For comprehensive testing across multiple OS versions, use GitHub Actions workflows (`.github/workflows/test.yml`). The workflows automatically test on:

- **Ubuntu**: Latest, 22.04, 20.04
- **macOS**: Latest, 13, 12
- **Windows**: Latest (with MSYS2/MinGW)

## Test Structure

### Directory Tree

```
tests/
├── create_test_tree.sh   # Script to create test directory tree
├── test_*.sh            # Test scripts for each shell
├── test_windows.sh      # Windows-specific test script
├── run_tests.sh         # Main test orchestration script (local testing)
└── README.md            # This file
```

### Test Directory Tree

The test suite creates a complex directory tree with:

- **Spaces**: `My Project`, `unit tests`, `integration tests`
- **Dashes**: `dir-with-dash`, `sub-dir`
- **Underscores**: `dir_with_underscore`
- **Dots**: `dir.with.dot`, `nested.dir`
- **Special Characters**: `@`, `#`, `$`, `%`, `&`, `*`, `()`, `[]`, `{}`, `~`
- **Unicode**: Chinese, Cyrillic, and Japanese characters (if supported)
- **Nested Structures**: Mixed character types in nested directories

### Test Scenarios

Each shell test script validates:

1. **Basic Navigation**: `cd` to directories with spaces, dashes, dots
2. **Nested Navigation**: `cd` to nested directories with special characters
3. **History Listing**: `cd -l` to view directory history
4. **History Navigation**: `cd -1`, `cd -2`, etc. to navigate back through history
5. **Mark Functionality**: Mark directories and navigate using marks
6. **Special Characters**: Handle directories with various special characters

## CI/CD Testing

GitHub Actions workflows (`.github/workflows/test.yml`) provide comprehensive testing across:

- **Ubuntu**: Multiple versions (latest, 22.04, 20.04)
- **macOS**: Multiple versions (latest, 13, 12)
- **Windows**: Latest with MSYS2/MinGW

Each workflow:
- Installs required dependencies
- Builds the project
- Runs tests for multiple shells
- Reports results automatically

## Test Scripts

### Shell-Specific Tests

Each shell has its own test script that:

1. Sets up the environment (SETD_DIR, MARK_DIR, PATH)
2. Sources the appropriate shell configuration (SETD_BASH or SETD_CSHRC)
3. Creates the test directory tree
4. Runs a series of test cases
5. Reports pass/fail status

### Available Shells

| Shell | Script | Interpreter | Notes |
|-------|--------|-------------|-------|
| bash | `test_bash.sh` | `/bin/bash` | Full feature support |
| zsh | `test_zsh.sh` | `/bin/zsh` | Compatible with bash scripts |
| csh | `test_csh.sh` | `/bin/csh` | Uses SETD_CSHRC |
| tcsh | `test_tcsh.sh` | `/bin/tcsh` | Enhanced csh |
| sh | `test_sh.sh` | `/bin/sh` | POSIX-compliant |
| dash | `test_dash.sh` | `/bin/dash` | Lightweight POSIX shell |
| ksh | `test_ksh.sh` | `/bin/ksh` | Korn shell |
| fish | `test_fish.sh` | `/usr/bin/fish` | Fish shell (different syntax) |

## Running Tests Manually

### Run a Single Test

```bash
# Build first
make clean && make all

# Set environment
export SETD_DIR=$HOME/bin
export MARK_DIR=$HOME/bin
export PATH="$HOME/bin:$PATH"

# Run a specific test
cd tests
bash test_bash.sh
zsh test_zsh.sh
```

### Interactive Testing

```bash
# Build and install
make clean && make all
mkdir -p $HOME/bin
cp setd $HOME/bin/
cp mark $HOME/bin/
export SETD_DIR=$HOME/bin
export MARK_DIR=$HOME/bin
export PATH="$HOME/bin:$PATH"

# Source shell configuration
source SETD_BASH  # or SETD_CSHRC for csh/tcsh

# Test manually
cd /tmp/test_tree
cd "My Project"
cd -l
cd -1
```

## Cleaning Up

### Remove Test Artifacts

```bash
make test-clean
# or manually:
rm -rf /tmp/test_tree
rm -f $HOME/bin/setd $HOME/bin/mark
```

## Troubleshooting

### Shell Not Available

Some shells may not be available on your system. The test runner will skip unavailable shells automatically. Install missing shells:

**Ubuntu/Debian:**
```bash
sudo apt-get install bash zsh dash ksh csh tcsh fish
```

**macOS:**
```bash
brew install bash zsh dash ksh tcsh fish
```

### Permission Issues

Ensure test scripts are executable:
```bash
chmod +x tests/*.sh
```

### Build Issues

- Ensure you have a C++ compiler (g++ or clang++)
- Check that make is installed
- Verify C++14 support in your compiler

### Test Failures

If a test fails:
1. Check the test output for specific error messages
2. Run the test manually to see detailed output
3. Verify the shell configuration is properly sourced
4. Check that the test directory tree was created correctly
5. Ensure SETD_DIR and MARK_DIR are set correctly

## Continuous Integration

The test suite includes comprehensive GitHub Actions workflows (`.github/workflows/test.yml`) that:

- Test on multiple OS versions automatically
- Test multiple shells per OS
- Run on every push and pull request
- Provide detailed test results

View workflow status in the GitHub Actions tab of your repository.

## Windows Testing

Windows testing is supported via GitHub Actions Windows runners. The test suite includes:

- **GitHub Actions Workflow**: `.github/workflows/test.yml` includes Windows testing
- **Windows Test Script**: `test_windows.sh` for Windows-specific testing
- **MSYS2/MinGW**: Uses MSYS2 environment for compilation and testing

### Running Windows Tests Locally

If you have MSYS2 installed:

```bash
# In MSYS2 terminal
cd /path/to/mark-setd
make clean
make all
cd tests
bash test_windows.sh
```

### GitHub Actions

The GitHub Actions workflow automatically:
1. Sets up MSYS2/MinGW on Windows runners
2. Compiles the project
3. Runs Windows-specific tests
4. Tests across multiple shells (bash, zsh, etc.)

GitHub Actions provides native Windows runners, making Windows testing straightforward without Docker.

## Contributing

When adding new tests:

1. Follow the existing test script structure
2. Add comprehensive test cases
3. Update this README if adding new OS or shell support
4. Ensure tests are idempotent (can be run multiple times)

## License

Same as the main project license.

