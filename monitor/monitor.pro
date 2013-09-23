#-------------------------------------------------
#
# Project created by QtCreator 2013-08-01T22:16:46
#
#-------------------------------------------------

include( $${PWD}/qwt.pri )

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = monitor
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
        data.cpp \
        speedo_meter.cpp

HEADERS  += mainwindow.h data.h speedo_meter.h

FORMS    += mainwindow.ui
