#include "updater.h"
#include "httpClient.h"

#include <iostream>
#include <fstream>
#include <Windows.h>

#include <QApplication>
#include <QFile>
#include <QDir>
#include <QProcess>

using namespace std;

Updater::Updater(std::unique_ptr<VersionComparator> comparator, QWidget *parent)
    : comparator(std::move(comparator)), QWidget(parent)
{
    setWindowTitle("Workflow Updater");
    setFixedSize(400, 300);

    if (!performUpdate()) {
        qDebug() << "Update process failed";
    }
    exit(0); // 直接退出应用程序
}

Updater::~Updater()
{}

static bool launchProgram(const string& path) {
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    return CreateProcess(path.c_str(), NULL, NULL, NULL,
        FALSE, 0, NULL, NULL, &si, &pi);
}

static string readLocalVersion(const string& path) {
    ifstream file(path);
    string version;
    return getline(file, version) ? version : "0.0.0";
}

static void writeLocalVersion(const string& path, const string& version) {
    ofstream(path) << version;
}

bool Updater::performUpdate() {
    string localVer = readLocalVersion(localVersionFile);
    string remoteVer = getRemoteVersion();

    if(comparator->isNewer(remoteUpdaterVersion, currentUpdaterVersion)) {
		if (!selfUpdate()) {
			qDebug() << "Self-update failed";
			return false;
		}
	}

    if (!comparator->isNewer(remoteVer, localVer)) {
        qDebug() << "Already up to date";
        return launchMainProgram();
    }

    // 获取文件列表
    if (fileList.empty()) return false;

    // 下载并更新文件
    for (const auto& file : fileList) {
        if (!downloadNewVersion(file) || !applyUpdate(file)) {
            return false;
        }
    }

    writeLocalVersion(localVersionFile, remoteVer);
    return launchMainProgram();
}

bool Updater::selfUpdate() {
    FileInfo selfUpdateFile;
    selfUpdateFile.filename = "updater.exe";
    selfUpdateFile.hash = updaterHash;
    if (!downloadNewVersion(selfUpdateFile)) {
		qDebug() << "Failed to download self-update file";
		return false;
	}
    QString appPath = QApplication::applicationDirPath();
    QString scriptPath = appPath + "/update_helper.bat";
    QFile scriptFile(scriptPath);
    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptFile);
        out.setCodec("IBM 850"); // 设置正确的编码以处理路径中的非ASCII字符

        out << "@echo off\n";
        // 1. 等待2秒，确保主程序完全退出
        out << "timeout /t 10 /nobreak > NUL\n";
        // 2. 覆盖旧文件 (move /Y 会强制覆盖)
        out << "move /Y \"updater.exe.tmp\" \"updater.exe\"\n";
        // 3. 重新启动更新后的程序
        out << "start \"\" \"updater.exe\"\n";
        // 4. 删除自己 (脚本)
        out << "del \"" << QDir::toNativeSeparators(scriptPath) << "\"\n";

        scriptFile.close();

        // 使用分离模式启动脚本，这样即使主程序退出，脚本也能继续运行
        QProcess::startDetached("cmd.exe", { "/C", QDir::toNativeSeparators(scriptPath) });

        // 关键：立即退出当前程序
        exit(0);
    }
    else {
		qDebug() << "Failed to create update script: " << scriptPath;
		return false;
	}
}

string Updater::getRemoteVersion() {
    // 获取远程版本信息
    HTTPRequest request(HTTP_GET, baseUrl + "/api/updater/version");
    request.SimpleDebug();
    HTTPResponse response = HTTPClient::getInstance().send(request);
    response.SimpleDebug();

    if (response.is_Status_200()) {
		QJsonObject res_json = response.get_payload_QJsonObject();

        this->mainProgram = res_json["main_program"].toString().toStdString();
        for (const auto& file : res_json["files"].toArray()) {
			FileInfo fileInfo;
			fileInfo.filename = file.toObject()["filename"].toString().toStdString();
			fileInfo.hash = file.toObject()["hash"].toString().toStdString();
			this->fileList.push_back(fileInfo);
		}
        
        remoteUpdaterVersion = res_json["updater_version"].toString().toStdString();
        updaterHash = res_json["updater_hash"].toString().toStdString();
		return res_json["version"].toString().toStdString();
	}
	else {
		qDebug() << "Failed to get remote version: " << response.status_code;
		return "0.0.0"; // 返回默认版本
	}
}

bool Updater::downloadNewVersion(const FileInfo& file) {
    string tempFile = file.filename + ".tmp";
    QString url = baseUrl + "/updater/" + QString::fromStdString(file.filename);
    HTTPRequest request(HTTP_GET, url);
    request.SimpleDebug();
    HTTPResponse response = HTTPClient::getInstance().send(request);
    response.SimpleDebug();

    if (!response.is_Status_200()) {
		qDebug() << "Failed to download file: " << QString::fromStdString(file.filename) 
			 << response.status_code << " " << response.reason_phrase;
		return false;
	}

    // 验证文件哈希
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
	ofstream outFile(tempFile, ios::binary);
	if (!outFile) {
		qDebug() << "Failed to open file for writing: " << QString::fromStdString(file.filename);
		return false;
	}
	outFile.write(response.get_payload_ByteArray().data(), response.get_payload_ByteArray().size());
	outFile.close();

    qDebug() << "Downloaded and verified: " << QString::fromStdString(file.filename);
	return true;
}

bool Updater::applyUpdate(const FileInfo& file) {
    // 检查旧文件是否存在
    ifstream oldFile(file.filename);
    string tempFile = file.filename + ".tmp";
    if (oldFile) {
        oldFile.close();
        qDebug() << "Old file exists, replacing...";
        if (remove(file.filename.c_str()) != 0) {
            qDebug() << "Failed to remove old file: " << QString::fromStdString(file.filename) << ": " << strerror(errno);
            return false;
        }
        if (rename(tempFile.c_str(), file.filename.c_str()) != 0) {
            qDebug() << "Failed to replace file: " << QString::fromStdString(file.filename) << ": " << strerror(errno);
            return false;
        }
    }
    else {
        oldFile.close();
        qDebug() << "No old file found, creating new one...";
        if (rename(tempFile.c_str(), file.filename.c_str()) != 0) {
            qDebug() << "Failed to create new file: " << QString::fromStdString(file.filename) << ": " << strerror(errno) << endl;
            return false;
        }
    }

    return true;
}

bool Updater::launchMainProgram() {
    if (!launchProgram(mainProgram)) {
        qDebug() << "Failed to launch program";
        return false;
    }
    return true;
}
 