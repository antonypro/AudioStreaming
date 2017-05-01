QT +=       core gui widgets multimedia

android:QT += androidextras

TARGET =    BroadcastClient
TEMPLATE =  app


SOURCES +=  main.cpp \
            mainwindow.cpp \
            levelwidget.cpp \
            bars.cpp \
            spectrumanalyzer.cpp

HEADERS  += mainwindow.h \
            levelwidget.h \
            bars.h \
            spectrumanalyzer.h

android {
SOURCES += \
        keepandroidawake.cpp
HEADERS += \
        keepandroidawake.h
}

INCLUDEPATH += $$PWD/3rdparty/kiss_fft130
SOURCES += $$PWD/3rdparty/kiss_fft130/kiss_fft.c

include(../../AudioStreamingLib.pri)
