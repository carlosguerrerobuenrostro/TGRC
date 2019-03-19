#include "dialog.h"
#include "ui_dialog.h"
#include <QMessageBox>
#include <QtCore>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTextStream>
#include <QFontDatabase>
#include <QNetworkInterface>
#include <QDebug>
//
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLegend>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QStackedBarSeries>
//
//
Logger *Dialog::logger;
//
QT_CHARTS_USE_NAMESPACE


Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
    {
    ui->setupUi(this);
    //
    createDirectories();        // check if config and data directories exist, if not, create
    //    // GPS Skyplot
    //skyplot = new SkyplotWidget();
    configureTextandColors();   // configure colors, type of font, and icons
    //
    // start log
    // QPlainTextEdit *editor = new QPlainTextEdit(this);
    QString fileName = config_path + "/log.txt";
    logger = new Logger(this, fileName, ui->txtDebug);
    logger->write("Starting Time Guardian...");
    //
    listNetworkInterfaces();
    //
    listSerialPorts();
    //
    readSettings();
    //
    // inicializa el tic desde la clase TICC
    TICC* tic=new TICC();
    tic->startTIC();
    //
    // GPS
    //M12MT* gps=new M12MT();
    GPS_AVA=gps->startGPS();
    connect(gps,SIGNAL(gpsReady()),this,SLOT(update_Data()));
    //
   if(GPS_AVA){
       connect(tic,SIGNAL(countReady()),this,SLOT(askGPS()));
   }


    // inicializo proceso
    process=new QProcess(this);

}
//
Dialog::~Dialog()
{
    delete ui;  // delete main dialog
}
//
void Dialog::update_Data()
{
    // CLCD
     c_count=(tic->count) + (gps->last_negative_sawtooth) + int_dly + cbl_dly + ref_dly;
     int channel=0;
     int sumoffset=0;
    //
     for(int i=1 ; i <=32 ; i++){
         if (gps->SVID[i].mode ==8){
            //  double snr=20*(qLn(gps->SVID[i].signal)/qLn(10))-163.4;
             sumoffset += gps->SVID[i].fractional_GPS_time;
             // GUI
             ui->skyplot->insert(static_cast<quint32>(i),gps->SVID[i].azimuth,gps->SVID[i].elevation,
                             QString::number(i).rightJustified(2, '0'),Qt::black,colorWheel(channel),Qt::black,
                             SkyplotWidget::SatelliteState::Visible /*| SkyplotWidget::SatelliteState::Flashing*/);
            //
             QGradientStops gradientPoints;
             gradientPoints << QGradientStop(0, colorWheel(channel));
             channels[channel]->setDataColors(gradientPoints);
             channels[channel]->setValue(static_cast<int>(gps->SVID[i].fractional_GPS_time));
            //
             channel +=1;
            }
         else{
             ui->skyplot->remove(static_cast<quint32>(i));  // quita los satelites que ya no son rastreados
         }
    }
    meanoffset = static_cast<double>(sumoffset)/channel;
    channel=0;
    satdiff=0.0;
    for(int i=1 ; i <=32 ; i++){
           if (gps->SVID[i].mode ==8){   //if satellite is in time solution
            satdiff = static_cast<double> (meanoffset - gps->SVID[i].fractional_GPS_time);
            channels[channel]->setFormat(QString::number(satdiff,'f',2));
           channel +=1;
           }
         gps->SVID[i].mode=0;
       }
    for(int i=channel; i < 12; i++){
      channels[i]->setValue(0);
      channels[i]->setFormat("");
    }
    updateGUI();
    SaveData();
}

