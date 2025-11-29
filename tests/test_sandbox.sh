#!/bin/bash
#
# Docker-based sandbox testing for mark-setd SQLite migration
# This script builds a Docker container and runs tests in isolation
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
CONTAINER_NAME="mark-setd-test-$$"

echo "=========================================="
echo "mark-setd SQLite Migration Test Sandbox"
echo "=========================================="
echo ""

# Build Docker image
echo "Building Docker test image..."
cd "$PROJECT_ROOT"
docker build -f tests/Dockerfile.test -t mark-setd-test:latest .

if [ $? -ne 0 ]; then
    echo "ERROR: Docker build failed"
    exit 1
fi

echo "Docker image built successfully"
echo ""

# Create test directories on host for volume mounting (optional)
TEST_DIR="$PROJECT_ROOT/test_sandbox"
mkdir -p "$TEST_DIR"

# Run interactive test container
echo "Starting test container: $CONTAINER_NAME"
echo "You can now test mark-setd in an isolated environment"
echo ""
echo "Test commands:"
echo "  mark test          - Create a test mark"
echo "  mark -list         - List all marks"
echo "  mark cloud:shared  - Create mark in cloud database"
echo "  mark -rm test      - Remove a mark"
echo "  cd test            - Navigate using mark"
echo "  sqlite3 ~/.config/mark/.mark_db 'SELECT * FROM marks;' - Inspect database"
echo ""
echo "Type 'exit' when done testing"
echo ""

# Run container with interactive shell
docker run -it --rm \
    --name "$CONTAINER_NAME" \
    -v "$PROJECT_ROOT:/source:ro" \
    -v "$TEST_DIR:/test_data" \
    mark-setd-test:latest \
    /bin/bash -c "
        cd /test
        echo 'Environment setup:'
        echo '  SETD_DIR=\$SETD_DIR'
        echo '  MARK_DIR=\$MARK_DIR'
        echo '  MARK_REMOTE_DIR=\$MARK_REMOTE_DIR'
        echo '  MARK_PATH=\$MARK_PATH'
        echo '  PATH=\$PATH'
        echo ''
        echo 'Source files available in /source'
        echo 'Test data directory: /test_data'
        echo ''
        echo 'cd command is aliased to use setd'
        echo 'Cloud database directory: \$MARK_REMOTE_DIR'
        echo ''
        # Source .bashrc to get cd alias
        source ~/.bashrc
        /bin/bash
    "

echo ""
echo "Test container stopped"
echo "Test data preserved in: $TEST_DIR"

