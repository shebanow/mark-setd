# Quick Start: Docker Sandbox Testing

## Prerequisites
- Docker installed and running
- No impact on your current mark-setd installation

## Step 1: Build and Enter Test Container

```bash
cd /path/to/mark-setd
./tests/test_sandbox.sh
```

This will:
1. Build the Docker image (first time only, ~2-3 minutes)
2. Start an interactive container
3. Give you a bash shell

## Step 2: Test Basic Functionality

Inside the container:

```bash
# Create some test marks
cd /tmp
mark test1
mark test2

# List marks
mark -list

# Navigate using marks
cd test1
pwd  # Should show /tmp

# Remove a mark
mark -rm test2
mark -list
```

## Step 3: Test SQLite Database

```bash
# Inspect the SQLite database
sqlite3 ~/.config/mark/.mark_db "SELECT * FROM marks;"

# Check database schema
sqlite3 ~/.config/mark/.mark_db ".schema"
```

## Step 4: Test Multiple Databases

```bash
# Set up multiple databases
mkdir -p ~/mark_local ~/mark_cloud
export MARK_PATH="local=~/mark_local;cloud=~/mark_cloud"

# Create marks in different databases
cd /tmp
mark local:local1
mark cloud:cloud1

# List all marks
mark -list
```

## Step 5: Test Migration (Optional)

```bash
# Run automated migration test
source /test/tests/test_migration.sh
```

## Step 6: Exit Container

```bash
exit
```

The container is automatically removed when you exit.

## Running Automated Tests

Instead of interactive testing, you can run automated test suites:

```bash
# Build image first
docker build -f tests/Dockerfile.test -t mark-setd-test:latest .

# Run SQLite tests
docker run -it --rm mark-setd-test:latest /bin/bash -c "source /test/tests/test_sqlite.sh"

# Run migration tests
docker run -it --rm mark-setd-test:latest /bin/bash -c "source /test/tests/test_migration.sh"
```

## Troubleshooting

**Docker not running?**
```bash
docker ps  # Should not error
```

**Build fails?**
```bash
docker system prune -a  # Clean up (optional)
docker build --no-cache -f tests/Dockerfile.test -t mark-setd-test:latest .
```

**Can't find mark command?**
```bash
which mark  # Should show /home/testuser/bin/mark
echo $PATH  # Should include /home/testuser/bin
```

## What's Safe?

✅ **Safe to test:**
- All mark operations
- Database creation/deletion
- Migration scripts
- Multiple database configurations

❌ **Won't affect:**
- Your host system's mark-setd installation
- Your host system's mark databases
- Your host system's environment variables

The container is completely isolated!

