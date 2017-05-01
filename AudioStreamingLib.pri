include (AudioStreamingLibSettings.pri)

QT += core network multimedia
CONFIG += c++11

if (contains(OPUS_ENABLED, TRUE)) CONFIG += WITH_OPUS

if (contains(OPENSSL_ENABLED, TRUE)) CONFIG += WITH_OPENSSL

WITH_OPUS:DEFINES += OPUS
WITH_OPENSSL:DEFINES += OPENSSL

#Include opus headers
WITH_OPUS {
win32:INCLUDEPATH += $$WIN_OPUS_INCLUDE
android:INCLUDEPATH += $$ANDROID_OPUS_INCLUDE
unix:!macx:!android:INCLUDEPATH += $$LINUX_OPUS_INCLUDE
}

INCLUDEPATH += $$PWD/AudioStreamingLib

win32-g++ { #MINGW
message("Using settings for Windows MinGW 32 bits")

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB:LIBS += "$$PWD/AudioStreamingLibCore/lib/x86/Debug/libAudioStreamingLibCore.a"
CONFIG(release, debug|release):!AUDIOSTREAMINGLIB:LIBS += "$$PWD/AudioStreamingLibCore/lib/x86/Release/libAudioStreamingLibCore.a"

WITH_OPUS:LIBS += $$MINGW_OPUS_LIB

WITH_OPENSSL {
LIBS += $$MINGW_OPENSSL_SSL_LIB
LIBS += $$MINGW_OPENSSL_CRYPTO_LIB
LIBS += -lGdi32
}
} #MINGW



contains(QT_ARCH, i386):win32-msvc* { #VISUAL STUDIO 32 bits
message("Using settings for Windows Visual Studio 32 bits")

QMAKE_CXXFLAGS += /wd4482 #Supress typedef enum error
QMAKE_CFLAGS += /wd4244 #Supress loss of data error

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB {
LIBS += "$$PWD/AudioStreamingLibCore/lib/x86/Debug/AudioStreamingLibCore.lib"

WITH_OPUS {
LIBS += $$MSVC_32BITS_DEBUG_OPUS_LIB
LIBS += $$MSVC_32BITS_DEBUG_CELT_LIB
LIBS += $$MSVC_32BITS_DEBUG_SILK_COMMON_LIB
LIBS += $$MSVC_32BITS_DEBUG_SILK_FLOAT_LIB
}
}

CONFIG(release, debug|release):!AUDIOSTREAMINGLIB {
LIBS += "$$PWD/AudioStreamingLibCore/lib/x86/Release/AudioStreamingLibCore.lib"

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

WITH_OPUS {
LIBS += $$MSVC_32BITS_RELEASE_OPUS_LIB
LIBS += $$MSVC_32BITS_RELEASE_CELT_LIB
LIBS += $$MSVC_32BITS_RELEASE_SILK_COMMON_LIB
LIBS += $$MSVC_32BITS_RELEASE_SILK_FLOAT_LIB
}
}

WITH_OPENSSL {
LIBS += $$MSVC_32BITS_OPENSSL_SSL_LIB
LIBS += $$MSVC_32BITS_OPENSSL_CRYPTO_LIB
LIBS += -lGdi32
}
} #VISUAL STUDIO 32 bits



contains(QT_ARCH, x86_64):win32-msvc* { #VISUAL STUDIO 64 bits
message("Using settings for Windows Visual Studio 64 bits")

QMAKE_CXXFLAGS += /wd4482 #Supress typedef enum error
QMAKE_CFLAGS += /wd4244 #Supress loss of data error

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB {
LIBS += "$$PWD/AudioStreamingLibCore/lib/x64/Debug/AudioStreamingLibCore.lib"

WITH_OPUS {
LIBS += $$MSVC_64BITS_DEBUG_OPUS_LIB
LIBS += $$MSVC_64BITS_DEBUG_CELT_LIB
LIBS += $$MSVC_64BITS_DEBUG_SILK_COMMON_LIB
LIBS += $$MSVC_64BITS_DEBUG_SILK_FLOAT_LIB
}
}

CONFIG(release, debug|release):!AUDIOSTREAMINGLIB {
LIBS += "$$PWD/AudioStreamingLibCore/lib/x64/Release/AudioStreamingLibCore.lib"

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

WITH_OPUS {
LIBS += $$MSVC_64BITS_RELEASE_OPUS_LIB
LIBS += $$MSVC_64BITS_RELEASE_CELT_LIB
LIBS += $$MSVC_64BITS_RELEASE_SILK_COMMON_LIB
LIBS += $$MSVC_64BITS_RELEASE_SILK_FLOAT_LIB
}
}

WITH_OPENSSL {
LIBS += $$MSVC_64BITS_OPENSSL_SSL_LIB
LIBS += $$MSVC_64BITS_OPENSSL_CRYPTO_LIB
LIBS += -lGdi32
}
} #VISUAL STUDIO 64 bits



