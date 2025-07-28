#include "updater.h"
#include "httpClient.h"

#include <fstream>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

#include <QDebug>

using namespace std;

Updater::Updater(std::unique_ptr<VersionComparator> comparator, QObject* parent)
    : comparator(std::move(comparator)), QObject(parent)
{
    qDebug() << "Updater initialized with comparator: "
			 << typeid(*this->comparator).name();
}

Updater::~Updater()
{
    qDebug() << "Updater destroyed.";
}

// --- Helper Functions ---
static int readLocalVersion(const string& path) {
    ifstream file(path);
    if (!file.is_open()) return 0;
    string version;
    return getline(file, version) ? stoi(version) : 0;
}

static void writeLocalVersion(const string& path, const string& version) {
    ofstream(path) << version;
}
// --- End Helper Functions ---

void Updater::process() {
    // 1. 获取远程版本信息
    emit progressChanged(10, "正在连接服务器，获取版本信息...");
    if (!getRemoteVersion()) {
        emit finished(false, "获取远程版本信息失败，请检查网络。");
        return;
    }

    assert(!mainProgram.empty() && "Main program path is empty");
    assert(!remoteInstallerVersion.empty() && "Remote installer version is empty");
    assert((remotehotfixVersion != -1) && "Remote hotfix version is empty");

    // 2. 检查大版本安装包更新
    emit progressChanged(20, "正在比较软件版本...");
    // 当本地版本号大于远程版本号，表示正在使用特殊版本，不再检查任何更新
    if (comparator->isNewer(installerVersion, remoteInstallerVersion)) {
        qDebug() << "Current version is newer than remote version, skipping installer update.";
        emit launchProgramRequested(QString::fromStdString(mainProgram));
        emit finished(true, "当前版本已是最新，无需更新。");
        return;
    }

    if (comparator->isNewer(remoteInstallerVersion, installerVersion)) {
        qDebug() << "New installer version available: "
            << QString::fromStdString(remoteInstallerVersion)
            << " (local: " << QString::fromStdString(installerVersion) << ")";

        // 拼接安装包名称
        QString installerName = "iNE_Setup_" +
            QString::fromStdString(remoteInstallerVersion) + ".exe";

        // 下载并应用安装包更新
        emit progressChanged(30, "发现新版本，准备下载安装包...");
        if (!downloadAndPrepareInstaller(installerName)) {
            emit finished(false, "下载或验证安装包失败。");
			return;
		}

        // 安装包准备就绪，发出信号通知UI层处理
        emit launchInstallerRequested(installerName);
        emit finished(true, "新版本安装包已就绪，请按提示进行安装。");
        return;
    }
    else {
		qDebug() << "No new installer version available, checking hotfix...";
        emit progressChanged(30, "当前为最新版本，正在检查热更新...");
	}

    // 3. 检查热更新 自动文件替换
    // 读取本地 hotfix版本号
    int localhotfixVersion = readLocalVersion(hotfixVersionFile);

    if (remotehotfixVersion <= localhotfixVersion) {
		qDebug() << "No new hotfix version available.";
        emit progressChanged(100, "已是最新版本，无需更新。");
        emit launchProgramRequested(QString::fromStdString(mainProgram));
        emit finished(true, "无需更新，即将启动主程序。");
		return;
	}

    // 4. 第三步检查到有热更新，执行热更新
    qDebug() << "New hotfix version available: "
        << remotehotfixVersion
        << " (local: " << localhotfixVersion << ")";
    emit progressChanged(40, QString("发现热更新 (v%1 -> v%2)，准备下载文件...")
        .arg(localhotfixVersion)
        .arg(remotehotfixVersion));
    if (!downloadAndApplyHotfix()) {
        emit finished(false, "热更新过程中发生错误。");
        return;
    }

    // 5. 更新本地hotfix版本号并完成
    writeLocalVersion(hotfixVersionFile, to_string(remotehotfixVersion));
    emit progressChanged(100, "热更新应用成功！");
    emit launchProgramRequested(QString::fromStdString(mainProgram));
    emit finished(true, "更新完成，即将启动主程序。");
}