void Dialog::updateGUI()
{
    //
    // Date
    ui->lblTime->setText(gps->GPStime.toString(Qt::ISODate));
    ui->lblDate->setText(gps->GPSdate.toString(Qt::ISODate) + "  " + QString::number(JulianDate(gps->GPSdate)) );
    // Position
    ui->lblPosition->setText(QString::number(gps->latitud, 'g', 10) + "," + QString::number(gps->longitud, 'g', 10)  + "," + QString::number(gps->altitud, 'g', 10) );
    // Status
    ui->lblStatus->setText(gps->status);
    //
    ui->lblSVTS->setText(QString::number(gps->nvs,10) + "/" + QString::number(gps->nts,10) + "  " + QString::number(gps->negative_sawtooth,10));
    // LCD
    ui->lblLCD->setText(QString::number(tic->count,'f',3) + " ns");
    // CLCD
    ui->lblCLCD->setText(QString::number(c_count,'f',3) + " ns");
}
//
void Dialog::SaveData()
{   //
    seconds=seconds+1;
    count_one+=c_count;
    curr_minute=gps->GPStime.minute();
    //
    if (curr_minute != prev_minute) // diferente minute
    {
        //  this flag is false unless a new file is started later in the routine
        QString oldname=namefile;
        QString extension="." + EXT;; // filename extension
        //
        //  define file name
        namefile= QString::number(JulianDate(gps->GPSdate))  + extension;
        //
        QString filename= data_path + "/" + namefile;
        QFile file(filename);
        //
        if ((namefile != oldname)|| (!QFile(filename).exists()))
        {
          //  QString filename= data_path + "/" + namefile;
          //  QFile file(filename);
        //  if the file is not found, use the defaults defined here
            if (!QFile(filename).exists()) {
                if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
                      qDebug() << "no data file" <<endl;
                  }
                 else{
                    LLAtoECEF(gps->latitud,gps->longitud,gps->altitud);
                    QTextStream outStream(&file);
                    outStream.setCodec("UTF-8");
                    //default settings
                   // outStream << "#TIME,DIFF" << endl;            //
                    //default settings
                    outStream << "#LAB = " << LAB << endl;                 // Device number
                    outStream << "#ECEF" << endl;
                    outStream << "#X = " << QString::number(ECEF_X,'f',2) << " m (GPS)" << endl;            // X
                    outStream << "#Y = " << QString::number(ECEF_Y,'f',2) << " m (GPS)" << endl;            // Y
                    outStream << "#Z = " << QString::number(ECEF_Z,'f',2) << " m (GPS)" << endl;            // Z
                    outStream << "#WGS84" << endl;            //
                    outStream << "#LAT = " << QString::number(gps->latitud, 'g', 10) << " deg. (GPS)" << endl;            // X
                    outStream << "#LON = " << QString::number(gps->longitud, 'g', 10) << " deg. (GPS)" << endl;            // Y
                    outStream << "#ALT = " << QString::number(gps->altitud,'g',10) << " m (GPS)" << endl;            // Z
                    outStream << "#COMMENTS = " << COM << endl;            //
                    outStream << "#INT DLY = " << QString::number(int_dly,'f',2) << " ns" << endl;
                    outStream << "#CAB DLY = " << QString::number(cbl_dly,'f',2) << " ns" << endl;
                    outStream << "#REF DLY = " << QString::number(ref_dly,'f',2) << " ns" << endl;
                    outStream << "#REF = " << REF << endl;            //
                    outStream << "#" << endl;            //
                    outStream << "MINUTE,TIME,DIFF,SECONDS" << endl;            //
                    file.flush();
                    file.close();
                 }
            }
        }

                count_one=count_one/seconds;
                minutes=(gps->GPStime.hour()*60+gps->GPStime.minute());

    //  Guardado por minuto
    //
    file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    QTextStream outStream(&file);
     outStream.setCodec("UTF-8");
     outStream << QString::number(minutes)
                << ","
                << gps->GPStime.toString("hhmm") + "30"
                << ","
                << QString::number(count_one,'f',2)
                << ","
                << QString::number(seconds)
                << endl;
    file.flush();
    file.close();
    //
    // Limpio las variables
          count_one=0;
          seconds=0;
          prev_minute=curr_minute;
    }
}
//
/*
* utilitary functiones
*/
void Dialog::createDirectories()
{   //
    // Directory Creation
    path = QCoreApplication::applicationDirPath(); //working directory to path variable
    qDebug() << path << endl;
    config_path=path + "/config";
    data_path=path + "/data" ;
    //
    QDir  config(config_path);
    //
    if (!config.exists()){
        config.mkdir(config_path);
    }
    //
    QDir  data(data_path);
    //
    if (!data.exists()){
        data.mkdir(data_path);
    }
}
//
void Dialog::configureTextandColors()
{   //
    /*Set image Background*/
    QPixmap     bkgnd(":/img/SSTTF");       // set background image, now just a black image
    QPalette    palette;
    palette.setBrush(QPalette::Background, bkgnd);
    this->setPalette(palette);
    //
    /*Set Label background and foreground colors*/
    QPalette sample_palette;
    sample_palette.setColor(QPalette::Window, Qt::black);
    sample_palette.setColor(QPalette::Base, Qt::black);
    sample_palette.setColor(QPalette::WindowText, Qt::green);
    sample_palette.setColor(QPalette::Text,Qt::green);
    //
    ui->lblTime->setAutoFillBackground(false);
    ui->lblTime->setPalette(sample_palette);
    //
    ui->lblDate->setAutoFillBackground(false);
    ui->lblDate->setPalette(sample_palette);
    //
    ui->lblPosition->setAutoFillBackground(false);
    ui->lblPosition->setPalette(sample_palette);
    //
    ui->lblSVTS->setAutoFillBackground(false);
    ui->lblSVTS->setPalette(sample_palette);
    //
    ui->lblStatus->setAutoFillBackground(false);
    ui->lblStatus->setPalette(sample_palette);
    //
    ui->lblLCD->setAutoFillBackground(false);
    ui->lblLCD->setPalette(sample_palette);
    //
    ui->lblCLCD->setAutoFillBackground(false);
    ui->lblCLCD->setPalette(sample_palette);
    //
    ui->txtDebug->setAutoFillBackground(false);
    ui->txtDebug->setPalette(sample_palette);
    //
    /*Set Font type*/
    QFontDatabase::addApplicationFont(":/fnt/ATOMICCLOCKRADIO.TTF");
    QFont font = QFont("Atomic Clock Radio", 70, 1);
    ui->lblTime->setFont(font);
    font = QFont("Atomic Clock Radio", 30, 1);
    ui->lblDate->setFont(font);
    font = QFont("Atomic Clock Radio", 20, 1);
    ui->lblPosition->setFont(font);
    ui->lblStatus->setFont(font);
    ui->lblSVTS->setFont(font);
    font = QFont("Atomic Clock Radio", 30, 1);
    ui->lblLCD->setFont(font);
    ui->lblCLCD->setFont(font);
    //
    //font = QFont("Monospace", 10, 1);
    //ui->txtDebug->setFont(font);
    /* Make the button "invisible" */
    //
    QPixmap pixmap(":/img/FN.png");
    QIcon ButtonIcon(pixmap);
    ui->btnFuegoNuevo->setIcon(ButtonIcon);
    ui->btnFuegoNuevo->setIconSize(pixmap.rect().size());
    ui->btnFuegoNuevo->setFixedSize(pixmap.rect().size());
    //
    ui->btnFuegoNuevo_2->setIcon(ButtonIcon);
    ui->btnFuegoNuevo_2->setIconSize(pixmap.rect().size());
    ui->btnFuegoNuevo_2->setFixedSize(pixmap.rect().size());
    //
    ui->btnFuegoNuevo_3->setIcon(ButtonIcon);
    ui->btnFuegoNuevo_3->setIconSize(pixmap.rect().size());
    ui->btnFuegoNuevo_3->setFixedSize(pixmap.rect().size());
    //
    //
    // skyplot graph
    ui->skyplot->setGridTextColor(Qt::green);
    //skyplot->setFontColor(Qt::green);
    ui->skyplot->setFontScale(0.035);
    ui->skyplot->setGridColor(Qt::green);
    ui->skyplot->setGridWidth(1.5);
    ui->skyplot->setCrosses(2);
    ui->skyplot->setEllipses(6);
    ui->skyplot->setSatelliteScale(0.05);
    //
    // Phase and delay plots
    channels[0] = ui->ch01;
    channels[1] = ui->ch02;
    channels[2] = ui->ch03;
    channels[3] = ui->ch04;
    channels[4] = ui->ch05;
    channels[5] = ui->ch06;
    channels[6] = ui->ch07;
    channels[7] = ui->ch08;
    channels[8] = ui->ch09;
    channels[9] = ui->ch10;
    channels[10] = ui->ch11;
    channels[11] = ui->ch12;


    QGradientStops gradientPoints;
    gradientPoints << QGradientStop(0, Qt::green);// << QGradientStop(0.5, Qt::yellow) << QGradientStop(1, Qt::red);
    // and set it
    QPalette p1;
    p1.setBrush(QPalette::AlternateBase, Qt::black);
    p1.setColor(QPalette::Text, Qt::yellow);
    //
    QPalette p2(p1);
    p2.setBrush(QPalette::Base, Qt::black);
    p2.setColor(QPalette::Text, Qt::green);
    p2.setColor(QPalette::Shadow, Qt::NoBrush);
    //
    for (int i=0; i<12; i++){
        channels[i]->setPalette(p2);
        //  channels[i]->setDataColors(gradientPoints);
        //  channels[i]->setNullPosition(QRoundProgressBar::PositionRight);
        //  channel[i]->setBarStyle(QRoundProgressBar::StyleLine);
        //
        channels[i]->setMaximum(1000000);
        channels[i]->setMinimum(0);
        channels[i]->setFormat("");
    }
}
//
quint32 Dialog::JulianDate(QDate MJD)
{
    // This routine calculate the Modified Julian Data
    // input:    QDate
    // output:   Quint32 MJD
    return static_cast<quint32>((MJD.toJulianDay()-2400001));
}
//
/*Seccion de configuracion de parametros*/
void Dialog::writeSettings(QString iniFileName)
{   //
   // QString iniFileName=config_path + "/config.ini";
    // write Application params to ini file
    QSettings settings( iniFileName, QSettings::IniFormat );
    //
    settings.beginGroup("PORTS");
    settings.setValue("GPS", "DEMO");
    settings.setValue("TIC", "DEMO");
    settings.endGroup();
    //
    settings.beginGroup("DELAYS");
    settings.setValue("INT" , 0.0);
    settings.setValue("CAB" , 0.0);
    settings.setValue("REF" , 0.0);
    settings.endGroup();
    //
    settings.beginGroup("CONTACT");
    settings.setValue("LAB", "DEMO");
    settings.setValue("COM", "DEMO");
    settings.setValue("REF" ,"DEMO");
    settings.endGroup();
    //
    settings.beginGroup("EXTRA");
    settings.setValue("EXT", "csv");
    settings.setValue("IP" , "localhost");
    settings.endGroup();
    logger->write("Writting Settings");
}
//
void Dialog::readSettings()
{   //
    QString iniFileName=config_path + "/config.ini";
    //
    if (!QFileInfo(iniFileName).exists()){      //The file doesn´t exist
        writeSettings(iniFileName);
    }
    //
    // Load Application params from ini file
    QSettings settings( iniFileName, QSettings::IniFormat );
    //
    settings.beginGroup("PORTS");
    GPS_port_name = settings.value("GPS").toString();
    if (GPS_port_name != ""){
        GPS_is_available=true;
    }
    TIC_port_name = settings.value("TIC").toString();
    if (TIC_port_name != ""){
        TIC_is_available=true;
    }
    settings.endGroup();
    //
    settings.beginGroup("DELAYS");
        int_dly = settings.value("INT").toDouble();
        cbl_dly = settings.value("CAB").toDouble();
        ref_dly = settings.value("REF").toDouble();
//          qDebug() << "INT_DLY = " << QString::number(int_dly,'f',2)
//                   << "CBL_DLY = " << QString::number(cbl_dly,'f',2)
//                   << "REF_DLY = " << QString::number(ref_dly,'f',2)
//                   << endl;
    settings.endGroup();
    //
    settings.beginGroup("CONTACT");
        LAB = settings.value("LAB").toString();
        COM = settings.value("COM").toString();
        REF = settings.value("REF").toString();
    settings.endGroup();
    //
    settings.beginGroup("EXTRA");
        EXT = settings.value("EXT").toString();
        IP  = settings.value("IP").toString();
    settings.endGroup();
    //
}

