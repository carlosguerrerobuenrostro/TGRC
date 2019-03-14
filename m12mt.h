#ifndef M12MT_H
#define M12MT_H

#include <QObject>
#include <QtCore>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include <QTimer>


class M12MT : public QObject
{
    Q_OBJECT
public:
    explicit M12MT(QObject *parent = nullptr);
    // public variables
    static QTime GPStime;
    static QDate GPSdate;
    static qint8 nvs;
    static qint8 nts;
    static qint8 negative_sawtooth;
    static qint8 last_negative_sawtooth;
    static double latitud;
    static double longitud;
    static double altitud;
    static QString status;
    static bool DEMOmode;

    //Satellites Structure
    struct {
          //Ha
          quint8     mode=0;
          quint8     signal=0;
          quint8     IODE=0;
          quint8     padding_0=0;
          quint16    status=0;
          //Hn
          quint16    padding_1=0;
          quint32    fractional_GPS_time=0;  //-5000...5000
          //Bb
          qint16     Doppler=0;   //-5000...5000
          quint8     elevation=0; //0...90
          quint8     padding_2=0;
          quint16    azimuth=0;   //0...359
          qint8      sat_health=0;//0 healthy and not removed; 1 unhealthy and removed
          qint8      padding_3=0;
      }SVID[33];




private:
    QSerialPort *GPS;
    // VID & PID STEREN RS232_USB
    static const quint16 GPS_vid = 1659;       // Constants for automatic identification
    static const quint16 GPS_pid = 8963;
    //
   // QByteArray HnData;
    QByteArray serialData;
    QString serialBuffer;
    QString parsed_data;
    //
    double data_value;
    //
    quint16 response_lenght=0;
    bool    response_ready=false;
    //flags
    bool CMD_Ha=false;
    bool CMD_Hn=false;
    bool CMD_Bb=false;
    //

    //Ha
    //Global Variables Declaration
   // qint8   last_negative_sawtooth;
    QByteArray rawpos;
    qint32  latitudAU;
    qint32  longitudAU;
    qint32  heightAU;
    QString ID;
    quint16 receiver_status=0;
    //Hn commands
    quint8  pulse_status;
    quint8  PPS_sync;
    quint8  TimeRAIM_solution;
    quint8  TimeRAIM_status;
    quint32 TimeRAIM_svid;
    quint16 time_solution; 

    //
signals:
    double gpsReady();
    //
public slots:
    void startGPS();
    void startGPS(QString PORTNAME);
    void readSerialGPS();
    void DemoMode();
    void SendTripleCMD();
    void sendcommand(char *gpscommand, quint16 size, quint16 response);
    //
    bool savePosition(QString filename);
    void SetToDefaults();

    //
    void Bb_Data(QByteArray Bb);
    void Ha_Data(QByteArray Ha);
    void Hn_Data(QByteArray Hn);
    void ReceiverStatus(quint16 rs);
    //
    void StartPolling();
    // void SendTripleCMD();
    //
    void SetDateTimePPSAlignment();
    void PPSOneSat();
    void SendPosition(QString fileName);
    void SendPositionHold();
    void EnableTRAIM();
    void surveyPosition();
private slots:
    //
};

#endif // M12MT_H
