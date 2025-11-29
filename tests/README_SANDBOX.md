# Docker Sandbox Testing

This directory contains Docker-based testing tools for mark-setd SQLite migration. These tests run in complete isolation and will not affect your current mark-setd installation.

## Quick Start

### Interactive Testing

Run an interactive test container:

```bash
./tests/test_sandbox.sh
```

This will:
1. Build a Docker image with all dependencies
2. Start an interactive container
3. Give you a shell where you can test mark-setd safely

### Automated Tests

Run automated test suites:

```bash
# Test SQLite functionality
docker run -it --rm mark-setd-test:latest /bin/bash -c "source /test/tests/test_sqlite.sh"

# Test migration from old format
docker run -it --rm mark-setd-test:latest /bin/bash -c "source /test/tests/test_migration.sh"
```

## What's Tested

### SQLite Functionality (`test_sqlite.sh`)
- Creating marks
- Listing marks
- Removing marks
- Multiple database support (MARK_PATH)
- Auto-creation of databases
- setd integration

### Migration (`test_migration.sh`)
- Converting old text format to SQLite
- Backup creation (.mark_db_old)
- Verification of migrated data
- Integration with mark and setd commands

## Test Environment

The Docker container provides:
- Clean Ubuntu 22.04 environment
- All build dependencies (g++, make, sqlite3)
- Multiple shells (bash, zsh, csh, tcsh, dash, ksh, fish)
- Python 3 for migration script
- Isolated test user account
- Pre-built mark and setd binaries

## Environment Variables

Inside the container:
- `SETD_DIR=$HOME/bin` - setd database location
- `MARK_DIR=$HOME/.config/mark` - default mark database location
- `PATH` includes `$HOME/bin` for mark and setd commands

## Manual Testing

Once in the container, you can:

```bash
# Create marks
mark test
mark cloud:shared

# List marks
mark -list

# Remove marks
mark -rm test

# Navigate
cd test
cd -l  # directory history

# Inspect SQLite database
sqlite3 ~/.config/mark/.mark_db "SELECT * FROM marks;"

# Test multiple databases
export MARK_PATH="local=~/.config/mark;cloud=~/cloud/mark"
mark local:home
mark cloud:work
```

## Cleanup

The container is automatically removed when you exit (--rm flag). Test data is preserved in `test_sandbox/` directory on the host if you need to inspect it later.

## Troubleshooting

### Docker build fails
- Ensure Docker is running: `docker ps`
- Check disk space: `docker system df`
- Try rebuilding: `docker build --no-cache -f tests/Dockerfile.test -t mark-setd-test:latest .`

### Tests fail
- Check container logs: `docker logs <container-name>`
- Run interactively to debug: `./tests/test_sandbox.sh`
- Verify binaries: `which mark` and `which setd` inside container

### Permission issues
- The container runs as `testuser` with appropriate permissions
- All test directories are owned by testuser

