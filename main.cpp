#include <QApplication>
#include "dialog.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Dialog w;
    QString title="Time Guardian RC";
    /*https://wiki.qt.io/Get_OS_name*/
  #ifdef Q_OS_WIN

  //  title.append(" Fecha de Compilación:  ");
  // title.append(QString("%1 %2").arg(__DATE__).arg(__TIME__));
      w.setFixedSize(800,480);
      w.setModal(true);
      w.setWindowTitle(title);
  #endif
//
  #ifdef Q_OS_LINUX
      w.setFixedSize(800,480);
      w.setModal(true);
      w.setWindowFlags(Qt::FramelessWindowHint);
      w.setWindowTitle(title);
  #endif

    w.show();
    return a.exec();
}
