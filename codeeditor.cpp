#include "codeeditor.h"

/**
 * @brief CodeEditor::CodeEditor
 * @param parent
 * @param dark
 */
CodeEditor::CodeEditor(QWidget *parent, bool dark) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);
    this->dark = dark;
    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect, int)), this, SLOT(updateLineNumberArea(QRect, int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    // Algorithme par défaut
    setPlainText("Algorithme <Nom>\nTypes:\nVariables:\nInstructions:\n");
    // Couleur du curseur
    QPalette p = palette();
    p.setColor(QPalette::Active, QPalette::Base, (dark) ? QColor("#272822") : QColor("#FFF"));
    p.setColor(QPalette::Inactive, QPalette::Base, (dark) ? QColor("#272822") : QColor("#FFF"));
    QString _c = (dark) ? "#FFF" : "#000";
    setStyleSheet(QString("color: ") + _c + QString(";"));
    setPalette(p);
    setAcceptDrops(true);
}

/**
 * @brief CodeEditor::~CodeEditor
 */
CodeEditor::~CodeEditor()
{
}

/**
 * @brief CodeEditor::setDark
 * @param dark
 */
void CodeEditor::setDark(bool dark)
{
    this->dark = dark;
}

/**
 * @brief CodeEditor::lineNumberAreaWidth
 * @return
 */
int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());

    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 12 + fontMetrics().width(QLatin1Char('9')) * digits;
    return space;
}

/**
 * @brief CodeEditor::updateLineNumberAreaWidth
 */
void CodeEditor::updateLineNumberAreaWidth(int)
{
    // Change la marge du CodeEditor
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

/**
 * @brief CodeEditor::updateLineNumberArea
 * @param rect
 * @param dy
 */
void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy != 0) {
        lineNumberArea->scroll(0, dy);
    } else {
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

/**
 * @brief CodeEditor::resizeEvent
 * @param e
 */
void CodeEditor::resizeEvent(QResizeEvent *e)
{
    // Quand on resize la fenêtre, change la taille de la gutter
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

/**
 * @brief CodeEditor::highlightCurrentLine
 */
void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = (dark) ? QColor("#3E3D32") : QColor("#E8F2FF");
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

/**
 * @brief CodeEditor::goToLine
 * @param l
 */
void CodeEditor::goToLine(int l)
{
    QTextCursor cur = this->textCursor();
    int currentLine = cur.blockNumber() + 1;

    if (l > currentLine) {
        cur.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, l - currentLine);
    } else {
        cur.movePosition(QTextCursor::PreviousBlock, QTextCursor::MoveAnchor, currentLine - l);
    }

    setTextCursor(cur);
}

/**
 * @brief CodeEditor::lineNumberAreaPaintEvent
 * @param event
 */
void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberArea->width(),
                                     fontMetrics().height(),
                             Qt::AlignRight, number.append(" ").prepend(" "));
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

/**
 * @brief CodeEditor::completer
 * @return
 */
QCompleter *CodeEditor::completer() const
{
    return this->c;
}

/**
 * Re-définition du completer.
 *
 * @brief CodeEditor::setCompleter
 * @param completer
 */
void CodeEditor::setCompleter(QCompleter *completer)
{
    this->c = completer;

    if (completer == NULL) {
        return;
    }

    c->setWidget(this);
    c->setCompletionMode(QCompleter::PopupCompletion);
    c->setCaseSensitivity(Qt::CaseInsensitive);
    //QObject::connect(c, SIGNAL(activated(const QString &)), this, (insertCompletion(const QString &)));
    QObject::connect(c, SIGNAL(highlighted(QString)), this, SLOT(toInsert(QString)));
}

/**
 * Insertion d'un mot choisi par l'utilisateur.
 *
 * @brief CodeEditor::insertCompletion
 * @param completion
 */
void CodeEditor::insertCompletion(const QString &completion)
{
    if (c->widget() != this) {
        return;
    }

    // Si l'utilisateur a choisi un mot parmis l'autocomplétion, on l'ajoute
    QTextCursor tc = textCursor();
    int extra = completion.length() - this->c->completionPrefix().length();
    tc.insertText(completion.right(extra));
    setTextCursor(tc);
}

