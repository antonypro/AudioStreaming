QT +=       core gui widgets qml uitools

TARGET =    WalkieTalkie
TEMPLATE =  app


SOURCES +=  main.cpp \
            mainwindow.cpp \
            settings.cpp \
            levelwidget.cpp \
            audiorecorder.cpp \
            mp3recorder.cpp

HEADERS +=  mainwindow.h \
            common.h \
            settings.h \
            levelwidget.h \
            audiorecorder.h \
            mp3recorder.h

INCLUDEPATH += $$PWD/3rdparty/lame/include

include(../../AudioStreamingLib.pri)
