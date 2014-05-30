#ifndef ALGOUTTSYNTAXHIGHLIGHTER_H
#define ALGOUTTSYNTAXHIGHLIGHTER_H

#include "stable.h"
#include "codeeditor.h"

class AlgoUttSyntaxHighLighter : public QSyntaxHighlighter
{
    public:
        // Mots provenant d'autocompletion.txt
        QStringList words;
        bool dark;
        AlgoUttSyntaxHighLighter(QTextDocument *document, bool dark);

        void setDark(bool dark);

        void highlightBlock(const QString &text);
};

#endif // ALGOUTTSYNTAXHIGHLIGHTER_H
