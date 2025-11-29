#include "mark_db.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cctype>

// MarkDatabase implementation
MarkDatabase::MarkDatabase() : maxMarkSize(0) {
    remoteMarkFile = "";
}

MarkDatabase::~MarkDatabase() {
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

bool MarkDatabase::readFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        // File doesn't exist - that's OK, just return true
        return true;
    }
    
    // Determine if this is a remote file based on filename
    // If remoteMarkFile is empty, check if filename contains MARK_REMOTE_DIR
    bool isRemoteFile = false;
    if (!remoteMarkFile.empty()) {
        isRemoteFile = (filename == remoteMarkFile);
    } else {
        // Check if filename is in MARK_REMOTE_DIR
        const char* remoteDirEnv = std::getenv("MARK_REMOTE_DIR");
        if (remoteDirEnv) {
            std::string remoteDb = std::string(remoteDirEnv) + "/.mark_db";
            isRemoteFile = (filename == remoteDb);
        }
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string setenv, markPrefix, mark, path;
        
        iss >> setenv >> markPrefix >> path;
        
        if (setenv == "unsetenv") continue;
        if (setenv != "setenv") {
            std::cerr << "readFromFile: Error in mark database format" << std::endl;
            continue;
        }
        
        // Extract mark name from "mark_<name>"
        if (markPrefix.substr(0, 5) == "mark_") {
            mark = markPrefix.substr(5);
        } else {
            continue;
        }
        
        // Handle escaped paths
        path = unescapePath(path);
        
        // Check if mark already exists (local takes precedence)
        bool exists = false;
        for (auto& entry : marks) {
            if (entry->mark == mark) {
                // Local mark exists, skip this remote mark
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            auto entry = std::make_unique<MarkEntry>(mark, path, isRemoteFile);
            marks.push_back(std::move(entry));
            
            if (mark.length() > static_cast<size_t>(maxMarkSize)) {
                maxMarkSize = mark.length();
            }
        }
    }
    
    file.close();
    return true;
}

bool MarkDatabase::writeToFile(const std::string& filename) {
    std::string updateFile = filename + "_update";
    std::ofstream file(updateFile);
    if (!file.is_open()) {
        std::cerr << "writeToFile: Unable to update " << filename << std::endl;
        return false;
    }
    
    for (const auto& entry : marks) {
        if (!entry->unsetFlag) {
            std::string escapedPath = escapePath(entry->path);
            file << "setenv mark_" << entry->mark << " " << escapedPath << std::endl;
        } else {
            file << "unsetenv mark_" << entry->mark << std::endl;
        }
    }
    
    file.close();
    
    // Atomic rename
    if (rename(updateFile.c_str(), filename.c_str()) != 0) {
        std::cerr << "writeToFile: Unable to rename update file" << std::endl;
        return false;
    }
    
    return true;
}

void MarkDatabase::sortMarks() {
    std::sort(marks.begin(), marks.end(), 
        [](const std::unique_ptr<MarkEntry>& a, const std::unique_ptr<MarkEntry>& b) {
            return a->mark < b->mark;
        });
}

bool MarkDatabase::initialize(const std::string& markDir) {
    const char* markDirEnv = std::getenv("MARK_DIR");
    if (!markDirEnv) {
        std::cerr << "initialize: Must set environment var $MARK_DIR" << std::endl;
        return false;
    }
    
    markFile = std::string(markDirEnv) + "/.mark_db";
    
    // Check for remote mark directory (optional)
    const char* remoteDirEnv = std::getenv("MARK_REMOTE_DIR");
    if (remoteDirEnv) {
        remoteMarkFile = std::string(remoteDirEnv) + "/.mark_db";
        // Load remote marks if remote directory is configured
        readFromFile(remoteMarkFile);
    }
    
    // Load local marks (takes precedence)
    if (!readFromFile(markFile)) {
        std::cerr << "initialize: Unable to read mark file " << markFile << std::endl;
        return false;
    }
    
    return true;
}

bool MarkDatabase::addMark(const std::string& mark, const std::string& path, bool isCloud) {
    if (!isValidMarkName(mark)) {
        std::cerr << "addMark: mark must be alphanumeric" << std::endl;
        return false;
    }
    
    // Check if mark already exists
    for (auto& entry : marks) {
        if (entry->mark == mark) {
            entry->path = path;
            entry->isCloud = isCloud;
            entry->unsetFlag = false;
            std::cerr << "addMark: mark \"" << mark << "\" (" << path << ") updated" << std::endl;
            return true;
        }
    }
    
    // Create new mark
    auto entry = std::make_unique<MarkEntry>(mark, path, isCloud);
    marks.push_back(std::move(entry));
    
    if (mark.length() > static_cast<size_t>(maxMarkSize)) {
        maxMarkSize = mark.length();
    }
    
    std::cerr << "addMark: mark \"" << mark << "\" (" << path << ") set" << std::endl;
    return true;
}

