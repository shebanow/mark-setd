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
export SETD_DIR=$HOME/bin          # Directory for setd database
export MARK_DIR=$HOME/.config/mark # Directory for mark database (can be anywhere)
source /path/to/SETD_BASH
```

**For Csh/Tcsh:**
```csh
setenv SETD_DIR ~/bin
setenv MARK_DIR ~/.config/mark
source /path/to/SETD_CSHRC
```

**Note:** The `MARK_DIR` environment variable specifies where the `.mark_db` file will be stored. You can set it to any directory you prefer (e.g., `$HOME/.config/mark`, `$HOME/.local/share/mark`, or `$HOME/bin`). The filename is always `.mark_db`.

## Usage

### Marking Directories

```bash
# Mark current directory
mark myproject

# Mark with cloud sync (universal mark - requires MARK_REMOTE_DIR to be set)
mark -c myproject

# List all marks
mark -list

# Remove a mark
mark -rm myproject

# Reset all marks
mark -reset
```

### Cloud/Remote Marks Setup

To use cloud-synced marks, set the `MARK_REMOTE_DIR` environment variable to point to your mounted cloud drive directory:

```bash
# Example: Google Drive (mounted)
export MARK_REMOTE_DIR=/Users/username/Google\ Drive/mark-setd

# Example: OneDrive (mounted)
export MARK_REMOTE_DIR=/Users/username/OneDrive/mark-setd

# Example: Dropbox (mounted)
export MARK_REMOTE_DIR=/Users/username/Dropbox/mark-setd
```

**Note:** The cloud drive must be pre-mounted on your local filesystem. The `MARK_REMOTE_DIR` should point to a directory where you want the remote `.mark_db` file stored.

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

# Mark with cloud sync (accessible from other machines - requires MARK_REMOTE_DIR)
mark -c proj

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

- `$MARK_DIR/.mark_db` - Local mark database (location configurable via `$MARK_DIR` environment variable)
- `$MARK_REMOTE_DIR/.mark_db` - Remote/cloud mark database (optional, location configurable via `$MARK_REMOTE_DIR` environment variable)
- `$SETD_DIR/setd_db` - Directory queue database

**Note:** When searching for marks, `setd` checks both `$MARK_DIR/.mark_db` (local) and `$MARK_REMOTE_DIR/.mark_db` (remote). Local marks take precedence over remote marks with the same name.

## Space Handling

The utilities now properly handle spaces in directory names when escaped with backslashes:

```bash
# These work correctly:
cd "My Project"
cd My\ Project
mark -c "My Project"
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

## Historical Context

The `mark` and `setd` utilities were originally created by Sunil William Savkar in 1991, providing directory marking and navigation capabilities that were ahead of their time. These tools introduced two key concepts that have since become standard in modern shell environments:

### The Original Vision (1991-1993)

The original `mark` and `setd` tools provided:
- **Named directory bookmarks**: Mark frequently used directories with short aliases (`mark myalias`)
- **Directory stack navigation**: Navigate through a history of visited directories (`cd -l`, `cd -N`)
- **Persistent directory history**: Maintain a queue of recently visited locations

This functionality was revolutionary at a time when most shells lacked these features, making directory navigation significantly more efficient for power users.

### Modern Shell Adoption

Over the decades since their creation, many of these features have been adopted into mainstream shells:

- **Zsh** (best native match): Implements both named directories (`hash -d`) and directory stacks (`cd -N`, `dirs -v`) natively, closely matching the original mark/setd workflow
- **Bash**: Has directory stacks (`pushd/popd/dirs`) but requires third-party tools like `bashmarks` or `zoxide` for named bookmarks
- **Fish**: Provides directory history and named directory variables, though with a different style
- **tcsh/csh**: Has had directory aliases since the 1980s, making it historically closest to the original tools

### Why Modernize mark-setd?

Despite modern shells incorporating similar features, the original mark/setd tools offer several advantages:

1. **Cross-shell compatibility**: Works consistently across bash, zsh, csh, tcsh, and other shells
2. **Unified interface**: Single set of commands regardless of your shell
3. **Cloud synchronization**: Version 2.0 adds cloud-based universal marks, allowing marks to sync across machines
4. **Legacy support**: Maintains the familiar workflow for users who have relied on these tools for over 30 years
5. **Lightweight**: Simple, focused tools without shell-specific dependencies

### The Evolution to Version 2.0

This modern C++ implementation (Version 2.0) preserves the original functionality while adding:
- Cloud-based mark synchronization (OneDrive, Google Drive, Dropbox, iCloud)
- Improved performance through modern C++ implementation
- Better handling of spaces and special characters in directory names
- Enhanced database management for marks and directory queues
- Comprehensive test suite across multiple operating systems and shells

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