/**
 * @brief CodeEditor::toInsert
 * @param text
 */
void CodeEditor::toInsert(const QString &text) {
    this->nextWord = text;
}

/**
 * @brief CodeEditor::appendText
 * @param text
 */
void CodeEditor::appendText(const QString &text)
{
    if (c->widget() != this) {
        return;
    }

    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::NextBlock);
    tc.insertText(text);
    setTextCursor(tc);
}

/**
 * @brief CodeEditor::textUnderCursor
 * @return
 */
QString CodeEditor::textUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

/**
 * @brief CodeEditor::focusInEvent
 * @param e
 */
void CodeEditor::focusInEvent(QFocusEvent *e)
{
    if (this->c != NULL) {
        this->c->setWidget(this);
    }
    QPlainTextEdit::focusInEvent(e);
}

/**
 * Gestion du clavier.
 *
 * @brief CodeEditor::keyPressEvent
 * @param e
 */
void CodeEditor::keyPressEvent(QKeyEvent *e)
{
    // On cache la popup de completion en cas d'appui sur ESC.
    if (e->key() == Qt::Key_Escape) {
        if (c && c->popup()->isVisible()) {
            c->popup()->hide();
        }
        return;
    }
    // Expliciter les constantes !
    else if (e->key() == Qt::Key_Enter || e->key() == 16777220 || e->key() == 16777221) {
        if (this->c && this->c->popup()->isVisible()) {
            insertCompletion(nextWord);
            this->nextWord = "";
            this->c->popup()->hide();
            return;
        }
    }

    // Si on fait Ctrl+Space / Cmd+Space
    bool isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_Space);

    // Si c'est une touche "classique", on forward au TextEdit
    if ( ! this->c || ! isShortcut) {
        QPlainTextEdit::keyPressEvent(e);
    }

    // Si il n'y a que la touche Ctrl ou Shift, on ignore.
    const bool ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);

    if ( ! this->c || (ctrlOrShift && e->text().isEmpty())) {
        return;
    }

    // Derniers caractères spéciaux
    static QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-=");
    bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;
    QString completionPrefix = textUnderCursor();

    // Si une condition n'est pas valide ou est dévalidée en tapant du texte, on cache la box de completion
    if ( ! isShortcut && (hasModifier
                          || e->text().isEmpty()
                          || completionPrefix.length() < 1
                          || eow.contains(e->text().right(1)))) {
        c->popup()->hide();
        return;
    }

    if (completionPrefix != this->c->completionPrefix()) {
        this->c->setCompletionPrefix(completionPrefix);
        this->c->popup()->setCurrentIndex(this->c->completionModel()->index(0, 0));
    }

    QRect cr = cursorRect();
    cr.setWidth(c->popup()->sizeHintForColumn(0) + c->popup()->verticalScrollBar()->sizeHint().width());
    this->c->complete(cr);
}

/**
 * @brief CodeEditor::dragEnterEvent
 * @param e
 */
void CodeEditor::dragEnterEvent(QDragEnterEvent *e)
{
    e->acceptProposedAction();
}

/**
 * @brief CodeEditor::dragMoveEvent
 * @param e
 */
void CodeEditor::dragMoveEvent(QDragMoveEvent *e)
{
    e->acceptProposedAction();
}

/**
 * @brief CodeEditor::dragLeaveEvent
 * @param e
 */
void CodeEditor::dragLeaveEvent(QDragLeaveEvent *e)
{
    e->accept();
}

/**
 * @brief CodeEditor::dropEvent
 * @param e
 */
void CodeEditor::dropEvent(QDropEvent *e)
{
    const QMimeData *mimeData = e->mimeData();

    QList<QUrl> urlList;
    if (mimeData->hasUrls()) {
        urlList = mimeData->urls();
        // .mid(8) => on enlève file:///
        char *url = (char *) urlList.at(0).toString().mid(8).toStdString().c_str();
        sDropEvent(url);
    }
}
