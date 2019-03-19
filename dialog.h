#ifndef DIALOG_H
#define DIALOG_H
//
#include <QDialog>
#include <QtSerialPort/QSerialPort>
#include <QByteArray>
#include <QDate>
#include <QTime>
#include <QProcess>
//
#include "ticc.h"
#include "m12mt.h"
#include "logger.h"
#include "qroundprogressbar.h"
//
namespace Ui {
    class Dialog;
}
    //
class Dialog : public QDialog
{
    Q_OBJECT
    //
public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();
    //
    M12MT* gps=new M12MT();

static Logger *logger;
//static SkyplotWidget *skyplot;

private slots:
    //

    void update_Data();
    void updateGUI();
    void SaveData();
    // Buttons
    void on_btnFuegoNuevo_clicked();
    //
    void on_btnFuegoNuevo_2_clicked();
    //
    void on_btnFuegoNuevo_3_clicked();

    void on_pbSavePosition_clicked();

    void on_pbSetDefaults_clicked();

    void on_pbSendPosition_clicked();

    void on_pbSurvey_clicked();

    void askGPS();

    void on_pbShutdown_clicked();

    void on_pbReboot_clicked();

private:
    Ui::Dialog *ui;
    //
    //
    TICC* tic;
   // Logger *logger;
    //
    //  store time offset, used in time solution, elevation angle
    QString path="";
    QString config_path="";
    QString data_path="";
    //  TIC Variables
    QString debug="";
    QString namefile="";
    bool    debug_bool=false;
    //
    // utilitary functions
    quint32 JulianDate(QDate MJD);
    void    createDirectories();
    void    configureTextandColors();
    void    writeSettings(QString iniFileName);
    void    readSettings();
    void    listNetworkInterfaces();
    void    listSerialPorts();
    QColor  colorCodeGPS(int svid);
    QColor  colorWheel(int index);
    //ECEF
    double  ECEF_X;
    double  ECEF_Y;
    double  ECEF_Z;
    void    LLAtoECEF(double lat, double lon, double h);
    //
    // Config Settings Global Variables
    // config.ini
    // [PORTS]
    bool    GPS_is_available = false;
    QString GPS_port_name;
    bool    TIC_is_available = false;
    QString TIC_port_name;
    // [DELAYS]
    double  int_dly=0.0;
    double  cbl_dly=0.0;
    double  ref_dly=0.0;
    // [CONTACT]
    QString LAB;
    QString COM;
    QString REF;
    // [EXTRA]
    QString EXT;
    QString IP;
    //Table
    enum    GPS{
        PRN,TD,DT
       // PRN,dBm,AZTH,ELV
    };
    //
    double c_count=0.0;
    double  count_one=0.0;
    int  seconds=0;
    int minutes=0;
    double meanoffset=0.0;
    double satdiff=0.0;
    //
    QRoundProgressBar* channels[12];
    //
    int curr_minute;
    int prev_minute;
    //
    // Genera el proceso para hacer llamadas al sistema
      QProcess *process;
    //
     bool GPS_AVA;
};
//
#endif // DIALOG_H
