/*
 * Copyright (c) 2025 Michael Shebanow and Sunil William Savkar
 * 
 * Original work:
 * Copyright (c) 1991 Sunil William Savkar. All rights reserved.
 */

#ifndef SETD_HPP
#define SETD_HPP

#include <string>
#include <vector>
#include <memory>

/**
 * DirectoryQueueEntry class - represents a directory in the queue
 */
class DirectoryQueueEntry {
public:
    std::string path;
    std::unique_ptr<DirectoryQueueEntry> next;

    DirectoryQueueEntry(const std::string& p) : path(p), next(nullptr) {}
};

/**
 * SetdDatabase class - manages the directory queue database
 */
class SetdDatabase {
private:
    std::unique_ptr<DirectoryQueueEntry> queueHead;
    int queueLength;
    int maxQueue;
    std::string setdFile;

    bool readFromFile();
    bool writeToFile();
    void pushToQueue(const std::string& path);
    void removeFromQueue(const std::string& path);
    DirectoryQueueEntry* getQueueEntry(int index) const;
    static const char* readMarkFromFile(const std::string& filename, const std::string& markName);

public:
    SetdDatabase();
    ~SetdDatabase();

    bool initialize(const std::string& setdDir);
    bool addPwd(const std::string& pwd);
    bool setMaxQueue(int max);
    bool listQueue() const;
    std::string returnDest(const std::string& path) const;
    
    // Utility methods
    static std::string escapePath(const std::string& path);
    static std::string unescapePath(const std::string& path);
    static bool convertToDecimal(const std::string& str, int& result);
    static void upperString(std::string& str);
};

#endif // SETD_HPP

