QT +=       core gui widgets

android:QT += androidextras

TARGET =    WalkieTalkie
TEMPLATE =  app


SOURCES +=  main.cpp \
            mainwindow.cpp \
            levelwidget.cpp \
            audiorecorder.cpp

HEADERS +=  mainwindow.h \
            levelwidget.h \
            audiorecorder.h

android {
SOURCES += \
        keepandroidawake.cpp
HEADERS += \
        keepandroidawake.h
}

include(../../AudioStreamingLib.pri)
