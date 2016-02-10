QT       += core gui widgets multimedia network

TARGET = Client
TEMPLATE = app

include(../common.pri)

SOURCES += main.cpp\
        mainwindow.cpp \
    audiooutput.cpp \
    client.cpp \
    audiorecorder.cpp \
    levelwidget.cpp

HEADERS  += mainwindow.h \
    audiooutput.h \
    client.h \
    audiorecorder.h \
    levelwidget.h

WITH_OPUS {

HEADERS  += \
    opusdecode.h

SOURCES += \
    opusdecode.cpp
}
