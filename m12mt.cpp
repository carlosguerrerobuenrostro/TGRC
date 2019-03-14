#include "m12mt.h"
//
// global variables
QTime M12MT::GPStime;
QDate M12MT::GPSdate;
qint8 M12MT::nvs;
qint8 M12MT::nts;
qint8 M12MT::negative_sawtooth;
qint8 M12MT::last_negative_sawtooth;
double M12MT::latitud;
double M12MT::longitud;
double M12MT::altitud;
QString M12MT::status;
bool M12MT::DEMOmode;

//
M12MT::M12MT(QObject *parent) : QObject(parent)
{

}

void M12MT::startGPS(){
    /*
    *   Identify the ports
    */
    bool GPS_is_available = false;
    QString GPS_port_name;
    //
    //  For each available serial port
    foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts())
    {
       //  check if the serialport has both a product identifier and a vendor identifier
       if(serialPortInfo.hasProductIdentifier() && serialPortInfo.hasVendorIdentifier())
       {
           //  check if the product ID and the vendor ID match those of the GPS
           if((serialPortInfo.productIdentifier() == GPS_pid)
                   && (serialPortInfo.vendorIdentifier() == GPS_vid)){
               GPS_is_available = true; //    TIC is available on this port
               GPS_port_name = serialPortInfo.portName();
           }
       }//end if serial
    }//end foreach
    /*
    *  Open and configure the TIC port if available
    */
    if(GPS_is_available){
        //Serial Port TIC
        qDebug() << "Found the GPS" << GPS_port_name << endl;
        startGPS(GPS_port_name);
    }
    else{
        qDebug() << "GPS Demo Mode" << endl;
        //demo mode
        QTimer *timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(DemoMode()));
        timer->start(1000);
    }
}

void M12MT::startGPS(QString PORTNAME)
{
/*
*  Open and configure the GPS port if available
*/
    GPS= new QSerialPort(this); // new serial port object
    //Serial Port GPS
   // qDebug() << "Found the GPS" << PORTNAME << endl;
    GPS->setPortName(PORTNAME);
    GPS->open(QSerialPort::ReadWrite); //Read and Write Device, Unbuffered
    GPS->setBaudRate(QSerialPort::Baud9600);
    GPS->setDataBits(QSerialPort::Data8);
    GPS->setFlowControl(QSerialPort::NoFlowControl);
    GPS->setParity(QSerialPort::NoParity);
    GPS->setStopBits(QSerialPort::OneStop);
    DEMOmode=false;
    QObject::connect(GPS, SIGNAL(readyRead()), this, SLOT(readSerialGPS()));
}