bool Updater::getRemoteVersion() {
    // 获取远程版本信息
    HTTPRequest request(HTTP_GET, baseUrl + "/api/updater/version");
    request.SimpleDebug();
    HTTPResponse response = HTTPClient::getInstance().send(request);
    response.SimpleDebug();

    if (response.is_Status_200()) {
		QJsonObject res_json = response.get_payload_QJsonObject();

        this->mainProgram = res_json["main_program"].toString().toStdString();

        this->remoteInstallerVersion = res_json["version"].toString().toStdString();
        this->installerHash = res_json["hash"].toString().toStdString();

        this->remotehotfixVersion = res_json["hotfix"].toString().toInt();

        hotfixFileList.clear();
        for (const auto& file : res_json["files"].toArray()) {
			FileInfo fileInfo;
			fileInfo.filename = file.toObject()["filename"].toString().toStdString();
			fileInfo.hash = file.toObject()["hash"].toString().toStdString();
			this->hotfixFileList.push_back(fileInfo);
		}
		return true;
	}
	else {
		qDebug() << "Failed to get remote version: " << response.status_code;
		return false;
	}
}

bool Updater::downloadAndPrepareInstaller(const QString& installerName) {
    QString url = baseUrl + "/updater/" + installerName;

    emit progressChanged(50, "正在下载安装包: " + installerName);

    FileInfo installerInfo;
    installerInfo.filename = installerName.toStdString();
    installerInfo.hash = installerHash;

    return downloadFile(installerInfo, url, installerName);
}

bool Updater::downloadAndApplyHotfix() {
    if (hotfixFileList.empty()) {
        qDebug() << "Hotfix file list is empty, nothing to do.";
        return true;
    }

    int totalFiles = hotfixFileList.size();
    for (int i = 0; i < totalFiles; ++i) {
        const auto& file = hotfixFileList[i];
        int progress = 40 + static_cast<int>((static_cast<double>(i) / totalFiles) * 60.0);

        // 下载
        emit progressChanged(progress, QString("正在下载文件: %1 (%2/%3)...")
            .arg(QString::fromStdString(file.filename))
            .arg(i + 1)
            .arg(totalFiles));
        QString url = baseUrl + "/updater/" + QString::fromStdString(file.filename);
        QString tempPath = QString::fromStdString(file.filename) + ".tmp";
        if (!downloadFile(file, url, tempPath)) {
            return false;
        }

        // 应用
        emit progressChanged(progress, QString("正在应用更新: %1...")
            .arg(QString::fromStdString(file.filename)));
        if (!applyUpdate(file)) {
            return false;
        }
    }
    return true;
}

bool Updater::downloadFile(const FileInfo& file, const QString& url, const QString& savePath) {
    HTTPRequest request(HTTP_GET, url);
    request.set_headers({
        {"Authorization", "Basic YmpmdTpiamZ1"}
        });
    request.SimpleDebug();
    HTTPResponse response = HTTPClient::getInstance().send(request);
    response.SimpleDebug();

    if (!response.is_Status_200()) {
        qDebug() << "Failed to download file: " << QString::fromStdString(file.filename)
            << response.status_code << " " << response.reason_phrase;
        return false;
    }

    // 验证文件哈希
    emit progressChanged(-1, QString("正在校验文件: %1...") // -1 表示进度条不动
        .arg(QString::fromStdString(file.filename)));
    QString temp_hash = QCryptographicHash::hash(
        response.get_payload_ByteArray(), QCryptographicHash::Md5
    ).toHex();

    if (temp_hash != QString::fromStdString(file.hash)) {
        qDebug() << "Hash mismatch for file: " << QString::fromStdString(file.filename)
            << "Expected: " << QString::fromStdString(file.hash)
            << "Got: " << temp_hash;
        return false;
    }

    // 保存文件
    ofstream outFile(savePath.toStdString(), ios::binary);
    if (!outFile) {
        qDebug() << "Failed to open file for writing: " << savePath;
        return false;
    }
    outFile.write(response.get_payload_ByteArray().data(), response.get_payload_ByteArray().size());
    outFile.close();

    qDebug() << "Downloaded and verified: " << QString::fromStdString(file.filename);
    return true;
}

bool Updater::applyUpdate(const FileInfo& file) {
    string tempFile = file.filename + ".tmp";
    string originalFile = file.filename;

    if (QFile::exists(QString::fromStdString(originalFile))) {
        // 存在旧文件 先删除
        if (!QFile::remove(QString::fromStdString(originalFile))) {
            qDebug() << "Failed to remove old file: " << QString::fromStdString(originalFile);
            return false;
        }
    }

    if (!QFile::rename(QString::fromStdString(tempFile), QString::fromStdString(originalFile))) {
        qDebug() << "Failed to rename/move file: " << QString::fromStdString(tempFile) << " to " << QString::fromStdString(originalFile);
        return false;
    }

    return true;
}
 