#include "mark.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

// CloudStorage implementation
CloudStorage::CloudType CloudStorage::parseCloudType(const std::string& type) {
    std::string lower = type;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "onedrive") return ONEDRIVE;
    if (lower == "googledrive" || lower == "gdrive") return GOOGLE_DRIVE;
    if (lower == "dropbox") return DROPBOX;
    if (lower == "icloud") return ICLOUD;
    return NONE;
}

std::string CloudStorage::cloudTypeToString(CloudType type) {
    switch (type) {
        case ONEDRIVE: return "onedrive";
        case GOOGLE_DRIVE: return "googledrive";
        case DROPBOX: return "dropbox";
        case ICLOUD: return "icloud";
        default: return "none";
    }
}

std::string CloudStorage::getCloudPath(CloudType type, const std::string& basePath) {
    if (type == NONE || basePath.empty()) return "";
    
    std::string cloudPath = basePath;
    if (cloudPath.back() != '/') cloudPath += "/";
    cloudPath += "mark-setd/";
    return cloudPath;
}

bool CloudStorage::syncToCloud(const std::string& localPath, const std::string& cloudPath, CloudType type) {
    if (type == NONE || cloudPath.empty()) return false;
    
    // Create cloud directory if it doesn't exist
    std::string dir = cloudPath;
    size_t pos = dir.find_last_of('/');
    if (pos != std::string::npos) {
        dir = dir.substr(0, pos);
        struct stat st;
        if (stat(dir.c_str(), &st) != 0) {
            // Create directory recursively
            std::string cmd = "mkdir -p \"" + dir + "\"";
            system(cmd.c_str());
        }
    }
    
    // Copy file to cloud location
    std::string cmd = "cp \"" + localPath + "\" \"" + cloudPath + "\"";
    return system(cmd.c_str()) == 0;
}

bool CloudStorage::syncFromCloud(const std::string& cloudPath, const std::string& localPath, CloudType type) {
    if (type == NONE || cloudPath.empty()) return false;
    
    struct stat st;
    if (stat(cloudPath.c_str(), &st) != 0) return false; // Cloud file doesn't exist
    
    // Copy file from cloud location
    std::string cmd = "cp \"" + cloudPath + "\" \"" + localPath + "\"";
    return system(cmd.c_str()) == 0;
}

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
    bool isRemoteFile = (filename == remoteMarkFile);
    
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

// Main function
int main(int argc, char* argv[]) {
    MarkDatabase db;
    
    const char* markDir = std::getenv("MARK_DIR");
    if (!markDir) {
        std::cerr << "mark: Must set environment var $MARK_DIR" << std::endl;
        return 1;
    }
    
    if (!db.initialize(std::string(markDir))) {
        std::cerr << "mark: error initializing database" << std::endl;
        return 1;
    }
    
    // Get current directory
    char* pwd = std::getenv("PWD");
    if (!pwd) {
        char cwd[1024];
        if (!getcwd(cwd, sizeof(cwd))) {
            std::cerr << "mark: Unable to get current directory" << std::endl;
            return 1;
        }
        pwd = cwd;
    }
    
    // Handle /tmp_mnt prefix (legacy support)
    std::string currentDir = pwd;
    if (currentDir.substr(0, 8) == "/tmp_mnt") {
        currentDir = currentDir.substr(8);
    }
    
    // Parse arguments
    if (argc == 1) {
        // List marks
        db.listMarks();
        return 0;
    }
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "-help") {
            std::cout << "Mark Directory\nusage:\tmark <options>\n\n"
                      << "option\t\t\tdescription\n\n"
                      << "<cr>\n"
                      << "-l<ist>\t\t\tLists current marks and their directories\n"
                      << "[mark]\t\t\tAliases current directory to alphanumeric mark name\n"
                      << "-rm [mark]\n"
                      << "-remove [mark]\t\tRemoves specified mark\n"
                      << "-v<ersion>\t\tPrints current version of the program\n"
                      << "-h<elp>\t\t\tThis help message\n"
                      << "-reset\t\t\tClears all marks in the current environment\n"
                      << "-r<efresh>\t\tRefreshes all marks in the current environment\n"
                      << "-c [mark]\t\tMake mark cloud-based (stored in $MARK_REMOTE_DIR)\n"
                      << "\nexamples:\tmark xxx, mark -list, mark -reset, mark -rm xxx, mark -cl xxx" << std::endl;
            return 0;
        } else if (arg == "-v" || arg == "-ver" || arg == "-version") {
            std::cout << "mark-setd version 2.0" << std::endl;
            return 0;
        } else if (arg == "-l" || arg == "-list") {
            db.listMarks();
        } else if (arg == "-rm" || arg == "-remove") {
            if (i + 1 < argc) {
                db.removeMark(argv[++i]);
                db.updateFile();
            } else {
                std::cerr << "mark: -rm requires a mark name" << std::endl;
            }
        } else if (arg == "-reset") {
            db.resetMarks();
            db.updateFile();
        } else if (arg == "-r" || arg == "-refresh" || arg == "-ref") {
            db.refreshMarks();
        } else if (arg == "-c") {
            // Cloud mark option (stored in MARK_REMOTE_DIR)
            if (i + 1 < argc) {
                std::string markName = argv[++i];
                const char* remoteDir = std::getenv("MARK_REMOTE_DIR");
                if (!remoteDir) {
                    std::cerr << "mark: -c requires $MARK_REMOTE_DIR to be set" << std::endl;
                    return 1;
                }
                db.addMark(markName, currentDir, true);
                db.updateFile();
            } else {
                std::cerr << "mark: -c requires a mark name" << std::endl;
            }
        } else if (arg[0] != '-') {
            // Regular mark (local by default)
            db.addMark(arg, currentDir, false);
            db.updateFile();
        } else {
            std::cerr << "mark: unrecognized option: " << arg << std::endl;
        }
    }
    
    return 0;
}

