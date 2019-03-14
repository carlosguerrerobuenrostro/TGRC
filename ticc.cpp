#include "ticc.h"

double TICC::count = 0.0; // global variable

TICC::TICC(QObject *parent) : QObject(parent)
{

}

void TICC::readSerialTIC()
{
       //  qint64 dataSize = 0;
         int     STX=0;
         int     ETX=0;

         TIC_Data += TIC-> readAll();
         qDebug() << TIC_Data << endl;

         STX = TIC_Data.indexOf(0x02,0);               // busco el caracter de inicio

        if ((TIC_Data.length() > 28) || (STX < 0))
        {
            TIC_Data.clear();                          // borro todo el buffer
        }

        if (STX >= 0)
        {
            TIC_Data.remove(0,STX);

        STX = TIC_Data.indexOf(0x02,0);
        ETX = TIC_Data.indexOf(0x03,STX);

        qDebug()    <<  "Index of STX"  <<  STX;
        qDebug()    <<  "Index of ETX"  <<  ETX;


            if (ETX > 0 ){

                TIC_Buffer=TIC_Data.mid(STX+1,(ETX-STX)-1); // extraigo el dato
                bool ok=true;
                count=TIC_Buffer.toDouble(&ok);                // lo convierto
                if(ok==false){
                    qDebug() << "Error" << endl;
                }
                count=count/1000;
                qDebug() <<  " COUNT = " << count << endl;
                TIC_Data.remove(STX,ETX+1);
                TIC_Data.clear();
                emit  countReady(); // oly emit signal when data is complete
            }

        }




}

void TICC::startTIC()
{ 
    /*
    *   Identify the ports
    */
    bool TIC_is_available = false;
    QString TIC_port_name;
    //
    //  For each available serial port
    foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts())
    {
       //  check if the serialport has both a product identifier and a vendor identifier
       if(serialPortInfo.hasProductIdentifier() && serialPortInfo.hasVendorIdentifier())
       {
           //  check if the product ID and the vendor ID match those of the GPS
           if((serialPortInfo.productIdentifier() == TIC_pid)
                   && (serialPortInfo.vendorIdentifier() == TIC_vid)){
               TIC_is_available = true; //    TIC is available on this port
               TIC_port_name = serialPortInfo.portName();
           }
       }//end if serial
    }//end foreach
    /*
    *  Open and configure the TIC port if available
    */
    if(TIC_is_available){
        //Serial Port TIC
        qDebug() << "Found the TIC" << TIC_port_name << endl;
        startTIC(TIC_port_name);
    }
    else{
        qDebug() << " TIC Demo Mode" << endl;
        //demo mode
        QTimer *timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(DemoMode()));
        timer->start(1000);
    }

}

void TICC::startTIC(QString PORTNAME)
{
TIC= new QSerialPort(this); //creo un nuevo objeto de puerto serial
TIC_Buffer="";

qDebug() << "Found the TIC" << PORTNAME << endl;
TIC->setPortName(PORTNAME);
TIC->open(QSerialPort::ReadOnly); //Read and Write Device, Unbuffered
TIC->setBaudRate(QSerialPort::Baud115200);
TIC->setDataBits(QSerialPort::Data8);
TIC->setFlowControl(QSerialPort::NoFlowControl);
TIC->setParity(QSerialPort::NoParity);
TIC->setStopBits(QSerialPort::OneStop);
TIC->flush();
QObject::connect(TIC, SIGNAL(readyRead()), this, SLOT(readSerialTIC()));

}

void TICC::DemoMode()
{
    int seed=200;
    int a=qrand()%((((seed/10)+1)-0)+0);
    count= (seed+a);
    emit  countReady();
}






