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

    const QString baseUrl = "http://localhost:8000";

    // 需要安装包更新的大版本号
    const std::string installerVersion = "0.0.3";
    std::string remoteInstallerVersion;
    std::string installerHash;

    // 热更新 hotfix 版本文件
    const std::string hotfixVersionFile = "version";
    int remotehotfixVersion = -1;
    std::vector<FileInfo> hotfixFileList;

    bool getRemoteVersion();

    bool downloadNewVersion(const FileInfo& file);
    bool applyUpdate(const FileInfo& file);

    bool launchMainProgram();
    bool applyInstaller();
};

