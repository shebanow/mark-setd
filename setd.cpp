/*
 * Copyright (c) 2025 Michael Shebanow and Sunil William Savkar
 * 
 * Original work:
 * Copyright (c) 1991 Sunil William Savkar. All rights reserved.
 */

#include "setd.hpp"
#include "mark_db.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <unistd.h>
#include <sys/stat.h>
#include <cmath>
#include <limits>

// SetdDatabase implementation
SetdDatabase::SetdDatabase() : queueHead(nullptr), queueLength(0), maxQueue(10) {
}

SetdDatabase::~SetdDatabase() {
    // Clean up queue
    while (queueHead) {
        auto next = std::move(queueHead->next);
        queueHead = std::move(next);
    }
}

std::string SetdDatabase::escapePath(const std::string& path) {
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

std::string SetdDatabase::unescapePath(const std::string& path) {
    std::string unescaped;
    for (size_t i = 0; i < path.length(); ++i) {
        if (path[i] == '\\' && i + 1 < path.length()) {
            if (path[i + 1] == ' ') {
                unescaped += ' ';
                ++i;
            } else if (path[i + 1] == '\\') {
                unescaped += '\\';
                ++i;
            } else {
                unescaped += path[i];
            }
        } else {
            unescaped += path[i];
        }
    }
    return unescaped;
}

bool SetdDatabase::convertToDecimal(const std::string& str, int& result) {
    if (str.empty()) return false;
    
    int num = 0;
    int muln = 1;
    size_t start = 0;
    
    if (str[0] == '-') {
        if (str.length() == 1) return false;
        muln = -1;
        start = 1;
    } else if (str[0] == '+') {
        if (str.length() == 1) return false;
        start = 1;
    }
    
    for (size_t i = start; i < str.length(); i++) {
        if (!std::isdigit(str[i])) return false;
        num = num * 10 + (str[i] - '0');
    }
    
    result = num * muln;
    return true;
}

void SetdDatabase::upperString(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

bool SetdDatabase::readFromFile() {
    std::ifstream file(setdFile);
    if (!file.is_open()) {
        std::cerr << "readFromFile: Unable to open " << setdFile << std::endl;
        return false;
    }
    
    // Read max queue
    maxQueue = 0;
    std::string firstLine;
    if (std::getline(file, firstLine)) {
        std::istringstream iss(firstLine);
        iss >> maxQueue;
        if (maxQueue <= 0) {
            maxQueue = 10;
        }
    } else {
        maxQueue = 10;
    }
    
    // Read queue entries (use getline to handle paths with spaces)
    std::string path;
    while (std::getline(file, path)) {
        if (path.empty()) continue;
        path = unescapePath(path);
        pushToQueue(path);
    }
    
    file.close();
    return true;
}

bool SetdDatabase::writeToFile() {
    std::ofstream file(setdFile);
    if (!file.is_open()) {
        std::cerr << "writeToFile: Unable to update " << setdFile << std::endl;
        return false;
    }
    
    file << maxQueue << std::endl;
    
    // Write queue in reverse order (oldest first)
    std::vector<std::string> paths;
    auto* current = queueHead.get();
    while (current) {
        paths.push_back(current->path);
        current = current->next.get();
    }
    
    // Write in reverse
    for (auto it = paths.rbegin(); it != paths.rend(); ++it) {
        std::string escaped = escapePath(*it);
        file << escaped << std::endl;
    }
    
    file.close();
    return true;
}

void SetdDatabase::pushToQueue(const std::string& path) {
    auto newEntry = std::make_unique<DirectoryQueueEntry>(path);
    newEntry->next = std::move(queueHead);
    queueHead = std::move(newEntry);
    queueLength++;
}

void SetdDatabase::removeFromQueue(const std::string& path) {
    if (!queueHead) return;
    
    // Check if head matches
    if (queueHead->path == path) {
        queueHead = std::move(queueHead->next);
        queueLength--;
        return;
    }
    
    // Search for matching entry
    auto* prev = queueHead.get();
    auto* current = queueHead->next.get();
    
    while (current) {
        if (current->path == path) {
            prev->next = std::move(current->next);
            queueLength--;
            return;
        }
        prev = current;
        current = current->next.get();
    }
}

DirectoryQueueEntry* SetdDatabase::getQueueEntry(int index) const {
    if (index < 0 || index >= queueLength) return nullptr;
    
    auto* current = queueHead.get();
    for (int i = 0; i < index && current; i++) {
        current = current->next.get();
    }
    return current;
}

bool SetdDatabase::initialize(const std::string& setdDir) {
    const char* setdDirEnv = std::getenv("SETD_DIR");
    if (!setdDirEnv) {
        std::cerr << "initialize: Must set environment var $SETD_DIR" << std::endl;
        return false;
    }
    
    setdFile = std::string(setdDirEnv) + "/setd_db";
    
    // Ensure file exists
    std::ofstream testFile(setdFile, std::ios::app);
    testFile.close();
    
    if (!readFromFile()) {
        std::cerr << "initialize: Unable to read setd file " << setdFile << std::endl;
        return false;
    }
    
    return true;
}

bool SetdDatabase::addPwd(const std::string& pwd) {
    // Don't add if same as current head
    if (queueHead && queueHead->path == pwd) {
        return true;
    }
    
    // Remove if already exists in queue
    removeFromQueue(pwd);
    
    // Add to front
    pushToQueue(pwd);
    
    // Trim if too long
    while (queueLength > maxQueue) {
        // Remove last entry
        if (!queueHead) break;
        if (!queueHead->next) {
            queueHead = nullptr;
            queueLength = 0;
            break;
        }
        
        auto* prev = queueHead.get();
        auto* current = queueHead->next.get();
        while (current->next) {
            prev = current;
            current = current->next.get();
        }
        prev->next = nullptr;
        queueLength--;
    }
    
    return writeToFile();
}

// Static helper - now uses MarkDatabaseManager (deprecated, kept for compatibility)
const char* SetdDatabase::readMarkFromFile(const std::string& directory, const std::string& markName) {
    static std::string cachedMarkPath;
    
    // Use MarkDatabase to read from the directory (directory/.mark_db)
    MarkDatabase tempDb;
    if (tempDb.initialize(directory, false)) {
        std::string path = tempDb.getMarkPath(markName);
        if (!path.empty()) {
            cachedMarkPath = path;
            return cachedMarkPath.c_str();
        }
    }
    
    return nullptr;
}

bool SetdDatabase::setMaxQueue(int max) {
    if (max <= 0) return false;
    maxQueue = max;
    return writeToFile();
}

bool SetdDatabase::listQueue() const {
    std::cerr << "Current Queue (Max = " << maxQueue << ")" << std::endl;
    std::cerr << "-------------" << std::endl << std::endl;
    
    int i = 0;
    auto* current = queueHead.get();
    while (current) {
        std::cerr << i << ". " << current->path << std::endl;
        current = current->next.get();
        i++;
    }
    
    return true;
}

std::string SetdDatabase::returnDest(const std::string& path) const {
    // Handle escaped paths
    std::string unescapedPath = unescapePath(path);
    
    // Try direct chdir first
    if (chdir(unescapedPath.c_str()) == 0) {
        return unescapedPath;
    }
    
    // Check if it's a mark (from environment variable - set by shell scripts)
    std::string markEnv = "mark_" + unescapedPath;
    const char* mark = std::getenv(markEnv.c_str());
    
    // If not found in environment, try using MarkDatabaseManager
    if (!mark) {
        static MarkDatabaseManager* manager = nullptr;
        static bool managerInitialized = false;
        
        if (!managerInitialized) {
            manager = new MarkDatabaseManager();
            if (manager->initialize()) {
                managerInitialized = true;
            } else {
                delete manager;
                manager = nullptr;
                managerInitialized = true; // Don't try again
            }
        }
        
        if (manager) {
            // Check for -w flag (warn duplicates) - this would need to be passed in
            // For now, don't warn
            std::string markPath = manager->findMark(unescapedPath, false);
            if (!markPath.empty()) {
                static std::string cachedMark;
                cachedMark = markPath;
                mark = cachedMark.c_str();
            }
        }
    }
    
    // Check if it's an environment variable
    const char* env = std::getenv(unescapedPath.c_str());
    
    // Check uppercase version
    std::string upperPath = unescapedPath;
    upperString(upperPath);
    const char* upperEnv = std::getenv(upperPath.c_str());
    
    // Handle mark/subdir or env/subdir
    size_t slashPos = unescapedPath.find('/');
    if (slashPos != std::string::npos) {
        std::string prefix = unescapedPath.substr(0, slashPos);
        std::string suffix = unescapedPath.substr(slashPos);
        
        std::string markPrefix = "mark_" + prefix;
        const char* markBase = std::getenv(markPrefix.c_str());
        
        // If not in environment, try using MarkDatabaseManager
        if (!markBase) {
            static MarkDatabaseManager* manager = nullptr;
            static bool managerInitialized = false;
            
            if (!managerInitialized) {
                manager = new MarkDatabaseManager();
                if (manager->initialize()) {
                    managerInitialized = true;
                } else {
                    delete manager;
                    manager = nullptr;
                    managerInitialized = true;
                }
            }
            
            if (manager) {
                std::string markPath = manager->findMark(prefix, false);
                if (!markPath.empty()) {
                    static std::string cachedMarkBase;
                    cachedMarkBase = markPath;
                    markBase = cachedMarkBase.c_str();
                }
            }
        }
        
        if (markBase) {
            std::string result = std::string(markBase) + suffix;
            if (chdir(result.c_str()) == 0) {
                return result;
            }
        }
        
        // Try as environment variable
        const char* envBase = std::getenv(prefix.c_str());
        if (envBase) {
            std::string result = std::string(envBase) + suffix;
            if (chdir(result.c_str()) == 0) {
                return result;
            }
        }
        
        // Try uppercase
        std::string upperPrefix = prefix;
        upperString(upperPrefix);
        const char* upperBase = std::getenv(upperPrefix.c_str());
        if (upperBase) {
            std::string result = std::string(upperBase) + suffix;
            if (chdir(result.c_str()) == 0) {
                return result;
            }
        }
    }
    
    // Return mark if found
    if (mark) {
        return std::string(mark);
    }
    
    // Return env if found
    if (env) {
        return std::string(env);
    }
    
    // Return uppercase env if found
    if (upperEnv) {
        return std::string(upperEnv);
    }
    
    // Check if it's a numeric offset
    int num = 0;
    if (convertToDecimal(unescapedPath, num)) {
        int absNum = std::abs(num);
        if (absNum >= queueLength) {
            std::cerr << "returnDest: out of bounds (-" << queueLength 
                      << " <= num <= " << queueLength << ")" << std::endl;
            return unescapedPath;
        }
        
        auto* entry = getQueueEntry(absNum);
        if (entry) {
            return entry->path;
        }
    }
    
    // Handle %directory (same level)
    if (unescapedPath[0] == '%') {
        std::string newPath = "../" + unescapedPath.substr(1);
        return returnDest(newPath);
    }
    
    // Handle @partial_path (search queue)
    if (unescapedPath[0] == '@') {
        std::string searchStr = unescapedPath.substr(1);
        auto* current = queueHead.get();
        while (current) {
            size_t pos = current->path.find(searchStr);
            if (pos != std::string::npos && 
                pos + searchStr.length() == current->path.length()) {
                return current->path;
            }
            current = current->next.get();
        }
    }
    
    // Return original path (let cd handle error)
    return unescapedPath;
}

// Main function
int main(int argc, char* argv[]) {
    SetdDatabase db;
    
    const char* setdDir = std::getenv("SETD_DIR");
    if (!setdDir) {
        std::cerr << "setd: Must set environment var $SETD_DIR" << std::endl;
        return 1;
    }
    
    if (!db.initialize(std::string(setdDir))) {
        std::cerr << "setd: error initializing database" << std::endl;
        return 1;
    }
    
    // Get current directory
    char* pwd = std::getenv("PWD");
    if (!pwd) {
        char cwd[1024];
        if (!getcwd(cwd, sizeof(cwd))) {
            std::cerr << "setd: Unable to get current directory" << std::endl;
            return 1;
        }
        pwd = cwd;
    }
    
    // Handle /tmp_mnt prefix (legacy support)
    std::string currentDir = pwd;
    if (currentDir.substr(0, 8) == "/tmp_mnt") {
        currentDir = currentDir.substr(8);
    }
    
    // Add current directory to queue
    db.addPwd(currentDir);
    
    std::string dest = currentDir;
    bool warnDuplicates = false;
    
    // Parse arguments
    if (argc == 1) {
        // Go to home
        const char* home = std::getenv("HOME");
        if (home) {
            dest = home;
        }
    } else {
        // Collect all non-flag arguments into a single path (handles paths with spaces)
        std::string combinedPath;
        bool foundPath = false;
        
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            
            if (arg == "-h" || arg == "-help") {
                std::cout << "Set Directory\nusage:\tcd <options>\n\n"
                          << "option\t\tdescription\n\n"
                          << "[path]\t\tAttempts change to specified directory pathname\n"
                          << "[mark]\t\tAttempts change to directory specified by the mark alias\n"
                          << "[mark]/[path]\tAttempts change to base mark plus appended pathname\n"
                          << "@[string]\tAttempts change to directory based upon match of substring\n"
                          << "\t\twith element in current directory list\n"
                          << "[env]\t\tAttempts change to directory spec'd by environment variable\n"
                          << "%[path]\t\tAttempts change to subdirectory pathname of root one above\n"
                          << "-l<ist>\t\tLists previous directories up to maximum set list length\n"
                          << "-m<ax>\t\tSets the maximum depth of the past directory list\n"
                          << "-w\t\tWarn about duplicate marks in multiple databases\n"
                          << "numeric\t\tChanges directory to specified list pos, or offset from top (-)\n"
                          << "\nexamples:\tcd ~savkar, cd %bin, cd -4, cd MARK_NAME, cd MARK_NAME/xxx" << std::endl;
                return 0;
            } else if (arg == "-v" || arg == "-ver" || arg == "-version") {
                std::cout << "mark-setd version 2.0" << std::endl;
                return 0;
            } else if (arg == "-l" || arg == "-list") {
                db.listQueue();
                return 0;
            } else if (arg == "-m" || arg == "-max") {
                if (i + 1 < argc) {
                    int max = 0;
                    if (SetdDatabase::convertToDecimal(argv[++i], max) && max > 0) {
                        db.setMaxQueue(max);
                    } else {
                        std::cerr << "setd: invalid maximum specified" << std::endl;
                    }
                } else {
                    std::cerr << "setd: -max requires a number" << std::endl;
                }
                return 0;
            } else if (arg == "-w") {
                warnDuplicates = true;
            } else {
                // Collect non-flag arguments into combined path
                if (foundPath) {
                    combinedPath += " " + arg;
                } else {
                    combinedPath = arg;
                    foundPath = true;
                }
            }
        }
        
        // Process the combined path if we found one
        if (foundPath) {
            dest = db.returnDest(combinedPath);
        }
    }
    
    std::cout << dest;
    return 0;
}

