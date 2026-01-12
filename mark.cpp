/*
 * Copyright (c) 2025 Michael Shebanow and Sunil William Savkar
 * 
 * Original work:
 * Copyright (c) 1991 Sunil William Savkar. All rights reserved.
 */

#include "mark_db.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <unistd.h>
#include <string>

// Main function
int main(int argc, char* argv[]) {
    MarkDatabaseManager manager;
    if (!manager.initialize()) {
        std::cerr << "mark: Must set environment var $MARK_PATH or $MARK_DIR" << std::endl;
        return 1;
    }
    
    MarkDatabase* db = manager.getDefaultDatabase();
    if (!db) {
        std::cerr << "mark: No default database available" << std::endl;
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
        // List marks from all databases
        for (const auto& entry : manager.getDatabases()) {
            if (!entry.alias.empty()) {
                std::cout << "\n[" << entry.alias << "]" << std::endl;
            } else {
                std::cout << "\n[" << entry.path << "]" << std::endl;
            }
            entry.db->listMarks();
        }
        return 0;
    }
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "-help") {
            std::cout << "Mark Directory\nusage:\tmark <options>\n\n"
                      << "option\t\t\tdescription\n\n"
                      << "<cr>\n"
                      << "-l<ist>\t\t\tLists current marks and their directories\n"
                      << "[mark] or [db]:[mark]\tAliases current directory to mark name\n"
                      << "\t\t\t\tUse 'db:mark' to specify which database\n"
                      << "-rm [mark]\n"
                      << "-remove [mark]\t\tRemoves specified mark\n"
                      << "-v<ersion>\t\tPrints current version of the program\n"
                      << "-h<elp>\t\t\tThis help message\n"
                      << "-reset\t\t\tClears all marks in the current environment (no confirmation)\n"
                      << "-clear\t\t\tClears all marks with confirmation prompt\n"
                      << "-r<efresh>\t\tRefreshes all marks in the current environment\n"
                      << "-c [mark]\t\tMake mark cloud-based (backward compat, maps to cloud:mark)\n"
                      << "\nexamples:\tmark xxx, mark cloud:xxx, mark -list, mark -reset, mark -clear, mark -rm xxx" << std::endl;
            return 0;
        } else if (arg == "-v" || arg == "-ver" || arg == "-version") {
            std::cout << "mark-setd version 2.0" << std::endl;
            return 0;
        } else if (arg == "-l" || arg == "-list") {
            for (const auto& entry : manager.getDatabases()) {
                if (!entry.alias.empty()) {
                    std::cout << "\n[" << entry.alias << "]" << std::endl;
                } else {
                    std::cout << "\n[" << entry.path << "]" << std::endl;
                }
                entry.db->listMarks();
            }
        } else if (arg == "-rm" || arg == "-remove") {
            if (i + 1 < argc) {
                db->removeMark(argv[++i]);
            } else {
                std::cerr << "mark: -rm requires a mark name" << std::endl;
            }
        } else if (arg == "-reset") {
            db->resetMarks();
        } else if (arg == "-clear") {
            // Clear all marks with confirmation
            std::cout << "This will remove ALL marks from the database." << std::endl;
            std::cout << "Are you sure? (yes/no): ";
            std::string confirmation;
            std::getline(std::cin, confirmation);
            
            // Convert to lowercase for comparison
            std::string lowerConfirmation = confirmation;
            for (char& c : lowerConfirmation) {
                c = std::tolower(c);
            }
            
            if (lowerConfirmation == "yes" || lowerConfirmation == "y") {
                if (db->resetMarks()) {
                    std::cout << "All marks cleared." << std::endl;
                } else {
                    std::cerr << "mark: Failed to clear marks" << std::endl;
                    return 1;
                }
            } else {
                std::cout << "Operation cancelled." << std::endl;
            }
            return 0;
        } else if (arg == "-r" || arg == "-refresh" || arg == "-ref") {
            db->refreshMarks();
        } else if (arg == "-c") {
            // Cloud mark option (backward compatibility - maps to cloud:mark)
            if (i + 1 < argc) {
                std::string markName = argv[++i];
                MarkDatabase* cloudDb = manager.findDatabase("cloud");
                if (!cloudDb) {
                    std::cerr << "mark: -c requires cloud database (set MARK_PATH or MARK_REMOTE_DIR)" << std::endl;
                    return 1;
                }
                
                std::string existingPath = cloudDb->getMarkPath(markName);
                if (!existingPath.empty()) {
                    std::cout << "mark: Mark \"" << markName << "\" already exists at: " << existingPath << std::endl;
                    std::cout << "mark: Update to current directory? (y/n): ";
                    std::string response;
                    std::getline(std::cin, response);
                    if (response == "y" || response == "Y" || response == "yes" || response == "Yes") {
                        cloudDb->addMark(markName, currentDir);
                    } else {
                        std::cerr << "mark: Operation cancelled" << std::endl;
                        return 0;
                    }
                } else {
                    cloudDb->addMark(markName, currentDir);
                }
            } else {
                std::cerr << "mark: -c requires a mark name" << std::endl;
            }
        } else if (arg[0] != '-') {
            // Check for <db>:<alias> syntax
            size_t colonPos = arg.find(':');
            if (colonPos != std::string::npos && colonPos > 0 && colonPos < arg.length() - 1) {
                // New syntax: mark <db>:<alias>
                std::string dbSpec = arg.substr(0, colonPos);
                std::string alias = arg.substr(colonPos + 1);
                
                MarkDatabase* targetDb = manager.findDatabase(dbSpec);
                if (!targetDb) {
                    std::cerr << "mark: Failed to create or access database \"" << dbSpec << "\"" << std::endl;
                    return 1;
                }
                
                targetDb->addMark(alias, currentDir);
            } else {
                // Regular mark (default database)
                db->addMark(arg, currentDir);
            }
        } else {
            std::cerr << "mark: unrecognized option: " << arg << std::endl;
        }
    }
    
    return 0;
}

