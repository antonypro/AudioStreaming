QT       += core gui widgets multimedia network

TARGET = Server
TEMPLATE = app

include(../common.pri)

SOURCES += main.cpp\
        mainwindow.cpp \
    audioinput.cpp \
    server.cpp

HEADERS  += mainwindow.h \
    audioinput.h \
    server.h

WITH_OPUS {

SOURCES += \
    opusencode.cpp \
    r8brain.cpp \
    r8brain/r8bbase.cpp

HEADERS  += \
    opusencode.h \
    r8brain.h \
    r8brain/CDSPBlockConvolver.h \
    r8brain/CDSPFIRFilter.h \
    r8brain/CDSPFracInterpolator.h \
    r8brain/CDSPProcessor.h \
    r8brain/CDSPRealFFT.h \
    r8brain/CDSPResampler.h \
    r8brain/CDSPSincFilterGen.h \
    r8brain/fft4g.h \
    r8brain/r8bbase.h \
    r8brain/r8bconf.h \
    r8brain/r8butil.h
}
