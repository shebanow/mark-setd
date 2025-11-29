#!/bin/bash
#
# Test migration from old text format to SQLite
# Runs inside Docker container
#

set -e

echo "=========================================="
echo "Testing Migration from Text to SQLite"
echo "=========================================="
echo ""

# Create test mark directory
mkdir -p "$HOME/.config/mark"
cd "$HOME/.config/mark"

# Create old-style text mark database
echo "Creating old-style text mark database..."
cat > .mark_db << 'EOF'
setenv mark_home /home/testuser
setenv mark_tmp /tmp
setenv mark_root /root
setenv mark_test /home/testuser/test
EOF

echo "Old database created with 4 marks"
echo ""

# Verify old format
echo "Old database contents:"
cat .mark_db
echo ""

# Run migration
echo "Running migration script..."
cd /test
python3 mark_migration

if [ $? -ne 0 ]; then
    echo "ERROR: Migration failed"
    exit 1
fi

echo ""
echo "Migration completed"
echo ""

# Verify SQLite database exists
if [ ! -f "$HOME/.config/mark/.mark_db" ]; then
    echo "ERROR: SQLite database not created"
    exit 1
fi

# Check if it's actually SQLite
if ! file "$HOME/.config/mark/.mark_db" | grep -q "SQLite"; then
    echo "ERROR: Database is not SQLite format"
    exit 1
fi

echo "SQLite database created successfully"
echo ""

# Verify backup was created
if [ ! -f "$HOME/.config/mark/.mark_db_old" ]; then
    echo "WARNING: Backup file not found"
else
    echo "Backup file created: .mark_db_old"
fi
echo ""

# Query SQLite database
echo "Verifying marks in SQLite database:"
sqlite3 "$HOME/.config/mark/.mark_db" "SELECT name, path FROM marks ORDER BY name;"
echo ""

# Test mark command
echo "Testing mark command:"
mark -list
echo ""

# Test setd
echo "Testing setd with marks:"
cd /tmp
setd home
echo "Current directory: $(pwd)"
echo ""

cd /home/testuser
setd tmp
echo "Current directory: $(pwd)"
echo ""

echo "=========================================="
echo "Migration test completed successfully!"
echo "=========================================="

