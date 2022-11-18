#include "udpconnect.h"


UDP::UDP(int _MaxChannelNum, QString ip, int udp_ctl, int port) //default read spec
    : writeBuffer(QByteArray::fromHex(COMMAND[udp_ctl]) ),
      timersp(new QTimer),
      waitTimersp(new QTimer),
      bombTimersp(new QTimer),
      pSocket(make_shared<QUdpSocket>(this)),
      maxChannelNum(_MaxChannelNum)
{

    timersp->setTimerType(Qt::PreciseTimer);
    timersp->setInterval(60000);//1min定时器
    waitTimersp->setTimerType(Qt::PreciseTimer);
    waitTimersp->setInterval(2000);//2ms
    bombTimersp->setTimerType(Qt::PreciseTimer);
    bombTimersp->setInterval(60000 * 60 * 4);//5h bomb
    if(udp_ctl == 1)
    {
        pSocket->bind(QHostAddress::AnyIPv4, 8080);
        pSocket->writeDatagram(writeBuffer, QHostAddress(ip), port);
        cout << "Init Read Wave Lenght" << endl;
        connect(pSocket.get(), SIGNAL(readyRead()),this, SLOT(readWLDataSlot()));
    }
    else if(udp_ctl == 0)
    {

        pSocket->bind(QHostAddress::AnyIPv4, 8080);
        //maxChannelNum = 5;
        OriDataQueSp = make_shared<CirQueue<QByteArray>>(5 * maxChannelNum * 10000);//6024 * 5 * maxChannelNum * 20一个包6024字节， 5个包， 通道数， 频率低于20Hz
        MessagequeSp = make_shared<CirQueue<pair<int,int>>>(maxChannelNum * 10000); //maxChannelNum * 100
        SpOriData.resize(maxChannelNum);
        cout << SpOriData.size() << endl;
        recivedPacNum = 0;
        curChannelNum = 0;

        writeBuffer += QByteArray::fromHex("0");
        pSocket->writeDatagram(writeBuffer, QHostAddress(ip), port);
        waitTimersp->start();

        saveFile.resize(maxChannelNum);
        of.resize(maxChannelNum);
        flashFileName();
        timersp->start();

        bombTimersp->start();
        connect(waitTimersp.get(), SIGNAL(timeout()), this, SLOT(reSend()));
        connect(this, SIGNAL(sendWriteAble()), this, SLOT(restoreSPData()));
        connect(timersp.get(), SIGNAL(timeout()), this, SLOT(flashFileName()) );
        connect(pSocket.get(), SIGNAL(readyRead()),this, SLOT(readSPDataSlot()));
        connect(bombTimersp.get(), SIGNAL(timeout()), this, SLOT(exit()) );
        cout << "Init Read Spectrum" << endl;
//
    }
}

void UDP::exit()
{
    cout << "exit" << endl;
    //this->deleteLater();
    sendExit();

}

//not used
//UDP::UDP(QString ip, int udp_ctl, int port)
//    : writeBuffer(QByteArray::fromHex(COMMAND[udp_ctl]) ),
//      timersp(new QTimer),
//      pSocket(make_shared<QUdpSocket>(this))
//{

//    timersp->setTimerType(Qt::PreciseTimer);
//    timersp->setInterval(60000);//1min定时器
//    if(udp_ctl == 1)
//    {
//        pSocket->bind(QHostAddress::AnyIPv4, 8080);
//        pSocket->writeDatagram(writeBuffer, QHostAddress(ip), port);
//        cout << "Init Read Wave Lenght" << endl;
//        connect(pSocket.get(), SIGNAL(readyRead()),this, SLOT(readWLDataSlot()));
//    }
//    else if(udp_ctl == 0)
//    {

//        pSocket->bind(QHostAddress::AnyIPv4, 8080);
//        maxChannelNum = 5;
//        OriDataQueSp = make_shared<CirQueue<QByteArray>>(5 * maxChannelNum * 10000);//6024 * 5 * maxChannelNum * 20一个包6024字节， 5个包， 通道数， 频率低于20Hz
//        MessagequeSp = make_shared<CirQueue<pair<int,int>>>(maxChannelNum * 10000); //maxChannelNum * 100
//        SpOriData.resize(maxChannelNum + 1);
//        cout << SpOriData.size() << endl;
//        recivedPacNum = 0;
//        curChannelNum = 0;
//        writeBuffer += QByteArray::fromHex("0");
//        pSocket->writeDatagram(writeBuffer, QHostAddress(ip), port);

//        saveFile.resize(maxChannelNum + 1);
//        of.resize(maxChannelNum + 1);
//        flashFileName();
//        timersp->start();
//        connect(this, SIGNAL(sendWriteAble()), this, SLOT(restoreSPData()));
//        connect(timersp.get(), SIGNAL(timeout()), this, SLOT(flashFileName()) );
//        connect(pSocket.get(), SIGNAL(readyRead()),this, SLOT(readSPDataSlot()));
//        cout << "Init Read Spectrum" << endl;
////
//    }
//}


UDP::UDP(QString ip, int port)
    : writeBuffer(QByteArray::fromHex(STARTCONNECT) ),
      pSocket(make_shared<QUdpSocket>(this))
{
    pSocket->bind(QHostAddress::AnyIPv4, 8080);
    pSocket->writeDatagram(writeBuffer, QHostAddress(ip), port);
    connect(pSocket.get(), SIGNAL(readyRead()),this, SLOT(readDate()));
}

