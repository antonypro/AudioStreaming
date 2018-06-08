QT += core network sql
QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

SOURCES += main.cpp \
        openssllib.cpp \
        certificate.cpp \
        sql.cpp \
        sslserver.cpp \
        tcpserver.cpp

HEADERS += \
        common.h \
        openssllib.h \
        certificate.h \
        sql.h \
        sslserver.h \
        tcpserver.h

INCLUDEPATH += $$PWD/3rdparty