void M12MT::readSerialGPS()
{
    QByteArray _STX("@@");
    QByteArray _ETX("\r\n");
    QByteArray Ha("Ha");
    QByteArray Hn("Hn");
    QByteArray Bb("Bb");
    QByteArray Cb("Cb"); //almanac
    // Respuestas de la secuencia de inicio del GPS
    QByteArray Cf("Cf");    // Set to Defaults
    QByteArray Sc("Sc");    // Set Constellation Mode to GPS Only
    QByteArray St("St");    // Set sets time & 1PPS alignment source to UTC(USNO)
    //
    QByteArray Gb("Gb");    // Set DateTime to GPS   // DEsactivado
    QByteArray Gc("Gc");    // Set PPS to one sat only
    QByteArray Ch("Ch");    // Almanac input
    QByteArray As("As");    // Position Hold input
    QByteArray Gd("Gd");    // Set Position Hold
    QByteArray Ay("Ay");    // Time Offset Command
    QByteArray Az("Az");    // Cable Delay Correction
    QByteArray Ge("Ge");    // Set Enable T-RAIM
    //
    QByteArray  _CMD;
    //
    int     STX=0;
    int     ETX=0;
    qint64 dataSize = 0;
    quint16 cmd_lenght=0;

    dataSize = GPS->bytesAvailable(); //determino numero de caracteres en el puerto
   //qDebug() <<   "GPS Bytes Available: " << dataSize << "\n";
    serialData = serialData + GPS-> read(dataSize); //leo el numero de caracteres
   //qDebug() << serialData <<endl;
   //qDebug() << serialData.size() << endl;

    if (response_lenght==0)
    {
        if(serialData.size()>=6)
        {
           //Find STX (@@)
           STX = serialData.indexOf(_STX,0);
           if(STX >= 0)
           {
        //   qDebug()    <<  "Index of STX"  <<  STX;
           _CMD=serialData.mid(STX+2,2);
          // qDebug()  << "CMD: " << _CMD  ;

           if (_CMD==Ha){
               cmd_lenght=154;
           }

           if (_CMD==Hn){
               cmd_lenght=78;
           }

           if (_CMD==Bb){
               cmd_lenght=92;
           }

           ETX = serialData.indexOf(_ETX,STX+cmd_lenght-4);
       //     qDebug()    <<  "Index of ETX"  << ETX;

           }

         //   qDebug()    <<  "Index of ETX"  <<  serialData.indexOf(_ETX,0);

              if (ETX > 0)
              {
                  if (_CMD==Ha){
                  Ha_Data(serialData.mid(STX,ETX+2));
                  serialData.remove(STX,ETX+2);
                  CMD_Ha=true;
                  }

                  if (_CMD==Hn){
                  Hn_Data(serialData.mid(STX,ETX+2));
                  serialData.remove(STX,ETX+2);
                  CMD_Hn=true;
                  }

                  if (_CMD==Bb){
                  Bb_Data(serialData.mid(STX,ETX+2));
                  serialData.remove(STX,ETX+2);
                  CMD_Bb=true;
                  }
                   //
                  if(CMD_Ha & CMD_Hn & CMD_Bb){
                   emit gpsReady();
                   CMD_Ha=false;
                   CMD_Hn=false;
                   CMD_Bb=false;

                  }
              }
        }

    }
    else if (serialData.size()>=response_lenght)
    {
     qDebug() << "[" << serialData.size() << "] = " << serialData << endl;
    //
     STX = serialData.indexOf(_STX,0);
         if(STX >= 0)
         {
             _CMD=serialData.mid(STX+2,2);
            //Aqui defino la secuencia de comandos iniciales, esto es muy mal hecho y necesita cambiarse
             if (_CMD==Cf){
                  qDebug() << "GPS Set to Defaults" << endl;
                  serialData.clear();
             }

             if (_CMD==Sc){
             qDebug() << "Constallation Set to GPS Only" << endl;
                 SetDateTimePPSAlignment();
             }

             if (_CMD==St){
                 qDebug() << "Time & 1PPS aligned to UTC(USNO)" << endl;
                //TimeMessage();
                 PPSOneSat();
             }

             if (_CMD==Gb){
                  qDebug() << "GPS Set Date and Time" << endl;
                  PPSOneSat();
             }
             if (_CMD==Gc){
                  qDebug() << "1PPS Control Message Set to active only when tracking at least one satellite" << endl;
                  SendPosition("/config/binary.pos");
             }
 //           if (_CMD==Ch){
   //              qDebug() << "Almanac Data Input" << endl;
//                 SendPosition();
     //        }
             if (_CMD==As){
                 qDebug() << "Position to Hold Input" << endl;
                 SendPositionHold();
             }
             if (_CMD==Gd){
                  qDebug() << "Position Hold Mode" << endl;
                  EnableTRAIM();
             }
             if (_CMD==Ay){
                 qDebug()<< "Advanced 1PPS 5ms" << endl;
                // Cable_delay();
             }
             if (_CMD==Az){
               //  qDebug()<< "Cable delay corrected " << QString::number(cable_delay,10) << " ns" << endl;
                // EnableTRAIM();
             }
             if (_CMD==Ge){
                qDebug() << "T-RAIM Enabled" << endl;
            //      StartPolling();
             }
         }
     response_ready=true;
     serialData.clear();
    }
}

void M12MT::DemoMode()
{
    // Date
    QDateTime cdt = QDateTime::currentDateTime();
    GPStime=cdt.toUTC().time();
    GPSdate=cdt.toUTC().date();
    nvs=0;
    nts=0;
    negative_sawtooth=0;
    status="DemoMode";
    //CENAM google position
    latitud=20.535605;
    longitud=-100.258345;
    altitud=1917.16;
    //
    DEMOmode=true;
    emit  gpsReady();
}

