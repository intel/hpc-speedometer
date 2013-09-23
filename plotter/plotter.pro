#-------------------------------------------------
#
# Project created by QtCreator 2013-07-24T08:12:01
#
#-------------------------------------------------

include( $${PWD}/qwt.pri )
QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = plotter
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
        data.cpp

HEADERS  += mainwindow.h data.h

FORMS    += mainwindow.ui