UDP::~UDP()
{
    writeBuffer = QByteArray::fromHex(DISCONNECT);
    pSocket->writeDatagram(writeBuffer, QHostAddress(IP), PORT);
    pSocket->close();
    while(pSocket->isOpen());
    cout << "UDP disconnect" << endl;
}

void UDP::reSend()
{
    waitTimersp->stop();
    auto disconnectbuff = QByteArray::fromHex(DISCONNECT);
    pSocket->writeDatagram(disconnectbuff, QHostAddress(IP), PORT);
    Sleep(200);//2s
    pSocket->writeDatagram(writeBuffer, QHostAddress(IP), PORT);
    cout << "resend : recivedPacNum = " << recivedPacNum << " curChannelNum = " << curChannelNum << endl;
    waitTimersp->start();//restart
    //emit sendReinit();
}

void UDP::flashFileName()
{
    QMutexLocker locker(&writeLock);
    for(auto& off : of)
    {
        off.close();
    }
    QDateTime systemDate = QDateTime::currentDateTime();
    QTime systemTime = QTime::currentTime();
    QString fileTime = systemDate.toString("yyyyMMdd")+systemTime.toString("hhmmss");
    shared_ptr<QDir> folderSp(new QDir( QDir::currentPath() ));
    QString absolutePath = folderSp->absolutePath();
    QString savePath = absolutePath + "/spectrum data";
    shared_ptr<QDir> saveFolderSp(new QDir(savePath));
    qDebug() << savePath << endl;
    if(!saveFolderSp->exists())
    {
        saveFolderSp->mkdir(savePath);
        qDebug() << "存储光谱数据文件夹创建成功";
    }
    for(int i = 0; i < maxChannelNum; i++)
    {
        saveFile[i] = savePath + QString("/[CH")+QString::number(i + 1)
                +QString("]") + fileTime + QString(".bin");
        of[i] = ofstream(saveFile[i].toStdString().data(), ofstream::binary);
    }
}

void UDP::restoreSPData()
{
    QMutexLocker locker(&writeLock);
//    vector<int> OnceEnd(maxChannelNum + 1);
    int Messquelen = MessagequeSp->size();
    while(Messquelen--)
    {
        int CHnum = MessagequeSp->front().first;
        MessagequeSp->pop();
        //cout << "CHnum = " << CHnum << endl;
        const QByteArray &CHData = OriDataQueSp->front();
        //cout << "Messquelen = " << Messquelen << " CHData.size() = " << CHData.size() << endl;
        for(int j = 1; j < CHData.size(); j += 2)
        {
            //cout << hex << static_cast<unsigned short>(CHData[j - 1]) << " " <<  static_cast<unsigned short>(CHData[j]) << endl;
            unsigned short Di = static_cast<unsigned short>(CHData[j - 1]) + static_cast<unsigned short>(CHData[j]) << 8;
            float Df = static_cast<float>(Di) / 10;
            //cout << Df << endl;
            of[CHnum].write((const char*) &Df, sizeof(float));
        }
//        OnceEnd[CHnum]++;
//        if(OnceEnd[CHnum] == 4)//单次扫描结果存储完成
//        {
//            float fn = CHnum * 1.0;
//            of[CHnum].write((const char*) &fn, sizeof(float));
//            OnceEnd[CHnum] = 0;
//        }
        OriDataQueSp->pop();
    }
}

void UDP::send(const QByteArray &wbuff)
{
    pSocket->writeDatagram(wbuff, QHostAddress(IP), PORT);
    waitTimersp->start();//restart
}

/* read spectrum ori data */
/* only first 5 packages are needed */
void UDP::readSPDataSlot()
{
    //QMutexLocker locker(&writeLock);

    QByteArray buff;
    buff.resize(pSocket->bytesAvailable());
    pSocket->readDatagram(buff.data(), buff.size());
    //cout << "buff.size() = " << buff.size() << endl;
    buff.remove(0, 4);//去掉包头
    //cout << "recivedPacNum = " << recivedPacNum << endl;
    if(recivedPacNum == 5)
    {
        buff.clear();
        curChannelNum++;
        recivedPacNum = 0;
        if(curChannelNum >= maxChannelNum)
        {
            curChannelNum = 0;
            sendWriteAble();
        }
        /* write next channelNum Datagram */
        QByteArray CHn;
        CHn.setNum(curChannelNum);
        writeBuffer = QByteArray::fromHex(RDSPECTRUM) + QByteArray::fromHex(CHn);
        send(writeBuffer);
    }
    else if(recivedPacNum == 4)
    {
        buff.clear();
        recivedPacNum++;
    }
    else
    {
        //cout << "curChannelNum = " << curChannelNum << endl;
        OriDataQueSp->push(buff);
        //cout << "OriDataQueSp.size() = " << OriDataQueSp->size() << endl;
        MessagequeSp->push({curChannelNum, recivedPacNum});
        //cout << "OriDataQueSp.size() = " << MessagequeSp->size() << endl;
        recivedPacNum++;
    }

}

void UDP::readWLDataSlot()
{
    buffer.clear();
    buffer.resize(pSocket->bytesAvailable());
    pSocket->readDatagram(buffer.data(), buffer.size());
    for(auto& s : buffer)
    {
        //cout << hex << static_cast<short>(s) << " ";
        //charQue.push(s);
    }
    //cout << endl;
}

