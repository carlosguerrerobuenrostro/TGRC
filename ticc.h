#ifndef TICC_H
#define TICC_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include <QTimer>


class TICC : public QObject
{
    Q_OBJECT
public:
    explicit TICC(QObject *parent = nullptr);
  //  ~TICC();
    static double  count;

private:
    QSerialPort *TIC;
    QByteArray TIC_Data;
    QString TIC_Buffer;
    // VID & PID TAPR TICC
    static const quint16 TIC_vid = 9025;
    static const quint16 TIC_pid = 66;

signals:
    double countReady();

public slots:   
    void startTIC();
    void startTIC(QString PORTNAME);

private slots:
    void DemoMode();
    void readSerialTIC();
};

#endif // TICC_H
