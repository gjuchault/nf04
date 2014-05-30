#include <QApplication>
#include <QFile>
#include <QDir>
#include <QTranslator>
#include "mainwindow.h"

extern bool dark;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString path = QDir::tempPath();
    QString lng;

    /* Si le dossier n'existe pas, on le créé */
    if ( ! QDir(path + "/nf04/").exists()) {
        QDir().mkdir(path + "/nf04");
    }

    if (QFile::exists(path + "/nf04/language")) {
        QFile inf(path + "/nf04/language");
        inf.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream in(&inf);
        in.setCodec("UTF-8");
        lng = in.readAll();
    } else {
        lng = "fr";
    }

    QTranslator translator;
    translator.load(lng);
    a.installTranslator(&translator);

    MainWindow w(0, lng);
    w.show();

    if (argc == 2) {
        w.autoOpen(argv[1]);
    }

    return a.exec();
}
