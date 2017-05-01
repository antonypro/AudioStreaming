QT +=       core gui widgets multimedia concurrent

android:QT += androidextras

TARGET =    BroadcastServer
TEMPLATE =  app


SOURCES +=  main.cpp \
            mainwindow.cpp \
            levelwidget.cpp \
            bars.cpp \
            spectrumanalyzer.cpp

HEADERS +=  mainwindow.h \
            levelwidget.h \
            bars.h \
            spectrumanalyzer.h

win32 {
SOURCES += qwinloopback.cpp
HEADERS += qwinloopback.h
LIBS += -lOle32 -lwinmm -lksuser -luuid
}

android {
SOURCES += \
        keepandroidawake.cpp
HEADERS += \
        keepandroidawake.h
}

INCLUDEPATH += $$PWD/3rdparty/kiss_fft130
SOURCES += $$PWD/3rdparty/kiss_fft130/kiss_fft.c

include(../../AudioStreamingLib.pri)
