#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include "stable.h"
#include "linenumberarea.h"
#include "algouttsyntaxhighlighter.h"

class CodeEditor : public QPlainTextEdit
{
        Q_OBJECT

    public:
        CodeEditor(QWidget *parent = 0, bool dark = true);
        ~CodeEditor();
        void lineNumberAreaPaintEvent(QPaintEvent *event);
        // Récupère la taille de la gutter
        int lineNumberAreaWidth();
        void setCompleter(QCompleter *completer);
        QCompleter *completer() const;
        void goToLine(int l);
        bool dark;
        void setDark(bool dark);

    protected:
        void resizeEvent(QResizeEvent *e);
        void focusInEvent(QFocusEvent *e);
        void keyPressEvent(QKeyEvent *e);
        void dragEnterEvent(QDragEnterEvent *e);
        void dragMoveEvent(QDragMoveEvent *e);
        void dragLeaveEvent(QDragLeaveEvent *e);
        void dropEvent(QDropEvent *e);

    public slots:
        void toInsert(const QString &text);
        void insertCompletion(const QString &completion);
        void appendText(const QString &text);

    private slots:
        void updateLineNumberAreaWidth(int newBlockCount);
        void highlightCurrentLine();
        void updateLineNumberArea(const QRect &, int);

    private:
        QString nextWord;
        QWidget *lineNumberArea;
        QString textUnderCursor() const;
        QCompleter *c;

    signals:
        void sDropEvent(char *url);
};

#endif
