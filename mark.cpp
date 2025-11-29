#include "mark_db.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <string>

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
                
                // Check if mark already exists locally
                std::string existingPath = db.getMarkPath(markName);
                if (!existingPath.empty()) {
                    // Check if it's a local mark (not remote) by checking the database
                    // We need to check if the mark exists and is local
                    bool isLocalMark = false;
                    const auto& marks = db.getMarks();
                    for (const auto& entry : marks) {
                        if (entry->mark == markName && !entry->isCloud && !entry->unsetFlag) {
                            isLocalMark = true;
                            break;
                        }
                    }
                    
                    if (isLocalMark) {
                        std::cout << "mark: Local mark \"" << markName << "\" already exists at: " << existingPath << std::endl;
                        std::cout << "mark: Delete local mark and create remote mark? (y/n): ";
                        std::string response;
                        std::getline(std::cin, response);
                        if (response == "y" || response == "Y" || response == "yes" || response == "Yes") {
                            // Remove local mark first
                            db.removeMark(markName);
                            db.updateFile();
                            // Now add as remote mark
                            db.addMark(markName, currentDir, true);
                            db.updateFile();
                        } else {
                            std::cerr << "mark: Operation cancelled" << std::endl;
                            return 0;
                        }
                    } else {
                        // It's already a remote mark, just update it
                        db.addMark(markName, currentDir, true);
                        db.updateFile();
                    }
                } else {
                    // Mark doesn't exist, create as remote
                    db.addMark(markName, currentDir, true);
                    db.updateFile();
                }
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