void Dialog::listNetworkInterfaces()
{
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    logger->write("Number of Network Interfaces " + QString::number(interfaces.count()-1,10));
    for(int i=0; i<interfaces.count(); i++){
         QList<QNetworkAddressEntry> entries = interfaces.at(i).addressEntries();
             for(int j=0; j<entries.count(); j++){
                 if(entries.at(j).ip().protocol() == QAbstractSocket::IPv4Protocol){
                    if (entries.at(j).ip().toString() != "127.0.0.1"){
                      // qDebug() <<"Interface [" + QString::number(i+1) + "]";
                        logger->write("Interface [" + QString::number(i+1) +"]");
                        logger->write("IP:" + entries.at(j).ip().toString());
                        logger->write("MASK:" + entries.at(j).netmask().toString());
                        logger->write("GATEWAY:" + entries.at(j).broadcast().toString());
                     }
                  }
             }
    }
}

void Dialog::listSerialPorts()
{
    //  Testing code, prints the description, vendor id, and product id of all ports.
    logger->write("Number of Ports: " + QString::number(QSerialPortInfo::availablePorts().length(),10));
    foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()){
        logger->write("Description: " + serialPortInfo.description());
        logger->write("VID: 0x" + QString::number(serialPortInfo.vendorIdentifier(),16).rightJustified(4, '0'));
        logger->write("PID: 0x" + QString::number(serialPortInfo.productIdentifier(),16).rightJustified(4, '0'));
    }
}

