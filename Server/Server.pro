#-------------------------------------------------
#
# Project created by QtCreator 2014-08-26T23:25:10
#
#-------------------------------------------------

QT       += core gui multimedia network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Server
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    audioinput.cpp \
    server.cpp

HEADERS  += mainwindow.h \
    audioinput.h \
    server.h

FORMS    += mainwindow.ui
