#ifndef MARK_HPP
#define MARK_HPP

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <fstream>

// Forward declarations
class MarkEntry;

/**
 * MarkEntry class - represents a single mark entry
 */
class MarkEntry {
public:
    std::string mark;
    std::string path;
    bool isCloud;
    bool unsetFlag;

    MarkEntry(const std::string& m, const std::string& p, bool cloud = false)
        : mark(m), path(p), isCloud(cloud), unsetFlag(false) {}
};

/**
 * MarkDatabase class - manages the mark database
 */
class MarkDatabase {
private:
    std::vector<std::unique_ptr<MarkEntry>> marks;
    std::string markFile;
    std::string remoteMarkFile;
    int maxMarkSize;

    bool readFromFile(const std::string& filename);
    bool writeToFile(const std::string& filename);
    void sortMarks();

public:
    MarkDatabase();
    ~MarkDatabase();

    bool initialize(const std::string& markDir);
    bool addMark(const std::string& mark, const std::string& path, bool isCloud = false);
    bool removeMark(const std::string& mark);
    bool resetMarks();
    bool refreshMarks();
    bool listMarks();
    bool updateFile();
    
    std::string getMarkPath(const std::string& mark) const;
    const std::vector<std::unique_ptr<MarkEntry>>& getMarks() const { return marks; }
    
    // Utility methods
    static bool isValidMarkName(const std::string& mark);
    static std::string escapePath(const std::string& path);
    static std::string unescapePath(const std::string& path);
};

#endif // MARK_HPP