QColor Dialog::colorCodeGPS(int svid)
{
    QColor color;
    switch( svid )
    {
    case 1: color=QColor(0xbc,0xbb,0x22); break ;
    case 2: color=QColor(0x1d,0xbf,0xd0); break ;
    case 3: color=QColor(0x1f,0x77,0xb4); break ;
   // case 4: color=QColor(0xbc,0xbb,0x22); break ;
    case 5: color=QColor(0x94,0x67,0xbd); break ;
    case 6: color=QColor(0xe3,0x7a,0xc3); break ;
    case 7: color=QColor(0xff,0x7f,0x0e); break ;
    case 8: color=QColor(0x2c,0xc0,0x2c); break ;
    case 9: color=QColor(0xd6,0x27,0x28); break ;
    case 10: color=QColor(0xd6,0x27,0x28); break ;  //igual al anterior
        //
    case 11: color=QColor(0x94,0x67,0xbd); break ;
    case 12: color=QColor(0xd6,0x27,0x28); break ;
    case 13: color=QColor(0x96,0x6b,0xbe); break ;
    case 14: color=QColor(0x8c,0x56,0x4b); break ;
    case 15: color=QColor(0x17,0xbe,0xcf); break ;
    case 16: color=QColor(0x8c,0x56,0x4b); break ;
    case 17: color=QColor(0x17,0xbe,0xcf); break ;
    case 18: color=QColor(0x82,0x82,0x82); break ;
    case 19: color=QColor(0xd7,0x2c,0x2d); break ;
    case 20: color=QColor(0x2c,0xc0,0x2c); break ;
//
    case 21 : color=QColor(0xff,0x7f,0x0e); break ;
    case 22 : color=QColor(0x17,0xbe,0xcf); break ;
    case 23 : color=QColor(0xe3,0x77,0xc2); break ;
    case 24 : color=QColor(0x1f,0x77,0xb4); break ;
    case 25 : color=QColor(0x8c,0x56,0x4b); break ;
    case 26 : color=QColor(0xbc,0xbd,0x22); break ;
    case 27 : color=QColor(0x7f,0x7f,0x7f); break ;
    case 28 : color=QColor(0xbc,0xbd,0x22); break ;
    case 29 : color=QColor(0x7f,0x7f,0x7f); break ;
    case 30 : color=QColor(0xe3,0x77,0xc2); break ;
    case 31 : color=QColor(0x7f,0x7f,0x7f); break ;
    case 32 : color=QColor(0x94,0x67,0xbd); break ;
    }

    return color;
}

