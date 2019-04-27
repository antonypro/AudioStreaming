QT +=       core gui widgets multimedia concurrent

TARGET =    BroadcastServer
TEMPLATE =  app


SOURCES +=  main.cpp \
            mainwindow.cpp \
            levelwidget.cpp \
            barswidget.cpp \
            spectrumanalyzer.cpp \
            waveformwidget.cpp \
            audiorecorder.cpp \
            mp3recorder.cpp

HEADERS +=  mainwindow.h \
            common.h \
            levelwidget.h \
            barswidget.h \
            spectrumanalyzer.h \
            waveformwidget.h \
            audiorecorder.h \
            mp3recorder.h

win32 {
SOURCES += qwinloopback.cpp
HEADERS += qwinloopback.h
LIBS += -lOle32 -lwinmm -lksuser -luuid
}

!android {
SOURCES += ffmpegclass.cpp
HEADERS += ffmpegclass.h
}

INCLUDEPATH += $$PWD/3rdparty/kiss_fft130
SOURCES += $$PWD/3rdparty/kiss_fft130/kiss_fft.c

INCLUDEPATH += $$PWD/3rdparty/lame/include

include(../../AudioStreamingLib.pri)
