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
// ���������壨���ģʽ��
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

    // ��Ҫ��װ�����µĴ�汾��
    const std::string installerVersion = "0.0.3";
    std::string remoteInstallerVersion;
    std::string installerHash;

    // �ȸ��� hotfix �汾�ļ�
    const std::string hotfixVersionFile = "version";
    int remotehotfixVersion = -1;
    std::vector<FileInfo> hotfixFileList;

    bool getRemoteVersion();

    bool downloadNewVersion(const FileInfo& file);
    bool applyUpdate(const FileInfo& file);

    bool launchMainProgram();
    bool applyInstaller();
};