void M12MT::Bb_Data(QByteArray Bb)
{
    // 92 Bytes
    quint8 SAT_ID;
    // quint8  number_sats;
    // Start @ @ B b Data:   0,1,2,3
    // number_sats = Bb.at(4);
    // Satellite data are from byte 5 to 89
    for(int i=0 ; i < 12 ; i++)
    {
         SAT_ID=static_cast<quint8>(Bb.at(5+i*7)); // 63
         if (SAT_ID < 33){  // only GPS real sats
         SVID[SAT_ID].Doppler=static_cast<qint16>(Bb.at(6+i*7)<<8 & 0xFF00) | (Bb.at(7+i*7) & 0x00FF);
         SVID[SAT_ID].elevation=static_cast<quint8>(Bb.at(8+i*7));
         SVID[SAT_ID].azimuth=static_cast<quint16>(Bb.at(9+i*7)<<8 & 0xFF00) | (Bb.at(10+i*7) & 0x00FF);
         SVID[SAT_ID].sat_health=static_cast<qint8>(Bb.at(11+i*7));
         }
    }
}

void M12MT::Ha_Data(QByteArray Ha)
{
    //154 Bytes
    quint8 SAT_ID;
    // Start @ @ H a Data:   0,1,2,3
    // Date  m d y y Data:   4,5,6,7
    GPSdate.setDate((Ha.at(6)<<8 & 0xFF00) | (Ha.at(7) & 0x00FF),Ha.at(4),Ha.at(5));
    // Time h m s    Data:   8,9,10
    GPStime.setHMS(Ha.at(8),Ha.at(9),Ha.at(10),0);
    // nanoSeconds   Data:   11,12,13,14
    ///////////////////////////////////
    // Latitude FS   Data:   15,16,17,18
    // Longitud FS   Data:   19,20,21,22
    // GPS Height FS Data:   23,24,25,26
    // MSL Height FS Data:   27,28,29,30
    ///////////////////////////////////
    // Latitud AU    Data:   31,32,33,34
    rawpos.resize(12);
    latitudAU= static_cast<qint32>((static_cast<unsigned int>(Ha.at(31) << 24) & 0xFF000000) | (Ha.at(32) << 16 & 0x00FF0000) | (Ha.at(33) << 8 & 0x0000FF00)| (Ha.at(34) & 0x000000FF));
    rawpos[0]=Ha.at(31);
    rawpos[1]=Ha.at(32);
    rawpos[2]=Ha.at(33);
    rawpos[3]=Ha.at(34);
    latitud=static_cast<double> (latitudAU)/3600000;
    // Longitud AU   Data:   35,36,37,38
    longitudAU=static_cast<qint32>((static_cast<unsigned int>(Ha.at(35) << 24) & 0xFF000000) | (Ha.at(36) << 16 & 0x00FF0000) | (Ha.at(37) << 8 & 0x0000FF00)| (Ha.at(38) & 0x000000FF));
    rawpos[4]=Ha.at(35);
    rawpos[5]=Ha.at(36);
    rawpos[6]=Ha.at(37);
    rawpos[7]=Ha.at(38);
    longitud=static_cast<double> (longitudAU)/3600000;
    // GPS Height AU Data:   39,40,41,42
    heightAU=static_cast<qint32>((static_cast<unsigned int>(Ha.at(39) << 24) & 0xFF000000) | (Ha.at(40) << 16 & 0x00FF0000) | (Ha.at(41) << 8 & 0x0000FF00)| (Ha.at(42) & 0x000000FF));
    rawpos[8]=Ha.at(39);
    rawpos[9]=Ha.at(40);
    rawpos[10]=Ha.at(41);
    rawpos[11]=Ha.at(42);
    altitud = static_cast<double> (heightAU)/100;
    // MSL Height AU Data:   43,44,45,46
    // Speed 3D      Data:   47,48
    // Speed 2D      Data:   49,50
    // Heading 2D    Data:   51,52
    // Geometry      Data:   53,54
    // Satellite     Data:   55,56
    nvs=Ha.at(55);     //Number of visible satellites  0...12
    nts=Ha.at(56);     //Number of tracked satellites  0...12
    //Satellites    Data:   57-128
    for(int i=0 ; i <12 ; i++){
         SAT_ID=static_cast<quint8> (Ha.at(57+i*6)); //63
         //Validar que solo vea GPS
         if (SAT_ID < 33){
         SVID[SAT_ID].mode=static_cast<quint8> (Ha.at(58+i*6)); //64
         SVID[SAT_ID].signal= static_cast<quint8> (Ha.at(59+i*6));
         SVID[SAT_ID].IODE= static_cast<quint8> (Ha.at(60+i*6));
         SVID[SAT_ID].status=static_cast<quint16> ((Ha.at(61+i*6)<<8 & 0xFF00) | (Ha.at(62+i*6) & 0x00FF));
        /*  qDebug()    << "SatNum"  << QString::number(i+1, 10)
         *              <<  QString::number(SVID[i].mode, 10)
         *              <<  QString::number(SVID[i].signal, 10)
         *              <<  QString::number(SVID[i].IODE, 10) << endl;
        */
         }
    }


    // Receiver Status ss:   129,130
    receiver_status= (Ha.at(129)<<8 & 0xFF00) | (Ha.at(130) & 0x00FF);
    ReceiverStatus(receiver_status);
    // Reserved      Data:   131,132
    // Oscillator and Clock Parameters
    // Clock Bias cc Data:   133,134
    // OscOffset ooooData:   135,136,137,138
    // Temperature   Data:   139,140
    // TimeModeUTC u Data:   141
    // GMTOffset shm Data:   142,143,144
    // IDstring      Data:   145,146,147,148,149,150
    ID.append(Ha.at(145));ID.append(Ha.at(146));ID.append(Ha.at(147));ID.append(Ha.at(148));ID.append(Ha.at(149));ID.append(Ha.at(150));
    // Stop  C,CR,LF Data:   151,152,153
    // Print Debugs
    // qDebug() << "Ha Command Raw Data"   << Ha.size() << Ha.toHex() << endl;
    // qDebug() << "Date & Time : "     << GPSdate.toString() << "," << GPStime.toString() << endl;
    // qDebug() << "Position (Lat,Lon,Alt): "   << latitud << "," << longitud << "," << height << endl;
    // qDebug() << "Satellites (Visible, Tracked): "  << QString::number(nvs, 10) << "," << QString::number(nts, 10)<< endl ;
    // qDebug() << "ID" << ID << endl;
}

