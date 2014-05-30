#ifndef SYNTAXWINDOW_H
#define SYNTAXWINDOW_H

#include "stable.h"

namespace Ui
{
    class SyntaxWindow;
}

class SyntaxWindow : public QMainWindow
{
        Q_OBJECT

    public:
        explicit SyntaxWindow(QWidget *parent = 0);
        ~SyntaxWindow();

    private:
        Ui::SyntaxWindow *ui;
};

#endif // SYNTAXWINDOW_H
