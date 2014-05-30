#ifndef STABLE_H
#define STABLE_H

#if defined __cplusplus

#include <QtGlobal>

#ifdef Q_OS_WIN
#include "windows.h"
#else
#include <sys/types.h>
#include <sys/wait.h>
#endif
#include <iostream>
#include <QAbstractItemView>
#include <QApplication>
#include <QColor>
#include <QCompleter>
#include <QDesktopServices>
#include <QDir>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QHash>
#include <QInputDialog>
#include <QLabel>
#include <QList>
#include <QMainWindow>
#include <QMessageBox>
#include <QMimeData>
#include <QObject>
#include <QPaintEvent>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegExp>
#include <QResizeEvent>
#include <QScrollBar>
#include <QShortcut>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QSyntaxHighlighter>
#include <QTextStream>
#include <QtGui>
#include <QTimer>
#include <QWidget>

#include <QDebug>

#endif

#endif // STABLE_H