void M12MT::Hn_Data(QByteArray Hn)
{
    // 78 Bytes
      quint8 SAT_ID;
      // Start @ @ H n Data:   0,1,2,3
      pulse_status = static_cast<quint8> (Hn.at(4)); // 0 = off; 1 = on
      PPS_sync = static_cast<quint8> (Hn.at(5));
      TimeRAIM_solution = static_cast<quint8> (Hn.at(6));
      TimeRAIM_status = static_cast<quint8> (Hn.at(7));
      TimeRAIM_svid = static_cast<quint32>  ((static_cast<unsigned int>((Hn.at(8) << 24)) & 0xFF000000) | (Hn.at(9) << 16 & 0x00FF0000) | (Hn.at(10) << 8 & 0x0000FF00)| (Hn.at(11) & 0x000000FF));
      time_solution = static_cast<quint16>  ((Hn.at(12)<<8 & 0xFF00) | (Hn.at(13) & 0x00FF));
      last_negative_sawtooth = negative_sawtooth;  // Guardo el anterior
      negative_sawtooth = static_cast<qint8> (Hn.at(14));               // Actualizo el sawtooth
      //
      // Satellite data are from byte 15
      for(int i=0 ; i < 12 ; i++)
      {
          SAT_ID = static_cast<quint8> (Hn.at(15+i*5)); //15
          if (SAT_ID < 33){
              SVID[SAT_ID].fractional_GPS_time= static_cast<quint32> ((static_cast<unsigned int>((Hn.at(16+i*5)) << 24) & 0xFF000000) | (Hn.at(17+i*5) << 16 & 0x00FF0000) | (Hn.at(18+i*5) << 8 & 0x0000FF00)| (Hn.at(19+i*5) & 0x000000FF));
           //    qDebug() << SAT_ID << SVID[SAT_ID].mode << SVID[SAT_ID].fractional_GPS_time;
          }
      }
//      //Print Debugs
//          if(!pulse_status){
//              qDebug() << "Pulse Status: OFF";
//          }
//          else{
//              qDebug() << "Pulse Status: ON";
//          }
//          if(!PPS_sync){
//              qDebug() << "Pulse Sync: UTC";
//          }
//          else{
//              qDebug() << "Pulse Sync: GPS";
//          }
//          qDebug() << "T-RAIM solution:" << QString::number(TimeRAIM_solution);
//          qDebug() << "T-RAIM status:" << QString::number(TimeRAIM_status);
//          qDebug() << "T-RAIM SVID:" << QString::number(TimeRAIM_svid,16);
//          qDebug() << "1 sigma accuracy:" << QString::number(time_solution);
//          qDebug() << "Negative Sawtooth" << QString::number(negative_sawtooth) << endl;
}

