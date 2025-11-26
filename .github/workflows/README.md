# GitHub Actions Workflows

This directory contains GitHub Actions workflows for automated testing of mark-setd.

## Workflows

### test.yml

Main test workflow that runs on:
- **Ubuntu**: Latest, 22.04, 20.04 (with multiple shells)
- **Debian**: Similar to Ubuntu (tested on Ubuntu runners)
- **macOS**: Latest, 13, 12 (with multiple shells)
- **Windows**: Latest (using MSYS2/MinGW, with bash and zsh)
- **Special Shells**: csh, tcsh, ksh, fish (tested on Ubuntu)

**Triggers:**
- Push to `main` or `develop` branches
- Pull requests to `main` or `develop` branches
- Manual workflow dispatch

**Test Matrix:**
- Multiple OS versions × Multiple shells = Comprehensive coverage
- Each combination runs in parallel for fast results

## Windows Testing

Windows testing uses GitHub Actions Windows runners with MSYS2/MinGW:

1. **MSYS2 Setup**: Automatically installs MinGW-w64 toolchain
2. **Compilation**: Builds mark and setd using g++ in MSYS2 environment
3. **Testing**: Runs test suite using bash in MSYS2

### Why GitHub Actions for Windows?

- **Native Windows Environment**: Full Windows filesystem and path handling
- **MSYS2 Integration**: Easy access to Unix-like tools
- **No Container Limitations**: Avoids Windows container complexity
- **CI/CD Ready**: Integrated with GitHub's infrastructure

### Local Windows Testing

To test locally on Windows:

1. Install MSYS2 from https://www.msys2.org/
2. Open MSYS2 terminal
3. Install dependencies:
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
   ```
4. Build and test:
   ```bash
   cd /path/to/mark-setd
   make clean
   make all
   cd tests
   bash test_windows.sh
   ```

## Workflow Status

View workflow status in the GitHub Actions tab of your repository.

## Customization

To customize workflows:

1. Edit `.github/workflows/test.yml` for main testing
2. Edit `.github/workflows/test-windows-only.yml` for Windows-only testing
3. Adjust matrix strategies to test different OS/shell combinations
4. Add additional test steps as needed

## Dependencies

Workflows automatically install:
- **Linux**: build-essential, g++, make, multiple shells
- **Windows**: MSYS2, MinGW-w64, gcc, make
- **macOS**: Homebrew packages (bash, zsh, etc.)

## Notes

- Windows tests normalize paths (backslashes to forward slashes) for compatibility
- Some shells may not be available on all platforms (tests skip gracefully)
- All tests run on native GitHub Actions runners for fast, reliable results
- No Docker required - uses native OS environments