QColor Dialog::colorWheel(int index)
{
    QColor color;
    //
    switch( index +1){
    case 1: color=QColor(255,0,0); break ;      //  Red
    case 2: color=QColor(255,127,0); break ;    //  Orange
    case 3: color=QColor(255,255,0); break ;    //  Yellow
    case 4: color=QColor(127,255,0); break ;    //  Green Yellow
    case 5: color=QColor(0,255,0); break ;      //  Green
    case 6: color=QColor(0,255,127); break ;    //  Green Cyan
    case 7: color=QColor(0,255,255); break ;    //  Cyan
    case 8: color=QColor(0,127,255); break ;    //  Blue Cyan
    case 9: color=QColor(0,0,255); break ;      //  Blue
    case 10: color=QColor(127,0,255); break ;   //  Blue Magenta
    case 11: color=QColor(255,0,255); break ;   //  Magenta
    case 12: color=QColor(255,0,127); break ;   //  Red Magenta
    }
    //
    return color;
}
//
void Dialog::LLAtoECEF(double lat, double lon, double h)
{
    //    The basic formulas for converting from latitude, longitude, altitude
    //    (above reference ellipsoid) to Cartesian ECEF are given in the
    //    Astronomical Almanac in Appendix K.  They depend upon the following
    //    quantities:
    //
    //         a   the equatorial earth radius
    //         f   the "flattening" parameter ( = (a-b)/a ,the ratio of the
    //             difference between the equatorial and polar radio to a;
    //             this is a measure of how "elliptical" a polar cross-section
    //             is).
    //
    //    The eccentricity e of the figure of the earth is found from
    //
    //        e^2 = 2f - f^2 ,  or  e = sqrt(2f-f^2) .
    //
    //    For WGS84,
    //
    //             a   = 6378137 meters
    //           (1/f) = 298.257224
    //
    //    (the reciprocal of f is usually given instead of f itself, because the
    //    reciprocal is so close to an integer)
    //
    //    Given latitude (geodetic latitude, not geocentric latitude!), compute
    //
    //                                      1
    //            C =  ---------------------------------------------------
    //                 sqrt( cos^2(latitude) + (1-f)^2 * sin^2(latitude) )
    //
    //    and
    //            S = (1-f)^2 * C .
    //
    //    Then a point with (geodetic) latitude "lat," longitude "lon," and
    //    altitude h above the reference ellipsoid has ECEF coordinates
    //
    //           x = (aC+h)cos(lat)cos(lon)
    //           y = (aC+h)cos(lat)sin(lon)
    //           z = (aS+h)sin(lat)
    //
    double cosLat = qCos(lat * M_PI / 180.0);
    double cosLon = qCos(lon * M_PI / 180.0);
    double sinLat = qSin(lat * M_PI / 180.0);
    double sinLon = qSin(lon * M_PI / 180.0);
    //
    double a = 6378137;
    double f= 1.0/298.257224;
    double C=1.0/(qSqrt((cosLat*cosLat)+((1.0-f)*(1.0-f)*sinLat*sinLat)));
    double S=(1.0-f)*(1.0-f)*C;
    //
    ECEF_X = (a * C + h) * cosLat * cosLon;
    ECEF_Y = (a * C + h) * cosLat * sinLon;
    ECEF_Z = (a * S + h) * sinLat;
    //
    qDebug() << "lat" << QString::number(lat,'f',6) << "°" <<
                "lon" << QString::number(lon,'f',6) << "°" <<
                "alt" << QString::number(h,'f',2) << "m" <<
                endl;
    //
    qDebug() << "x" << QString::number(ECEF_X,'f',2) << "m" <<
                "y" << QString::number(ECEF_Y,'f',2) << "m" <<
                "z" << QString::number(ECEF_Z,'f',2) << "m" <<
                endl;
    //    http://www.oc.nps.edu/oc2902w/coord/llhxyz.htm
}
// Buttons
void Dialog::on_btnFuegoNuevo_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}
//
void Dialog::on_btnFuegoNuevo_2_clicked()
{
      ui->stackedWidget->setCurrentIndex(2);
}
//
void Dialog::on_btnFuegoNuevo_3_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void Dialog::on_pbSavePosition_clicked()
{
    bool saved;
    saved=gps->savePosition(config_path + "/" +"position.bin");
    if (saved){
        logger->write("File Saved at " + config_path + "/" +"position.bin");
    }
        else{
         logger->write("ERROR File NOT Saved");
    }
}

void Dialog::on_pbSetDefaults_clicked()
{
    gps->SetToDefaults();
}

void Dialog::on_pbSendPosition_clicked()
{
    gps->SendPosition(config_path + "/" +"position.bin");
}

void Dialog::on_pbSurvey_clicked()
{
    gps->surveyPosition();
}

void Dialog::askGPS()
{
    gps->SendTripleCMD();
}

void Dialog::on_pbShutdown_clicked()
{
    QString a;
#ifdef Q_OS_WIN
    a.append("shutdown -s -t 60");
#endif
//
#ifdef Q_OS_LINUX
    a.append("halt");
#endif
    process->startDetached(a);
    a.detach();
    logger->write("Shutting Down System");
}

void Dialog::on_pbReboot_clicked()
{
    QString a;
#ifdef Q_OS_WIN
    a.append("shutdown -r -t 60");
#endif
//
#ifdef Q_OS_LINUX
    a.append("reboot");
#endif
    process->startDetached(a);
    a.detach();
    logger->write("Rebooting System");
}
