/*
 * Copyright (c) 2025 Michael Shebanow and Sunil William Savkar
 * 
 * Original work:
 * Copyright (c) 1991 Sunil William Savkar. All rights reserved.
 */

#include "mark_db.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <sys/stat.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <unistd.h>

// MarkDatabase implementation
MarkDatabase::MarkDatabase() : db(nullptr), maxMarkSize(0) {
}

MarkDatabase::~MarkDatabase() {
    if (db) {
        sqlite3_close(db);
    }
}

bool MarkDatabase::isValidMarkName(const std::string& mark) {
    if (mark.empty()) return false;
    for (char c : mark) {
        if (!std::isalnum(c) && c != '_' && c != '.') {
            return false;
        }
    }
    return true;
}

std::string MarkDatabase::escapePath(const std::string& path) {
    std::string escaped;
    for (char c : path) {
        if (c == ' ') {
            escaped += "\\ ";
        } else if (c == '\\') {
            escaped += "\\\\";
        } else {
            escaped += c;
        }
    }
    return escaped;
}

std::string MarkDatabase::unescapePath(const std::string& path) {
    std::string unescaped;
    for (size_t i = 0; i < path.length(); ++i) {
        if (path[i] == '\\' && i + 1 < path.length()) {
            if (path[i + 1] == ' ') {
                unescaped += ' ';
                i++;
            } else if (path[i + 1] == '\\') {
                unescaped += '\\';
                i++;
            } else {
                unescaped += path[i];
            }
        } else {
            unescaped += path[i];
        }
    }
    return unescaped;
}

bool MarkDatabase::createSchema() {
    const char* sql = 
        "CREATE TABLE IF NOT EXISTS marks ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT UNIQUE NOT NULL,"
        "  path TEXT NOT NULL,"
        "  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_marks_name ON marks(name);";
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "createSchema: SQL error: " << (errMsg ? errMsg : "unknown") << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool MarkDatabase::loadMarks() {
    const char* sql = "SELECT name, path FROM marks ORDER BY name";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "loadMarks: Failed to prepare statement" << std::endl;
        return false;
    }
    
    maxMarkSize = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        
        if (name && path) {
            std::string markName(name);
            std::string markPath(path);
            
            if (markName.length() > static_cast<size_t>(maxMarkSize)) {
                maxMarkSize = markName.length();
            }
        }
    }
    
    sqlite3_finalize(stmt);
    return true;
}

void MarkDatabase::sortMarks() {
    // Marks are sorted in SQL queries, no need to sort in memory
}

bool MarkDatabase::initialize(const std::string& directory, bool createIfMissing) {
    // Ensure directory exists
    struct stat st;
    if (stat(directory.c_str(), &st) != 0) {
        // Directory doesn't exist, try to create it
        std::string cmd = "mkdir -p \"" + directory + "\"";
        if (system(cmd.c_str()) != 0) {
            std::cerr << "initialize: Failed to create directory: " << directory << std::endl;
            return false;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        std::cerr << "initialize: Path exists but is not a directory: " << directory << std::endl;
        return false;
    }
    
    // Build full path: directory/.mark_db
    dbPath = directory;
    if (dbPath.back() != '/') {
        dbPath += "/";
    }
    dbPath += ".mark_db";
    
    // Check if file exists
    bool exists = (access(dbPath.c_str(), F_OK) == 0);
    
    if (!exists && !createIfMissing) {
        // Database doesn't exist and we're not allowed to create it
        return false;
    }
    
    // Open or create database
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "initialize: Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        db = nullptr;
        return false;
    }
    
    // Create schema if new database
    if (!exists || createIfMissing) {
        if (!createSchema()) {
            sqlite3_close(db);
            db = nullptr;
            return false;
        }
    }
    
    // Load marks to calculate maxMarkSize
    loadMarks();
    
    return true;
}

bool MarkDatabase::addMark(const std::string& mark, const std::string& path) {
    if (!isValidMarkName(mark)) {
        std::cerr << "addMark: mark must be alphanumeric" << std::endl;
        return false;
    }
    
    if (!db) {
        std::cerr << "addMark: Database not initialized" << std::endl;
        return false;
    }
    
    // Use INSERT OR REPLACE to handle updates
    const char* sql = "INSERT OR REPLACE INTO marks (name, path, updated_at) VALUES (?, ?, CURRENT_TIMESTAMP)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "addMark: Failed to prepare statement" << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, mark.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, path.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "addMark: Failed to execute: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    // Update maxMarkSize
    if (mark.length() > static_cast<size_t>(maxMarkSize)) {
        maxMarkSize = mark.length();
    }
    
    std::cerr << "addMark: mark \"" << mark << "\" (" << path << ") set" << std::endl;
    return true;
}

