#include "aboutwindow.h"
#include "ui_aboutwindow.h"

/**
 * @brief AboutWindow::AboutWindow
 * @param parent
 */
AboutWindow::AboutWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::AboutWindow)
{
    ui->setupUi(this);
}

/**
 * @brief AboutWindow::~AboutWindow
 */
AboutWindow::~AboutWindow()
{
    delete ui;
}