contains(QT_ARCH, i386):unix:!macx:!android { #LINUX 32 bits
message("Using settings for Linux 32 bits")

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB:LIBS += "$$PWD/AudioStreamingLibCore/lib/x86/Debug/libAudioStreamingLibCore.a"
CONFIG(release, debug|release):!AUDIOSTREAMINGLIB:LIBS += "$$PWD/AudioStreamingLibCore/lib/x86/Release/libAudioStreamingLibCore.a"

WITH_OPUS:LIBS += $$LINUX_32BITS_OPUS_LIB
WITH_OPENSSL {
LIBS += $$LLINUX_32BITS_OPENSSL_SSL_LIB
LIBS += $$LINUX_32BITS_OPENSSL_CRYPTO_LIB
LIBS += -ldl
}
} #LINUX 32 bits

contains(QT_ARCH, x86_64):unix:!macx:!android { #LINUX 64 bits
message("Using settings for Linux 64 bits")

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB:LIBS += "$$PWD/AudioStreamingLibCore/lib/x64/Debug/libAudioStreamingLibCore.a"
CONFIG(release, debug|release):!AUDIOSTREAMINGLIB:LIBS += "$$PWD/AudioStreamingLibCore/lib/x64/Release/libAudioStreamingLibCore.a"

WITH_OPUS:LIBS += $$LINUX_64BITS_OPUS_LIB
WITH_OPENSSL {
LIBS += $$LINUX_64BITS_OPENSSL_SSL_LIB
LIBS += $$LINUX_64BITS_OPENSSL_CRYPTO_LIB
LIBS += -ldl
}
} #LINUX 64 bits



contains(ANDROID_TARGET_ARCH, armeabi):android { #ANDROID ARM
message("Using settings for Android ARM")

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB:LIBS += "$$PWD/AudioStreamingLibCore/android-lib/armeabi/Debug/libAudioStreamingLibCore.a"
CONFIG(release, debug|release):!AUDIOSTREAMINGLIB:LIBS += "$$PWD/AudioStreamingLibCore/android-lib/armeabi/Release/libAudioStreamingLibCore.a"

deployment.path = /libs/armeabi

WITH_OPUS {
deployment.files += $$ANDROID_ARM_OPUS_LIB
LIBS += $$ANDROID_ARM_OPUS_LIB
}
WITH_OPENSSL {
deployment.files += $$ANDROID_ARM_OPENSSL_SSL_LIB
deployment.files += $$ANDROID_ARM_OPENSSL_CRYPTO_LIB
LIBS += $$ANDROID_ARM_OPENSSL_SSL_LIB
LIBS += $$ANDROID_ARM_OPENSSL_CRYPTO_LIB
}
INSTALLS += deployment
} #ANDROID ARM

contains(ANDROID_TARGET_ARCH, armeabi-v7a):android { #ANDROID ARMV7A
message("Using settings for Android ARMV7A")

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB:LIBS += "$$PWD/AudioStreamingLibCore/android-lib/armeabi-v7a/Debug/libAudioStreamingLibCore.a"
CONFIG(release, debug|release):!AUDIOSTREAMINGLIB:LIBS += "$$PWD/AudioStreamingLibCore/android-lib/armeabi-v7a/Release/libAudioStreamingLibCore.a"

deployment.path = /libs/armeabi-v7a

WITH_OPUS {
deployment.files += $$ANDROID_ARMV7A_OPUS_LIB
LIBS += $$ANDROID_ARMV7A_OPUS_LIB
}
WITH_OPENSSL {
deployment.files += $$ANDROID_ARMV7A_OPENSSL_SSL_LIB
deployment.files += $$ANDROID_ARMV7A_OPENSSL_CRYPTO_LIB
LIBS += $$ANDROID_ARMV7A_OPENSSL_SSL_LIB
LIBS += $$ANDROID_ARMV7A_OPENSSL_CRYPTO_LIB
}
INSTALLS += deployment
} #ANDROID ARMV7A

contains(ANDROID_TARGET_ARCH, x86):android { #ANDROID x86
message("Using settings for Android x86")

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB:LIBS += "$$PWD/AudioStreamingLibCore/android-lib/x86/Debug/libAudioStreamingLibCore.a"
CONFIG(release, debug|release):!AUDIOSTREAMINGLIB:LIBS += "$$PWD/AudioStreamingLibCore/android-lib/x86/Release/libAudioStreamingLibCore.a"

deployment.path = /libs/x86

WITH_OPUS {
deployment.files += $$ANDROID_X86_OPUS_LIB
LIBS += $$ANDROID_X86_OPUS_LIB
}
WITH_OPENSSL {
deployment.files += $$ANDROID_X86_OPENSSL_SSL_LIB
deployment.files += $$ANDROID_X86_OPENSSL_CRYPTO_LIB
LIBS += $$ANDROID_X86_OPENSSL_SSL_LIB
LIBS += $$ANDROID_X86_OPENSSL_CRYPTO_LIB
}
} #ANDROID x86