void M12MT::ReceiverStatus(quint16 rs)
{
    // qDebug() << "Status" << QString::number(rs,16) << endl;
     //Test Bits 13-15:
     switch((rs >> 13) & 0x0007)
     {
         case 0x00: status="Reserved";break;
         case 0x01: status="Reserved";break;
         case 0x02: status="Bad Geometry";break;
         case 0x03: status="Adquiring Satellites";break;
         case 0x04: status="Position Hold";break;
         case 0x05: status="Propagate Mode";break;
         case 0x06: status="2D Fix";break;
         case 0x07: status="3D Fix";break;
     }
     //Bits 11-12: Reserved

     //Test Bit10: Narrow band tracking mode
     if((rs&0x0400)){
         status=status+" Narrow Band";
     }
     //Test Bit9: Fast Acquisition Position
     if((rs&0x0200)){
         status=status+" Fast Acq";
     }
     //Test Bit8: Filter Reset To Raw GPS Solution
     if((rs&0x0100)){
         status=status+" Filter Reset";
     }

     //Test Bit7: Cold Start
     if((rs&0x0080)){
         status=status+" Cold Start";
     }
     //Test Bit6: Diferential Fix
     if((rs&0x0040)){
         status=status+" Differential Fix";
     }
     //Test Bit5: Position Lock
     if((rs&0x0020)){
         status=status+" Position Lock";
     }
     //Test Bit4: Autosurvey Mode
     if((rs&0x0010)){
         status="Survey Mode";
     }
     //Test Bit3: Insufficient Visible Satellites
     if((rs&0x0008)){
         status=status+" Insuf V. Sat";
     }

     //Test Bits 2-1:
     switch((rs >> 1) & 0x0003)
     {
         case 0x00: /*status=status+" Ant OK"*/;break;
         case 0x01: status="Ant OC";break;
         case 0x02: status="Ant UC";break;
         case 0x03: status="Ant NV";break;
     }
     //Test Bit0: Code Location
     if((rs&0x0001)){
        // status=status+" Internal";
     }else{
        // status=status+" External";
     }

}
// commands
void M12MT::StartPolling()
{
    //@@Hn Time RAIM Status Message
    static char Bb[]={0x40, 0x40, 0x42, 0x62, 0x01, 0x21, 0x0D, 0x0A};
    static char Ha[]={0x40, 0x40, 0x48, 0x61, 0x01, 0x28, 0x0D, 0x0A};
    static char Hn[]={0x40, 0x40, 0x48, 0x6E, 0x01, 0x27, 0x0D, 0x0A};
    //Send Triple Command
    sendcommand(Bb,sizeof(Bb),0);
    sendcommand(Ha,sizeof(Ha),0);
    sendcommand(Hn,sizeof(Hn),0);
}

void M12MT::SendTripleCMD()
{
     static char Bb[]={0x40, 0x40, 0x42, 0x62, 0x00, 0x20, 0x0D, 0x0A};
     static char Ha[]={0x40, 0x40, 0x48, 0x61, 0x00, 0x29, 0x0D, 0x0A};
     static char Hn[]={0x40, 0x40, 0x48, 0x6E, 0x00, 0x26, 0x0D, 0x0A};
    //Send Triple Command
     sendcommand(Bb,sizeof(Bb),0);
     sendcommand(Ha,sizeof(Ha),0);
     sendcommand(Hn,sizeof(Hn),0);
}
// commands
void M12MT::SetDateTimePPSAlignment()
{
    // @@St Set Date/Time/PPS Alignment
    // This command sets or queries the time & 1PPS alignment source.
    // Query
    // 0x40 0x40 0x53 0x74 0x00 0x27 0x0D 0x0A
    // Set to UTC(USNO) with leap seconds
    // 0x40 0x40 0x53 0x74 0x02 0x25 0x0D 0x0A
     static char St[]={0x40, 0x40, 0x53, 0x74, 0x02, 0x25, 0x0D, 0x0A};
     sendcommand(St,sizeof(St),8);
}

