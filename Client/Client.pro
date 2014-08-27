#-------------------------------------------------
#
# Project created by QtCreator 2014-08-26T23:31:59
#
#-------------------------------------------------

QT       += core gui multimedia network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Client
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    audiooutput.cpp \
    client.cpp

HEADERS  += mainwindow.h \
    audiooutput.h \
    client.h

FORMS    += mainwindow.ui
