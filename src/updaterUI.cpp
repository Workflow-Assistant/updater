#include "updaterUI.h"
#include "updater.h"

#include <QIcon>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QThread>
#include <QProcess>
#include <QMessageBox>
#include <QCloseEvent>
#include <QTimer>

#include <QDebug>


UpdaterUI::UpdaterUI(QWidget* parent)
    : QWidget(parent)
{
    qDebug() << "Updater window created.";
    // 1. 初始化UI界面
    setWindowTitle("正在检查更新...");
    setFixedSize(400, 120);

    QIcon icon(":/favicon.ico");
    setWindowIcon(icon);
    
    statusLabel = new QLabel("正在准备...", this);
    statusLabel->setAlignment(Qt::AlignCenter);

    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setTextVisible(true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(statusLabel);
    layout->addWidget(progressBar);
    setLayout(layout);

    // 2. 创建线程和工作者对象
    workerThread = new QThread(this);

    worker = new Updater(std::make_unique<SemanticVersionComparator>());

    // 3. 将工作者“移动”到新线程
    worker->moveToThread(workerThread);

    // 4. 建立信号槽连接
    connect(workerThread, &QThread::started, worker, &Updater::process);

    // 将worker的信号连接到UpdaterUI的槽
    connect(worker, &Updater::progressChanged, this, &UpdaterUI::onUpdateProgress);
    connect(worker, &Updater::launchProgramRequested, this, &UpdaterUI::onLaunchProgramRequested);
    connect(worker, &Updater::launchInstallerRequested, this, &UpdaterUI::onLaunchInstallerRequested);
    connect(worker, &Updater::finished, this, &UpdaterUI::onUpdateFinished);

    // 线程结束后，自动退出事件循环并清理资源
    connect(worker, &Updater::finished, workerThread, &QThread::quit);
    connect(workerThread, &QThread::finished, worker, &Updater::deleteLater);
    // workerThread的父对象是this(UpdaterUI)，所以它会随窗口关闭而销毁，
    // 手动连接deleteLater以确保万无一失。
    connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);
}

UpdaterUI::~UpdaterUI()
{
    qDebug() << "Updater window destroyed\n";
}

void UpdaterUI::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (!workerStarted) {
        startUpdate();
        workerStarted = true;
    }
}

void UpdaterUI::closeEvent(QCloseEvent* event)
{
    // 如果更新还没结束，询问用户是否要退出
    if (!workerFinished) {
        QMessageBox::StandardButton res = QMessageBox::question(this, "确认退出",
            "更新正在进行中，确定要中断并退出吗？",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);

        if (res == QMessageBox::Yes) {
            // 确保线程被正确停止
            // isUpdateFinished需要再次检查取值，因为更新可能已经完成了
            if (!workerFinished && workerThread && workerThread->isRunning()) {
                workerThread->requestInterruption(); // (需要worker内部配合检查)
                workerThread->quit();
                workerThread->wait(1000); // 等待最多1秒
                event->accept();
            }
            else {
                QMessageBox::information(this, "提示", "更新已经完成，请查看窗口信息。");
                event->ignore();
            }
        }
        else {
            event->ignore();
        }
    }
    else {
        event->accept();
    }
}


void UpdaterUI::startUpdate()
{
    if (workerThread && !workerThread->isRunning()) {
        workerThread->start();
    }
}

void UpdaterUI::onUpdateProgress(int value, const QString& text)
{
    statusLabel->setText(text);
    if (value >= 0) { // 如果 value 是 -1，则不更新进度条，只更新文本
        progressBar->setValue(value);
    }
}

void UpdaterUI::onLaunchProgramRequested(const QString& programPath)
{
    if (!QProcess::startDetached(programPath, QStringList())) {
        qDebug() << "Failed to launch main program: " << programPath;
        launchSuccess = -1; // 标记启动主程序失败
        // Todo: 显示完整程序路径和启动失败原因
        QMessageBox::critical(this, "启动失败", "无法启动主程序: " + programPath);
    }
}

void UpdaterUI::onLaunchInstallerRequested(const QString& installerPath)
{
    if (!QProcess::startDetached(installerPath, QStringList())) {
        qDebug() << "Failed to launch installer: " << installerPath;
        launchSuccess = -2; // 标记启动安装程序失败
        // Todo: 显示完整程序路径和启动失败原因
        QMessageBox::critical(this, "启动失败", "无法启动安装程序: " + installerPath);
    }
}

void UpdaterUI::onUpdateFinished(bool workerSuccess, const QString& message)
{
    workerFinished = true; // 标记更新流程结束
    statusLabel->setText(message);
    if (workerSuccess && this->launchSuccess == 1) {
        qDebug() << "Update completed successfully: " << message;
        progressBar->setValue(100);
        // 更新成功后，1秒后自动关闭窗口
        QTimer::singleShot(1000, this, &UpdaterUI::close); // 延时1秒关闭窗口
    }
    else if (workerSuccess) {
		qDebug() << "Update completed, but program launch failed.";
        switch (this->launchSuccess)
        {
            case -1:
                statusLabel->setText("更新成功，但启动主程序失败，请联系开发者。");
                break;
            case -2:
                statusLabel->setText("更新成功，但启动安装程序失败，请手动运行安装包或联系开发者。");
				break;
        default:
            break;
        }
		// 更新成功但启动失败，显示黄色或橙色
		progressBar->setStyleSheet("QProgressBar::chunk { background-color: orange; }");
	}
    else {
        qDebug() << "Update failed: " << message;
        // 出错时，让进度条显示红色或停止
        progressBar->setStyleSheet("QProgressBar::chunk { background-color: red; }");
        QMessageBox::critical(this, "更新失败：", message);
    }
}
