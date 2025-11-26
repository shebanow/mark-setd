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

