#CONFIG += WITH_OPUS

WITH_OPUS {
DEFINES += OPUS

macx { #MAC
INCLUDEPATH += /usr/local/include/opus
LIBS += /usr/local/lib/libopus.a
}

unix:!macx { #LINUX
INCLUDEPATH += /usr/local/include/opus
LIBS += -lopus
}

win32-g++ { #MINGW
INCLUDEPATH += ..path_to../opus/include
LIBS += ..path_to../opus/.libs/libopus.a
}

win32-msvc* { #VISUAL STUDIO
INCLUDEPATH += ..path_to../opus/include

Debug {
LIBS += ..path_to../Debug/opus.lib
LIBS += ..path_to../Debug/celt.lib
LIBS += ..path_to../Debug/silk_common.lib
LIBS += ..path_to../Debug/silk_float.lib
}

Release {
LIBS += ..path_to../Release/opus.lib
LIBS += ..path_to../Release/celt.lib
LIBS += ..path_to../Release/silk_common.lib
LIBS += ..path_to../Release/silk_float.lib
}
}
}
