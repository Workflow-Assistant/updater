#pragma once

#include <string>
#include <vector>
#include <memory>

#include <QtWidgets/QWidget>
#include <QString>

#include "versionComparator.h"


struct FileInfo {
    std::string filename;
    std::string hash;
};


// ======================
// 更新器主体（外观模式）
// ======================
class Updater : public QWidget
{
    Q_OBJECT

public:
    explicit Updater(std::unique_ptr<VersionComparator> comparator, QWidget* parent = nullptr);
    ~Updater();

    bool performUpdate();

private:
    std::unique_ptr<VersionComparator> comparator;

    std::string mainProgram;
    std::vector<FileInfo> fileList;

    const std::string localVersionFile = "version";
    const QString baseUrl = "http://localhost:8000";

    bool selfUpdate();
    std::string currentUpdaterVersion = "0.0.1";
    std::string remoteUpdaterVersion = currentUpdaterVersion;
    std::string updaterHash;

    std::string getRemoteVersion();

    bool downloadNewVersion(const FileInfo& file);
    bool applyUpdate(const FileInfo& file);

    bool launchMainProgram();
};

