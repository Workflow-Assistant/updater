#include "updater.h"
#include "httpClient.h"

#include <iostream>
#include <fstream>

#include <QApplication>
#include <QProcess>

using namespace std;

Updater::Updater(std::unique_ptr<VersionComparator> comparator, QWidget *parent)
    : comparator(std::move(comparator)), QWidget(parent)
{
    setWindowTitle("Workflow Updater");
    setFixedSize(400, 300);
    qDebug() << "Starting update process...";

    if (!performUpdate()) {
        qDebug() << "Update process failed";
    }
   qDebug() << "Update process completed successfully, exiting application.";
    exit(0); // ֱ���˳�Ӧ�ó���
}

Updater::~Updater()
{}

static int readLocalVersion(const string& path) {
    ifstream file(path);
    string version;
    if (!file.is_open()) return 0;
    return getline(file, version) ? stoi(version) : 0;
}

static void writeLocalVersion(const string& path, const string& version) {
    ofstream(path) << version;
}

bool Updater::performUpdate() {
    // ȡ�������汾��Ϣ
    if (!getRemoteVersion()) {
        return false;
    }
    assert(!mainProgram.empty() && "Main program path is empty");
    assert(!remoteInstallerVersion.empty() && "Remote installer version is empty");
    assert((remotehotfixVersion != -1) && "Remote hotfix version is empty");


    // ��һ�׶� ��汾��װ������
    // ����汾����
    if (comparator->isNewer(remoteInstallerVersion, installerVersion)) {
        qDebug() << "New installer version available: "
            << QString::fromStdString(remoteInstallerVersion)
            << " (local: " << QString::fromStdString(installerVersion) << ")";

        // ���ز�Ӧ�ð�װ������
        if (!applyInstaller()) {
            qDebug() << "Failed to apply installer update.";
            return false;
        }
        return true;
    }

    else {
        qDebug() << "No new installer version available, checking hotfix...";
    }

    // �ڶ��׶� �ȸ����Զ��ļ��滻
    // ��ȡ���� hotfix�汾��
    int localhotfixVersion = readLocalVersion(hotfixVersionFile);

    if (remotehotfixVersion <= localhotfixVersion) {
        qDebug() << "No new hotfix version available: "
            << remotehotfixVersion
            << " (local: " << localhotfixVersion << ")";
        return launchMainProgram();
    }

    // ��ȡ�ļ��б�
    if (!hotfixFileList.empty()) {
        // ���ز������ļ�
        for (const auto& file : hotfixFileList) {
            if (!downloadNewVersion(file) || !applyUpdate(file)) {
                return false;
            }
        }
        // ���±��� hotfix �汾��
        writeLocalVersion(hotfixVersionFile, to_string(remotehotfixVersion));
        qDebug() << "Hotfix update applied successfully, new version: "
			<< remotehotfixVersion;
    }

    return launchMainProgram();
}

bool Updater::getRemoteVersion() {
    // ��ȡԶ�̰汾��Ϣ
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

bool Updater::downloadNewVersion(const FileInfo& file) {
    string tempFile = file.filename + ".tmp";
    QString url = baseUrl + "/updater/" + QString::fromStdString(file.filename);
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

    // ��֤�ļ���ϣ
    QString temp_hash = QCryptographicHash::hash(
        response.get_payload_ByteArray(), QCryptographicHash::Md5
    ).toHex();
    if (temp_hash != QString::fromStdString(file.hash)) {
        qDebug() << "Hash mismatch for file: " << QString::fromStdString(file.filename)
            << "Expected: " << QString::fromStdString(file.hash)
            << "Got: " << temp_hash;
        return false;
    }

    // �����ļ�
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
    // �����ļ��Ƿ����
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
    if (!QProcess::startDetached(QString::fromStdString(mainProgram), QStringList())) {
        qDebug() << "Failed to launch main program: " << QString::fromStdString(mainProgram);
        return false;
    }
    return true;
}

bool Updater::applyInstaller() {
    // ���ذ�װ��
    QString installer_name = "iNE_Setup_" + QString::fromStdString(remoteInstallerVersion) + ".exe";
    QString url = baseUrl + "/updater/" + installer_name;

    HTTPRequest request(HTTP_GET, url);
    request.set_headers({
        {"Authorization", "Basic YmpmdTpiamZ1"}
        });
    request.SimpleDebug();

    HTTPResponse response = HTTPClient::getInstance().send(request);
    response.SimpleDebug();

    if (!response.is_Status_200()) {
        qDebug() << "Failed to download installer: " << response.status_code << " " << response.reason_phrase;
        return false;
    }

    // ��֤��װ����ϣ
    QString temp_hash = QCryptographicHash::hash(
        response.get_payload_ByteArray(), QCryptographicHash::Md5
    ).toHex();
    if (temp_hash != QString::fromStdString(installerHash)) {
        qDebug() << "Installer hash mismatch: expected " << QString::fromStdString(installerHash)
            << ", got " << temp_hash;
        return false;
    }

    // ���氲װ������ʱ�ļ�
    string path = installer_name.toStdString();
    ofstream outFile(path, ios::binary);
    if (!outFile) {
        qDebug() << "Failed to open installer file for writing: " << QString::fromStdString(path);
        return false;
    }
    outFile.write(response.get_payload_ByteArray().data(), response.get_payload_ByteArray().size());
    outFile.close();

    if (!QProcess::startDetached(installer_name, QStringList())) {
        qDebug() << "Failed to launch installer: " << QString::fromStdString(path);
        return false;
    }
    qDebug() << "Installer launched successfully: " << QString::fromStdString(path);
    return true;
}
 