#include "updaterUI.h"

#include <QtWidgets/QApplication>


#include <QMutex>
#include <QMutexLocker>
#include <QFile>
#include <QTextStream>
#include <QDebug>
void myMessageHandle(QtMsgType, const QMessageLogContext&, const QString& msg)
{
    static QMutex mut; //多线程打印时需要加锁
    QMutexLocker locker(&mut);
    QFile file("updater.log");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream stream(&file);
        stream << msg << Qt::endl;
        file.close();
    }
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(myMessageHandle);
    QApplication app(argc, argv);
    app.setFont(QFont("微软雅黑", 12));
    UpdaterUI window;
    window.show();
    return app.exec();
}
