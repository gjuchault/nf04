#include "syntaxwindow.h"
#include "ui_syntaxwindow.h"

/**
 * @brief SyntaxWindow::SyntaxWindow
 * @param parent
 */
SyntaxWindow::SyntaxWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::SyntaxWindow)
{
    ui->setupUi(this);
}

/**
 * @brief SyntaxWindow::~SyntaxWindow
 */
SyntaxWindow::~SyntaxWindow()
{
    delete ui;
}
