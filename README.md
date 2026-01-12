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
- **SQLite database backend**: Fast, transactional mark storage with atomic updates
- **Multiple database support**: Use `MARK_PATH` to define a search path of mark databases
- **Database aliases**: Name your databases (e.g., `local=~/.config/mark;cloud=/gdrive/mark`)
- **Auto-creation**: Databases are automatically created when first used
- **Modern C++ implementation**: Improved performance and maintainability
- **Better space handling**: Properly handles spaces in directory names when escaped with backslashes

## Installation

### Prerequisites
- C++ compiler with C++14 support (g++ or clang++)
- Make
- SQLite3 development libraries (`libsqlite3-dev` on Debian/Ubuntu, `sqlite3` on macOS)
- Unix-like operating system (Linux, macOS, etc.)
- Python 3 (for migration script, if upgrading from older version)

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

**Note:** The `MARK_DIR` environment variable specifies the directory where the `.mark_db` SQLite database file will be stored. You can set it to any directory you prefer (e.g., `$HOME/.config/mark`, `$HOME/.local/share/mark`, or `$HOME/bin`). The filename is always `.mark_db` (SQLite format).

## Usage

### Marking Directories

```bash
# Mark current directory (uses default database from MARK_DIR or first in MARK_PATH)
mark myproject

# Mark in a specific database using alias
mark cloud:myproject

# Mark in a specific database using directory path
mark /path/to/db:myproject

# Mark with cloud sync (backward compatibility - maps to cloud:mark)
mark -c myproject

# List all marks from all databases
mark -list

# Remove a mark
mark -rm myproject

# Reset all marks in default database
mark -reset
```

### Multiple Database Support

You can define multiple mark databases using the `MARK_PATH` environment variable:

```bash
# Define a search path with aliases
export MARK_PATH="local=~/.config/mark;cloud=/gdrive/user/mark;work=~/work/marks"

# Now you can use:
mark local:home      # Store in local database
mark cloud:shared    # Store in cloud database
mark work:project    # Store in work database

# setd searches databases in order (first match wins)
cd home              # Uses first database that has "home" mark
```

If `MARK_PATH` is not set, the system falls back to `MARK_DIR` (local) and `MARK_REMOTE_DIR` (cloud) for backward compatibility.

### Migration from Old Format

If you're upgrading from a previous version that used text-based `.mark_db` files, use the migration script:

```bash
# Make sure MARK_DIR (and MARK_REMOTE_DIR if used) are set
export MARK_DIR=$HOME/.config/mark
export MARK_REMOTE_DIR=/gdrive/user/mark  # if applicable

# Run migration script
./mark_migration
```

The migration script will:
1. Create backups of old files (`.mark_db_old`)
2. Convert all marks to SQLite format
3. Preserve old files until you verify the migration worked

**Note:** The migration script requires Python 3 and the `mark` command to be in your PATH.

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

- `$MARK_DIR/.mark_db` - Local mark database (SQLite format, location configurable via `$MARK_DIR` environment variable)
- `$MARK_REMOTE_DIR/.mark_db` - Remote/cloud mark database (SQLite format, optional, location configurable via `$MARK_REMOTE_DIR` environment variable)
- `$SETD_DIR/setd_db` - Directory queue database (text format)

**Note:** 
- All `.mark_db` files are SQLite databases (binary format, but can be inspected with `sqlite3` command)
- When searching for marks, `setd` checks databases in `MARK_PATH` order (or `MARK_DIR` then `MARK_REMOTE_DIR` if `MARK_PATH` is not set)
- First match wins - local marks take precedence over remote marks with the same name
- Databases are automatically created when first used if they don't exist

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

- **MarkDatabase**: Manages a single SQLite mark database file
- **MarkDatabaseManager**: Manages multiple mark databases with search path support
- **MarkEntry**: Represents a single mark entry (in-memory representation)
- **SetdDatabase**: Manages the directory queue
- **DirectoryQueueEntry**: Represents a directory in the queue

### Database Format

Marks are stored in SQLite databases with the following schema:
- `marks` table: `id`, `name` (unique), `path`, `created_at`, `updated_at`
- Index on `name` for fast lookups
- Atomic transactions for safe concurrent access

## License

Copyright (c) 2025 Michael Shebanow and Sunil William Savkar

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

Contributions are welcome! Please feel free to submit a Pull Request or open an issue for discussion.

## Support

For issues and questions, please open an issue in the repository or contact the maintainers.