bool MarkDatabase::removeMark(const std::string& mark) {
    if (!db) {
        std::cerr << "removeMark: Database not initialized" << std::endl;
        return false;
    }
    
    // First check if mark exists and get its path for the message
    const char* selectSql = "SELECT path FROM marks WHERE name = ?";
    sqlite3_stmt* selectStmt;
    
    if (sqlite3_prepare_v2(db, selectSql, -1, &selectStmt, nullptr) != SQLITE_OK) {
        std::cerr << "removeMark: Failed to prepare select statement" << std::endl;
        return false;
    }
    
    sqlite3_bind_text(selectStmt, 1, mark.c_str(), -1, SQLITE_STATIC);
    
    std::string markPath;
    bool found = false;
    if (sqlite3_step(selectStmt) == SQLITE_ROW) {
        const char* path = reinterpret_cast<const char*>(sqlite3_column_text(selectStmt, 0));
        if (path) {
            markPath = path;
            found = true;
        }
    }
    sqlite3_finalize(selectStmt);
    
    if (!found) {
        std::cerr << "removeMark: mark \"" << mark << "\" not found" << std::endl;
        return false;
    }
    
    // Delete the mark
    const char* deleteSql = "DELETE FROM marks WHERE name = ?";
    sqlite3_stmt* deleteStmt;
    
    if (sqlite3_prepare_v2(db, deleteSql, -1, &deleteStmt, nullptr) != SQLITE_OK) {
        std::cerr << "removeMark: Failed to prepare delete statement" << std::endl;
        return false;
    }
    
    sqlite3_bind_text(deleteStmt, 1, mark.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(deleteStmt);
    sqlite3_finalize(deleteStmt);
    
    if (rc != SQLITE_DONE) {
        std::cerr << "removeMark: Failed to execute: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    std::cerr << "removeMark: mark \"" << mark << "\" (" << markPath << ") removed" << std::endl;
    return true;
}

bool MarkDatabase::resetMarks() {
    if (!db) {
        std::cerr << "resetMarks: Database not initialized" << std::endl;
        return false;
    }
    
    const char* sql = "DELETE FROM marks";
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "resetMarks: SQL error: " << (errMsg ? errMsg : "unknown") << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    maxMarkSize = 0;
    return true;
}

bool MarkDatabase::refreshMarks() {
    // For SQLite, refresh just means reloading (no-op since we query directly)
    return true;
}

bool MarkDatabase::listMarks() {
    if (!db) {
        std::cout << std::endl;
        return true;
    }
    
    const char* sql = "SELECT name, path FROM marks ORDER BY name";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cout << std::endl;
        return true;
    }
    
    std::vector<std::pair<std::string, std::string>> markList;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        
        if (name && path) {
            markList.push_back({std::string(name), std::string(path)});
        }
    }
    
    sqlite3_finalize(stmt);
    
    if (markList.empty()) {
        std::cout << std::endl;
        return true;
    }
    
    std::cout << "MARK";
    for (int i = 0; i < maxMarkSize + 1; i++) std::cout << " ";
    std::cout << "PATH" << std::endl;
    std::cout << "----";
    for (int i = 0; i < maxMarkSize + 1; i++) std::cout << " ";
    std::cout << "----" << std::endl;
    
    for (const auto& entry : markList) {
        std::cout << entry.first << " ";
        int spaces = maxMarkSize - entry.first.length() + 3;
        for (int i = 0; i < spaces; i++) std::cout << "_";
        std::cout << " " << entry.second << std::endl;
    }
    
    return true;
}

std::string MarkDatabase::getMarkPath(const std::string& mark) const {
    if (!db) {
        return "";
    }
    
    const char* sql = "SELECT path FROM marks WHERE name = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return "";
    }
    
    sqlite3_bind_text(stmt, 1, mark.c_str(), -1, SQLITE_STATIC);
    
    std::string result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (path) {
            result = path;
        }
    }
    
    sqlite3_finalize(stmt);
    return result;
}

// MarkDatabaseManager implementation
MarkDatabaseManager::MarkDatabaseManager() {
}

MarkDatabaseManager::~MarkDatabaseManager() {
}

std::string MarkDatabaseManager::expandPath(const std::string& path) {
    std::string expanded = path;
    
    // Expand ~ to home directory
    if (expanded[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) {
            expanded = std::string(home) + expanded.substr(1);
        }
    }
    
    return expanded;
}

