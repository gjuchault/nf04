#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "stable.h"
#include "codeeditor.h"

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = 0, QString lng = "");
        ~MainWindow();
        void changeLang(QString locale = "fr");
        void loadFile(QString path);
        void setCompletionFile(QString file = "");



    public slots:
        void start();
        void slotSave();
        void slotSaveAs();
        void slotOpen();
        void slotOpenNewWindow();
        void slotSyntax();
        void slotAbout();
        void slotQuit();
        void slotNew();
        void slotOpenCPP();
        void slotClair();
        void slotFonce();
        void slotOpenLog();
        void autoOpen(char *path);
        void slotLngFR();
        void slotLngEN();
        void readLog();

    protected:
        void dragEnterEvent(QDragEnterEvent *e);
        void dragMoveEvent(QDragMoveEvent *e);
        void dragLeaveEvent(QDragLeaveEvent *e);
        void dropEvent(QDropEvent *e);

    private:
        Ui::MainWindow *ui;
        QString lastFilePath;
        QFontDatabase fdb;
        QCompleter *completer;
        CodeEditor *textEdit;

        QStringList modelFromFile(const QString &fileName);
        QProcess *proc;
        QString mingwpath;
        QString opath;
        AlgoUttSyntaxHighLighter *SH;

        bool dark;
        void updateColor (bool dark);

};

#endif // MAINWINDOW_H
