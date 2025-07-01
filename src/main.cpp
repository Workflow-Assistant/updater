#include "updater.h"
#include "versionComparator.h"

#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Updater window(std::make_unique<SemanticVersionComparator>());
    window.show();
    return app.exec();
}
