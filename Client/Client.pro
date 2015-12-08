QT       += core gui multimedia network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Client
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    audiooutput.cpp \
    client.cpp \
    audiorecorder.cpp

HEADERS  += mainwindow.h \
    audiooutput.h \
    client.h \
    audiorecorder.h

FORMS    += mainwindow.ui
