#-------------------------------------------------
#
# Project created by QtCreator 2017-09-01T11:06:20
#
#-------------------------------------------------

QT       += core gui serialport network charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TGRC
TEMPLATE = app


SOURCES += main.cpp\
    ticc.cpp \
    m12mt.cpp \
    logger.cpp \
    dialog.cpp \
    qroundprogressbar.cpp \
    skyplotwidget.cpp


HEADERS  += dialog.h \
    ticc.h \
    m12mt.h \
    logger.h \
    skyplotwidget.h \
    qroundprogressbar.h

FORMS    += dialog.ui

RESOURCES += \
    resources.qrc

release: DESTDIR = build/release
debug:   DESTDIR = build/debug

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui
