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
MarkDatabase::MarkDatabase() : cloudType(CloudStorage::NONE), maxMarkSize(0) {
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

bool MarkDatabase::loadCloudConfig() {
    std::ifstream file(cloudConfigFile);
    if (!file.is_open()) {
        cloudType = CloudStorage::NONE;
        cloudBasePath = "";
        return true; // No config file is OK
    }
    
    std::string typeStr, path;
    if (std::getline(file, typeStr) && std::getline(file, path)) {
        cloudType = CloudStorage::parseCloudType(typeStr);
        cloudBasePath = path;
    } else {
        cloudType = CloudStorage::NONE;
        cloudBasePath = "";
    }
    file.close();
    return true;
}

bool MarkDatabase::saveCloudConfig() {
    std::ofstream file(cloudConfigFile);
    if (!file.is_open()) return false;
    
    file << CloudStorage::cloudTypeToString(cloudType) << std::endl;
    file << cloudBasePath << std::endl;
    file.close();
    return true;
}

bool MarkDatabase::readFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        // Create empty file if it doesn't exist
        std::ofstream createFile(filename);
        if (!createFile.is_open()) {
            std::cerr << "readFromFile: Unable to create " << filename << std::endl;
            return false;
        }
        createFile.close();
        return true;
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
        
        // Check if this is a cloud mark (we'll store this info separately)
        bool isCloud = false; // Will be determined by checking cloud database
        
        auto entry = std::make_unique<MarkEntry>(mark, path, isCloud);
        marks.push_back(std::move(entry));
        
        if (mark.length() > static_cast<size_t>(maxMarkSize)) {
            maxMarkSize = mark.length();
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
    cloudConfigFile = std::string(markDirEnv) + "/.mark_cloud_config";
    
    if (!loadCloudConfig()) {
        std::cerr << "initialize: Unable to load cloud config" << std::endl;
        return false;
    }
    
    // Sync from cloud if configured
    if (cloudType != CloudStorage::NONE && !cloudBasePath.empty()) {
        std::string cloudPath = CloudStorage::getCloudPath(cloudType, cloudBasePath) + ".mark_db";
        CloudStorage::syncFromCloud(cloudPath, markFile, cloudType);
    }
    
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
        if (entry->isCloud) std::cout << " [CLOUD]";
        std::cout << std::endl;
    }
    
    return true;
}

bool MarkDatabase::updateFile() {
    sortMarks();
    
    if (!writeToFile(markFile)) {
        return false;
    }
    
    // Sync to cloud if this is a cloud mark or if cloud is configured
    if (cloudType != CloudStorage::NONE && !cloudBasePath.empty()) {
        std::string cloudPath = CloudStorage::getCloudPath(cloudType, cloudBasePath) + ".mark_db";
        CloudStorage::syncToCloud(markFile, cloudPath, cloudType);
    }
    
    return true;
}

bool MarkDatabase::setupCloud(CloudStorage::CloudType type, const std::string& basePath) {
    cloudType = type;
    cloudBasePath = basePath;
    return saveCloudConfig();
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
                      << "-cloud-setup [type] [path]\tSetup cloud storage (onedrive, googledrive, dropbox, icloud)\n"
                      << "-cl [mark]\t\tMake mark cloud-based (universal)\n"
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
        } else if (arg == "-cloud-setup") {
            if (i + 2 < argc) {
                CloudStorage::CloudType type = CloudStorage::parseCloudType(argv[++i]);
                std::string path = argv[++i];
                if (db.setupCloud(type, path)) {
                    std::cout << "Cloud storage configured: " 
                              << CloudStorage::cloudTypeToString(type) 
                              << " at " << path << std::endl;
                } else {
                    std::cerr << "mark: error setting up cloud storage" << std::endl;
                }
            } else {
                std::cerr << "mark: -cloud-setup requires type and path" << std::endl;
            }
        } else if (arg == "-cl") {
            // Cloud mark option
            if (i + 1 < argc) {
                std::string markName = argv[++i];
                db.addMark(markName, currentDir, true);
                db.updateFile();
            } else {
                std::cerr << "mark: -cl requires a mark name" << std::endl;
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

