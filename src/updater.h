#pragma once

#include <string>
#include <vector>
#include <memory>

#include <QObject>

#include "versionComparator.h"


struct FileInfo {
    std::string filename;
    std::string hash;
};

// ======================
// 更新器主体（外观模式）
// ======================
class Updater : public QObject
{
    Q_OBJECT

public:
    explicit Updater(std::unique_ptr<VersionComparator> comparator, QObject* parent = nullptr);
    ~Updater();

public slots:
    // 这是将在新线程中执行的核心函数
    void process();

signals:
    // 信号：更新进度和状态文本
    void progressChanged(int value, const QString& text);
signals:
    // 信号：通知UI层启动主程序
    void launchProgramRequested(const QString& programPath);
signals:
    // 信号：通知UI层启动安装程序
    void launchInstallerRequested(const QString& installerPath);
signals:
    // 信号：整个更新流程结束
    void finished(bool success, const QString& message);

private:
    std::unique_ptr<VersionComparator> comparator;

    std::string mainProgram;

    const QString baseUrl = "http://localhost:8000";
    //const QString baseUrl = "http://localhost:8000";

    // 需要安装包更新的大版本号
    const std::string installerVersion = "2.0.0";
    std::string remoteInstallerVersion;
    std::string installerHash;

    // 热更新 hotfix 版本文件
    const std::string hotfixVersionFile = "version";
    int remotehotfixVersion = -1;
    std::vector<FileInfo> hotfixFileList;

    bool getRemoteVersion();

    bool downloadAndPrepareInstaller(const QString& installerName);
    bool downloadAndApplyHotfix();

    bool downloadFile(const FileInfo& file, const QString& url, const QString& tempPath);
    bool applyUpdate(const FileInfo& file);
};