void M12MT::PPSOneSat()
{
//    1PPS CONTROL MESSAGE
//    @@Gc
//    1PPS Disabled
//    0x40 0x40 0x47 0x63 0x00 0x24 0x0D 0x0A
//    1PPS Always
//    0x40 0x40 0x47 0x63 0x01 0x25 0x0D 0x0A
//    1PPS Active at least one satellite
//    0x40 0x40 0x47 0x63 0x02 0x26 0x0D 0x0A
//    1PPS Active when T-RAIM conditions are met
//    0x40 0x40 0x47 0x63 0x03 0x27 0x0D 0x0A

    static char Gc[]={0x40, 0x40, 0x47, 0x63, 0x02, 0x26, 0x0D, 0x0A};
    sendcommand(Gc,sizeof(Gc),8);
}

void M12MT::SendPosition(QString fileName)
{
    // open the position file and send it to the receiver'
    QFile file(fileName);
    //
    if (!file.open(QFile::ReadOnly)){
        qDebug() << "File not open" << endl;
    }
    else{
        QByteArray Position = file.readAll();
        qDebug() << "Position" << Position << endl;
        qDebug() << "Position size" << Position.size() << endl;
        file.close();
        //
        quint8 checksum = 0x32;          // calculate checksum
        GPS->putChar('@');
        GPS->putChar('@');
        GPS->putChar(0x41);
        GPS->putChar(0x73);
        //
        serialData.clear();   // clear received data buffer
        response_lenght=20;   // expected response length
        for (quint16 i=0; i < Position.size(); i++){
            GPS->putChar(Position.at(i) & 0x00FF);
            checksum = checksum ^ Position.at(i);
        }
        //
        GPS->putChar(0x00);
        GPS->putChar(checksum);
        GPS->putChar(0x0D);
        GPS->putChar(0x0A);
        }
}

void M12MT::SendPositionHold()
{
    //  @@Gd  Set Position Hold
    static char Gd[]={0x40, 0x40, 0x47, 0x64, 0x01, 0x22, 0x0D, 0x0A};
    sendcommand(Gd,sizeof(Gd),8);
}

void M12MT::EnableTRAIM()
{
    //    TIME RAIM SELECT MESSAGE
    //    @@Ge
    //    Query
    //    0x40 0x40 0x47 0x65 0xFF 0xDD 0x0D 0x0A
    //    Disable TRAIM
    //    0x40 0x40 0x47 0x65 0x00 0x22 0x0D 0x0A
    //    Enable TRAIM
    //    0x40 0x40 0x47 0x65 0x01 0x23 0x0D 0x0A
    static char Ge[]={0x40, 0x40, 0x47, 0x65, 0x01, 0x23, 0x0D, 0x0A};
    sendcommand(Ge,sizeof(Ge),8);
}

void M12MT::surveyPosition()
{
    static  char Gd[]={0x40, 0x40, 0x47, 0x64, 0x03, 0x20, 0x0D, 0x0A};   //@@Gd 0x03 autosurvey
    sendcommand(Gd,sizeof(Gd),8);
}

bool M12MT::savePosition(QString filename)
{
    //Position file creation
    QFile file(filename);

    if (!file.open(QFile::WriteOnly)){
        qDebug() << "File not open" <<endl;
        return false;
    }
    else
    {
        file.write(rawpos);     // correct
        file.flush();
        file.close();
        qDebug() << "File Saved" <<endl;
        return true;
    }

}

void M12MT::SetToDefaults()
{
    //@@Cf Set to Defaults
    static char Cf[]={0x40, 0x40, 0x43, 0x66, 0x25, 0x0D, 0x0A};
    sendcommand(Cf,sizeof(Cf),7);
}

void M12MT::sendcommand(char *gpscommand, quint16 size, quint16 response)
{

   serialData.clear();         // clear received data buffer
   response_lenght=response;   // expected response length

    for (quint16 i=0; i < size; i++)
    {
        GPS->putChar(gpscommand[i]);

    }

}
