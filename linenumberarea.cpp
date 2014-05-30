#include "linenumberarea.h"

/**
 * @brief LineNumberArea::LineNumberArea
 * @param editor
 */
LineNumberArea::LineNumberArea(CodeEditor *editor) : QWidget(editor)
{
    codeEditor = editor;
}

/**
 * @brief LineNumberArea::sizeHint
 * @return
 */
QSize LineNumberArea::sizeHint() const
{
    return QSize(codeEditor->lineNumberAreaWidth(), 0);
}

/**
 * @brief LineNumberArea::paintEvent
 * @param event
 */
void LineNumberArea::paintEvent(QPaintEvent *event)
{
    codeEditor->lineNumberAreaPaintEvent(event);
}