void MarkDatabaseManager::parseMarkPath(const std::string& markPath) {
    databases.clear();
    
    if (markPath.empty()) {
        // Fallback to MARK_DIR for backward compatibility
        const char* markDir = std::getenv("MARK_DIR");
        if (markDir) {
            DatabaseEntry entry;
            entry.alias = "";
            entry.path = expandPath(markDir);
            entry.db = std::make_unique<MarkDatabase>();
            // Auto-create default database if it doesn't exist
            if (!entry.db->initialize(entry.path, true)) {
                std::cerr << "Warning: Failed to initialize database in " << entry.path << std::endl;
                return;
            }
            databases.push_back(std::move(entry));
        }
        return;
    }
    
    // Parse semicolon-separated list
    std::istringstream iss(markPath);
    std::string item;
    
    while (std::getline(iss, item, ';')) {
        // Trim whitespace
        item.erase(0, item.find_first_not_of(" \t"));
        item.erase(item.find_last_not_of(" \t") + 1);
        
        if (item.empty()) continue;
        
        DatabaseEntry entry;
        
        // Check for alias=path format
        size_t eqPos = item.find('=');
        if (eqPos != std::string::npos) {
            entry.alias = item.substr(0, eqPos);
            entry.path = expandPath(item.substr(eqPos + 1));
        } else {
            // Direct path (directory)
            entry.alias = "";
            entry.path = expandPath(item);
        }
        
        entry.db = std::make_unique<MarkDatabase>();
        // Auto-create databases from MARK_PATH if they don't exist
        if (!entry.db->initialize(entry.path, true)) {
            std::cerr << "Warning: Failed to initialize database in " << entry.path << std::endl;
            continue;
        }
        databases.push_back(std::move(entry));
    }
}

bool MarkDatabaseManager::initialize() {
    const char* markPath = std::getenv("MARK_PATH");
    
    if (markPath) {
        parseMarkPath(markPath);
    } else {
        // Fallback: use MARK_DIR and MARK_REMOTE_DIR for backward compatibility
        const char* markDir = std::getenv("MARK_DIR");
        if (markDir) {
            DatabaseEntry entry;
            entry.alias = "local";
            entry.path = expandPath(markDir);
            entry.db = std::make_unique<MarkDatabase>();
            // Auto-create local database if it doesn't exist
            if (!entry.db->initialize(entry.path, true)) {
                std::cerr << "Warning: Failed to initialize local database in " << entry.path << std::endl;
            } else {
                databases.push_back(std::move(entry));
            }
        }
        
        const char* remoteDir = std::getenv("MARK_REMOTE_DIR");
        if (remoteDir) {
            DatabaseEntry entry;
            entry.alias = "cloud";
            entry.path = expandPath(remoteDir);
            entry.db = std::make_unique<MarkDatabase>();
            // Auto-create remote database if it doesn't exist
            if (!entry.db->initialize(entry.path, true)) {
                std::cerr << "Warning: Failed to initialize remote database in " << entry.path << std::endl;
            } else {
                databases.push_back(std::move(entry));
            }
        }
    }
    
    return !databases.empty();
}

MarkDatabase* MarkDatabaseManager::findDatabase(const std::string& dbSpec) {
    // Try to find by alias first
    for (auto& entry : databases) {
        if (entry.alias == dbSpec) {
            return entry.db.get();
        }
    }
    
    // Try to find by path (expanded)
    std::string expandedPath = expandPath(dbSpec);
    for (auto& entry : databases) {
        if (entry.path == expandedPath) {
            return entry.db.get();
        }
    }
    
    // Not found in existing databases - create a new one
    // dbSpec is a directory path, will create directory/.mark_db
    DatabaseEntry entry;
    entry.alias = "";
    entry.path = expandPath(dbSpec);
    entry.db = std::make_unique<MarkDatabase>();
    if (!entry.db->initialize(entry.path, true)) {
        std::cerr << "findDatabase: Failed to create database in " << entry.path << std::endl;
        return nullptr;
    }
    databases.push_back(std::move(entry));
    return databases.back().db.get();
}

MarkDatabase* MarkDatabaseManager::getDefaultDatabase() {
    if (databases.empty()) return nullptr;
    return databases[0].db.get();
}

std::string MarkDatabaseManager::findMark(const std::string& markName, bool warnDuplicates) {
    std::string firstMatch;
    std::vector<std::string> allMatches;
    
    for (const auto& entry : databases) {
        std::string path = entry.db->getMarkPath(markName);
        if (!path.empty()) {
            if (firstMatch.empty()) {
                firstMatch = path;
            }
            if (warnDuplicates) {
                std::string dbName = entry.alias.empty() ? entry.path : entry.alias;
                allMatches.push_back(dbName + ":" + path);
            }
        }
    }
    
    if (warnDuplicates && allMatches.size() > 1) {
        for (size_t i = 1; i < allMatches.size(); i++) {
            std::cerr << "setd: warning: duplicate mark \"" << markName 
                      << "\" found in " << allMatches[i] << std::endl;
        }
    }
    
    return firstMatch;
}
