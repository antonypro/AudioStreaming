QT +=       core gui widgets multimedia concurrent

android:QT += androidextras

TARGET =    BroadcastClient
TEMPLATE =  app


SOURCES +=  main.cpp \
            mainwindow.cpp \
            levelwidget.cpp \
            barswidget.cpp \
            spectrumanalyzer.cpp \
            waveformwidget.cpp \
            audiorecorder.cpp

HEADERS  += mainwindow.h \
            levelwidget.h \
            barswidget.h \
            spectrumanalyzer.h \
            waveformwidget.h \
            audiorecorder.h

android {
SOURCES += \
        keepandroidawake.cpp
HEADERS += \
        keepandroidawake.h
}

INCLUDEPATH += $$PWD/3rdparty/kiss_fft130
SOURCES += $$PWD/3rdparty/kiss_fft130/kiss_fft.c

include(../../AudioStreamingLib.pri)
