QT +=       core gui widgets

TARGET =    WalkieTalkie
TEMPLATE =  app


SOURCES +=  main.cpp \
            mainwindow.cpp \
            levelwidget.cpp \
            audiorecorder.cpp

HEADERS +=  mainwindow.h \
            levelwidget.h \
            audiorecorder.h

include(../../AudioStreamingLib.pri)
