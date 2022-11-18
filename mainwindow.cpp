#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    MaxChannelNum(0)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_comboBox_currentTextChanged(const QString &arg1)
{
    int channel = ui->comboBox->currentText().toInt();
    MaxChannelNum = channel;
    cout << "channel " << channel << endl;
    udpsocketSp = std::make_shared<UDP>(channel, "192.168.0.119", 0, 4010);
    ui->comboBox->close();
    QString text("当前通道数： ");
    QString numt;
    numt.setNum(channel);
    text = text + numt;
    ui->lineEdit->setText(text);
    connect(udpsocketSp.get(), SIGNAL(sendExit()), this, SLOT(exit()));
    connect(udpsocketSp.get(), SIGNAL(sendReinit()), this, SLOT(reinit()));
}

void MainWindow::exit()
{
    this->deleteLater();
}

void MainWindow::reinit()
{
    udpsocketSp.reset();
    udpsocketSp = std::make_shared<UDP>(MaxChannelNum, "192.168.0.119", 0, 4010);
}
