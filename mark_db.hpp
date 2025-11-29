#ifndef MARK_DB_HPP
#define MARK_DB_HPP

#include <string>
#include <vector>
#include <memory>

/**
 * MarkEntry class - represents a single mark entry
 * 
 * A mark entry associates an alphanumeric alias (mark) with a directory path.
 */
class MarkEntry {
private:
    const std::string _mark;  // The alphanumeric alias name for this mark (e.g., "proj", "home")
                              // Immutable once created - to rename a mark, create a new entry
    std::string _path;        // The absolute directory path this mark points to

public:
    MarkEntry(const std::string& m, const std::string& p)
        : _mark(m), _path(p) {}
    
    // Getters
    const std::string& mark() const { return _mark; }
    const std::string& path() const { return _path; }
    
    // Setters
    void setPath(const std::string& p) { _path = p; }
};

/**
 * MarkDatabase class - manages a single SQLite mark database file
 */
class MarkDatabase {
private:
    struct sqlite3* db;
    std::string dbPath;  // Full path to .mark_db SQLite file
    int maxMarkSize;

    bool createSchema();
    bool loadMarks();
    void sortMarks();

public:
    MarkDatabase();
    ~MarkDatabase();

    // Initialize with a directory path (will use directory/.mark_db)
    // If createIfMissing is true, creates the database if it doesn't exist
    bool initialize(const std::string& directory, bool createIfMissing = false);
    
    bool addMark(const std::string& mark, const std::string& path);
    bool removeMark(const std::string& mark);
    bool resetMarks();
    bool refreshMarks();
    bool listMarks();
    
    std::string getMarkPath(const std::string& mark) const;
    std::string getDbPath() const { return dbPath; }
    
    // Utility methods
    static bool isValidMarkName(const std::string& mark);
    static std::string escapePath(const std::string& path);
    static std::string unescapePath(const std::string& path);
};

/**
 * MarkDatabaseManager class - manages multiple mark databases with search path
 */
class MarkDatabaseManager {
private:
    struct DatabaseEntry {
        std::string alias;  // Empty if no alias (direct path)
        std::string path;
        std::unique_ptr<MarkDatabase> db;
    };
    
    std::vector<DatabaseEntry> databases;
    
    void parseMarkPath(const std::string& markPath);
    std::string expandPath(const std::string& path);

public:
    MarkDatabaseManager();
    ~MarkDatabaseManager();
    
    // Initialize from MARK_PATH environment variable
    bool initialize();
    
    // Find database by alias or path
    MarkDatabase* findDatabase(const std::string& dbSpec);
    
    // Get default (first) database
    MarkDatabase* getDefaultDatabase();
    
    // Search for mark across all databases (returns first match)
    std::string findMark(const std::string& markName, bool warnDuplicates = false);
    
    // Get all databases in priority order
    const std::vector<DatabaseEntry>& getDatabases() const { return databases; }
};

#endif // MARK_DB_HPP

