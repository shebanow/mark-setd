# mark-setd

A modern C++ implementation of directory marking and navigation utilities, originally based on the work by Sunil William Savkar.

## Overview

`mark-setd` provides powerful directory navigation capabilities through two main utilities:

- **mark**: Mark frequently used directories with short aliases
- **setd**: Enhanced `cd` command that understands marks, environment variables, and directory history

## Features

### Core Functionality
- Mark directories with short, memorable aliases
- Navigate using marks, environment variables, or directory history
- Queue-based directory history with configurable depth
- Support for subdirectory navigation (e.g., `cd mark_name/subdir`)

### New in Version 2.0
- **Cloud-based universal marks**: Share marks across machines via cloud storage (OneDrive, Google Drive, Dropbox, iCloud)
- **Modern C++ implementation**: Improved performance and maintainability
- **Better space handling**: Properly handles spaces in directory names when escaped with backslashes
- **Enhanced database management**: More efficient mark and queue storage

## Installation

### Prerequisites
- C++ compiler with C++11 support (g++ or clang++)
- Make
- Unix-like operating system (Linux, macOS, etc.)

### Build Instructions

```bash
make clean
make all
```

### Setup

Add to your shell configuration file (`.bashrc`, `.zshrc`, or `.cshrc`):

**For Bash/Zsh:**
```bash
export SETD_DIR=$HOME/bin
export MARK_DIR=$HOME/bin
source /path/to/SETD_BASH
```

**For Csh/Tcsh:**
```csh
setenv SETD_DIR ~/bin
setenv MARK_DIR ~/bin
source /path/to/SETD_CSHRC
```

## Usage

### Marking Directories

```bash
# Mark current directory
mark myproject

# Mark with cloud sync (universal mark)
mark -cl myproject

# List all marks
mark -list

# Remove a mark
mark -rm myproject

# Reset all marks
mark -reset
```

### Cloud Storage Setup

```bash
# Setup cloud storage for universal marks
mark -cloud-setup onedrive /path/to/onedrive
mark -cloud-setup googledrive /path/to/google/drive
mark -cloud-setup dropbox /path/to/dropbox
mark -cloud-setup icloud /path/to/icloud
```

### Navigating Directories

```bash
# Change to a marked directory
cd myproject

# Change to subdirectory of a mark
cd myproject/src

# Use environment variable
cd $HOME

# Navigate using queue offset
cd -1    # Previous directory
cd -2    # Two directories back

# Search queue by partial path
cd @src  # Find directory ending with "src"

# Same-level navigation
cd %bin  # ../bin
```

### Directory History

```bash
# List directory queue
cd -list

# Set maximum queue depth
cd -max 20
```

## Examples

```bash
# Mark your project directory
cd ~/projects/my-awesome-project
mark proj

# Mark with cloud sync (accessible from other machines)
mark -cl proj

# Navigate using the mark
cd proj

# Navigate to subdirectory
cd proj/src/components

# Go back to previous directory
cd -1

# List all marks
mark -list
```

## Files

- `$MARK_DIR/mark_db` - Local mark database
- `$MARK_DIR/mark_cloud_config` - Cloud storage configuration
- `$SETD_DIR/setd_db` - Directory queue database
- `$CLOUD_PATH/mark-setd/mark_db` - Cloud-synced mark database (if configured)

## Space Handling

The utilities now properly handle spaces in directory names when escaped with backslashes:

```bash
# These work correctly:
cd "My Project"
cd My\ Project
mark -cl "My Project"
```

## Architecture

### Classes

- **MarkDatabase**: Manages the mark database with support for local and cloud marks
- **MarkEntry**: Represents a single mark entry
- **CloudStorage**: Handles cloud storage operations
- **SetdDatabase**: Manages the directory queue
- **DirectoryQueueEntry**: Represents a directory in the queue

## License

Copyright (c) 2024 Michael Shebanow and Sunil William Savkar

This software is licensed under a proprietary license. All rights reserved.

This software may not be copied, modified, or distributed without explicit permission from the copyright holders.

## Credits

**Original Author**: Sunil William Savkar (sunil@hal.com)  
**Original Copyright**: Copyright (c) 1991 Sunil William Savkar

This project is based on the original mark/setd utilities by Sunil William Savkar, modernized and enhanced with cloud support and improved performance.

## Version

Current version: 2.0

## Testing

The project includes a comprehensive test suite that runs on multiple operating systems and shells:

### Local Testing

```bash
# Run all tests
make test

# Test specific OS
make test-debian
make test-ubuntu

# Test specific shell
make test-bash
make test-zsh
```

See `tests/README.md` for detailed testing documentation.

### CI/CD Testing

GitHub Actions workflows automatically test the project on:
- **Linux**: Ubuntu (multiple versions)
- **Windows**: Windows Latest (via MSYS2/MinGW)
- **macOS**: macOS Latest
- **Docker**: Multiple Linux distributions

Workflows run on push and pull requests. See `.github/workflows/` for workflow definitions.

## Contributing

This is currently a private project. Future contributions may be accepted when the project becomes public.

## Support

For issues and questions, please open an issue in the repository (when public) or contact the maintainers.

