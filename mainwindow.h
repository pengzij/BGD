#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <memory>
#include "udpconnect.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private slots:
    void on_comboBox_currentTextChanged(const QString &arg1);
    void reinit();
    void exit();

private:
    Ui::MainWindow *ui;
    std::shared_ptr<UDP> udpsocketSp;
    int MaxChannelNum;
};

#endif // MAINWINDOW_H