bool MarkDatabase::removeMark(const std::string& mark) {
    for (auto& entry : marks) {
        if (entry->mark == mark) {
            entry->unsetFlag = true;
            std::cerr << "removeMark: mark \"" << mark << "\" (" << entry->path << ") removed" << std::endl;
            return true;
        }
    }
    
    std::cerr << "removeMark: mark \"" << mark << "\" not found" << std::endl;
    return false;
}

bool MarkDatabase::resetMarks() {
    for (auto& entry : marks) {
        entry->unsetFlag = true;
    }
    return true;
}

bool MarkDatabase::refreshMarks() {
    // This would typically reload the marks into the shell environment
    // For now, we'll just return true
    return true;
}

bool MarkDatabase::listMarks() {
    if (maxMarkSize < 4) maxMarkSize = 4;
    
    if (marks.empty()) {
        std::cout << std::endl;
        return true;
    }
    
    // Filter out unset marks for display
    std::vector<MarkEntry*> activeMarks;
    for (const auto& entry : marks) {
        if (!entry->unsetFlag) {
            activeMarks.push_back(entry.get());
        }
    }
    
    if (activeMarks.empty()) {
        std::cout << std::endl;
        return true;
    }
    
    std::cout << "MARK";
    for (int i = 0; i < maxMarkSize + 1; i++) std::cout << " ";
    std::cout << "PATH" << std::endl;
    std::cout << "----";
    for (int i = 0; i < maxMarkSize + 1; i++) std::cout << " ";
    std::cout << "----" << std::endl;
    
    for (const auto* entry : activeMarks) {
        std::cout << entry->mark << " ";
        int spaces = maxMarkSize - entry->mark.length() + 3;
        for (int i = 0; i < spaces; i++) std::cout << "_";
        std::cout << " " << entry->path;
        if (entry->isCloud) std::cout << " [REMOTE]";
        std::cout << std::endl;
    }
    
    return true;
}

bool MarkDatabase::updateFile() {
    sortMarks();
    
    // Separate local and remote marks
    std::vector<MarkEntry*> localMarks;
    std::vector<MarkEntry*> remoteMarks;
    
    for (const auto& entry : marks) {
        if (!entry->unsetFlag) {
            if (entry->isCloud) {
                remoteMarks.push_back(entry.get());
            } else {
                localMarks.push_back(entry.get());
            }
        }
    }
    
    // Write local marks
    std::string updateFile = markFile + "_update";
    std::ofstream file(updateFile);
    if (!file.is_open()) {
        std::cerr << "updateFile: Unable to update " << markFile << std::endl;
        return false;
    }
    
    for (const auto* entry : localMarks) {
        std::string escapedPath = escapePath(entry->path);
        file << "setenv mark_" << entry->mark << " " << escapedPath << std::endl;
    }
    
    file.close();
    
    // Atomic rename for local
    if (rename(updateFile.c_str(), markFile.c_str()) != 0) {
        std::cerr << "updateFile: Unable to rename update file" << std::endl;
        return false;
    }
    
    // Write remote marks if MARK_REMOTE_DIR is set
    if (!remoteMarkFile.empty() && !remoteMarks.empty()) {
        std::string remoteUpdateFile = remoteMarkFile + "_update";
        std::ofstream remoteFile(remoteUpdateFile);
        if (remoteFile.is_open()) {
            for (const auto* entry : remoteMarks) {
                std::string escapedPath = escapePath(entry->path);
                remoteFile << "setenv mark_" << entry->mark << " " << escapedPath << std::endl;
            }
            remoteFile.close();
            
            // Atomic rename for remote
            if (rename(remoteUpdateFile.c_str(), remoteMarkFile.c_str()) != 0) {
                std::cerr << "updateFile: Unable to rename remote update file" << std::endl;
                // Don't fail if remote write fails
            }
        }
    }
    
    return true;
}

std::string MarkDatabase::getMarkPath(const std::string& mark) const {
    for (const auto& entry : marks) {
        if (entry->mark == mark && !entry->unsetFlag) {
            return entry->path;
        }
    }
    return "";
}

