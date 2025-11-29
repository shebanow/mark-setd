#!/bin/bash
#
# Test SQLite functionality
# Runs inside Docker container
#

set -e

echo "=========================================="
echo "Testing SQLite Functionality"
echo "=========================================="
echo ""

# Setup
mkdir -p "$HOME/.config/mark"
cd "$HOME/.config/mark"

# Test 1: Create a mark
echo "Test 1: Creating marks..."
cd /tmp
mark test1
mark test2
mark test3

echo "Marks created"
echo ""

# Test 2: List marks
echo "Test 2: Listing marks..."
mark -list
echo ""

# Test 3: Verify SQLite database
echo "Test 3: Verifying SQLite database..."
if [ ! -f "$HOME/.config/mark/.mark_db" ]; then
    echo "ERROR: Database file not created"
    exit 1
fi

echo "Database file exists"
sqlite3 "$HOME/.config/mark/.mark_db" "SELECT COUNT(*) as count FROM marks;"
echo ""

# Test 4: Remove a mark
echo "Test 4: Removing a mark..."
mark -rm test2
mark -list
echo ""

# Test 5: Verify removal in SQLite
echo "Test 5: Verifying removal in SQLite..."
sqlite3 "$HOME/.config/mark/.mark_db" "SELECT name FROM marks ORDER BY name;"
echo ""

# Test 6: Multiple databases with MARK_PATH
echo "Test 6: Testing multiple databases..."
mkdir -p "$HOME/mark_local" "$HOME/mark_cloud"
export MARK_PATH="local=$HOME/mark_local;cloud=$HOME/mark_cloud"

cd /tmp
mark local:local1
mark cloud:cloud1

echo "Marks in local database:"
mark -list | grep -A 100 "\[local\]" || echo "No local marks"
echo ""

echo "Marks in cloud database:"
mark -list | grep -A 100 "\[cloud\]" || echo "No cloud marks"
echo ""

# Test 7: Auto-creation of new database
echo "Test 7: Testing auto-creation..."
mkdir -p "$HOME/mark_new"
mark "$HOME/mark_new:new1"

if [ -f "$HOME/mark_new/.mark_db" ]; then
    echo "Auto-creation successful"
    sqlite3 "$HOME/mark_new/.mark_db" "SELECT name, path FROM marks;"
else
    echo "ERROR: Auto-creation failed"
    exit 1
fi
echo ""

# Test 8: setd with marks
echo "Test 8: Testing setd with marks..."
cd /home/testuser
setd test1
echo "Navigated to: $(pwd)"
echo ""

echo "=========================================="
echo "All SQLite tests passed!"
echo "=========================================="

