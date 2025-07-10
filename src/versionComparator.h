#pragma once
#include <string>
#include <vector>
#include <ctime>
#include <sstream>


// ======================
// 版本比较策略（策略模式）
// ======================
class VersionComparator {
public:
    virtual ~VersionComparator() = default;
    virtual bool isNewer(const std::string& remote, const std::string& local) = 0;
};

class SemanticVersionComparator : public VersionComparator {
public:
    bool isNewer(const std::string& remote, const std::string& local) override;

private:
    std::vector<int> splitVersion(const std::string& version);
};

class TimestampVersionComparator : public VersionComparator {
public:
    bool isNewer(const std::string& remote, const std::string& local) override;

private:
    long parseTimestamp(const std::string& timestampStr);
};
