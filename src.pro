#-------------------------------------------------
#
# Project created by QtCreator 2013-10-09T11:48:34
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = src
TEMPLATE = app
TRANSLATIONS = lang/en.ts lang/fr.ts

 PRECOMPILED_HEADER = stable.h

SOURCES += \
    syntaxwindow.cpp \
    mainwindow.cpp \
    main.cpp \
    converter.cpp \
    codeeditor.cpp \
    aboutwindow.cpp \
    linenumberarea.cpp \
    algouttsyntaxhighlighter.cpp

HEADERS  += \
    syntaxwindow.h \
    mainwindow.h \
    converter.h \
    codeeditor.h \
    aboutwindow.h \
    stable.h \
    linenumberarea.h \
    algouttsyntaxhighlighter.h

FORMS    += \
    syntaxwindow.ui \
    mainwindow.ui \
    aboutwindow.ui

OTHER_FILES +=

RESOURCES += \
    res.qrc
