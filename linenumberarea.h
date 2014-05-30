#ifndef LINENUMBERAREA_H
#define LINENUMBERAREA_H

#include "stable.h"
#include "codeeditor.h"

class CodeEditor;

/*
 * Classe wrapper qui va appeler ses composantes dans codeEditor
 */
class LineNumberArea : public QWidget
{
    public:
        LineNumberArea(CodeEditor *ce);
        QSize sizeHint() const;

    protected:
        void paintEvent(QPaintEvent *event);

    private:
        CodeEditor *codeEditor;
};

#endif // LINENUMBERAREA_H
