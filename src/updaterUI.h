#pragma once

#include <QWidget>

#include "versionComparator.h"


// 前向声明
class QLabel;
class QProgressBar;
class Updater;
class QThread;

class UpdaterUI : public QWidget
{
    Q_OBJECT

public:
    explicit UpdaterUI(QWidget* parent = nullptr);
    ~UpdaterUI();

protected:
    // 重写 showEvent，在窗口第一次显示时自动开始更新
    void showEvent(QShowEvent* event) override;
    // 重写 closeEvent，确保在关闭窗口时能正确处理线程
    void closeEvent(QCloseEvent* event) override;

private slots:
    // 槽：响应worker的进度更新信号
    void onUpdateProgress(int value, const QString& text);

    // 槽：响应worker的启动主程序信号
    void onLaunchProgramRequested(const QString& programPath);

    // 槽：响应worker的启动安装程序信号
    void onLaunchInstallerRequested(const QString& installerPath);

    // 槽：响应worker的完成信号
    void onUpdateFinished(bool workerSuccess, const QString& message);

private:
    void startUpdate();

    // UI 控件
    QLabel* statusLabel;
    QProgressBar* progressBar;

    // 线程和工作者
    QThread* workerThread;
    Updater* worker;

    // worke运行状态标志
    bool workerStarted = false;
    bool workerFinished = false;

    int launchSuccess = 1; // 标记更新后是否启动对应程序
};
