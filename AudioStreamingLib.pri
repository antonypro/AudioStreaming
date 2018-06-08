include (AudioStreamingLibSettings.pri)

QT += core network multimedia
CONFIG += c++11

if (contains(OPUS_ENABLED, TRUE)) CONFIG += WITH_OPUS

if (contains(OPENSSL_ENABLED, TRUE)) CONFIG += WITH_OPENSSL

if (contains(DEBUG_LEVEL, 1)) DEFINES += IS_TO_DEBUG_VERBOSE_1
if (contains(DEBUG_LEVEL, 2)) DEFINES += IS_TO_DEBUG_VERBOSE_2

WITH_OPUS:DEFINES += OPUS
WITH_OPENSSL:DEFINES += OPENSSL

#Include opus headers
WITH_OPUS {
win32:INCLUDEPATH += $$WIN_OPUS_INCLUDE
android:INCLUDEPATH += $$ANDROID_OPUS_INCLUDE
unix:!macx:!android:INCLUDEPATH += $$LINUX_OPUS_INCLUDE
macx:INCLUDEPATH += $$MACOS_OPUS_INCLUDE
}

INCLUDEPATH += $$PWD/AudioStreamingLib

contains(QT_ARCH, i386):win32-g++ { #MINGW 32 bits
message("Using settings for Windows MinGW 32 bits")

CONFIG += KNOWNDEVICE

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/lib/x86/Debug/" -lAudioStreamingLibCore
CONFIG(release, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/lib/x86/Release/" -lAudioStreamingLibCore

WITH_OPUS:LIBS += $$MINGW_32BITS_OPUS_LIB

WITH_OPENSSL {
LIBS += $$MINGW_32BITS_OPENSSL_SSL_LIB
LIBS += $$MINGW_32BITS_OPENSSL_CRYPTO_LIB
LIBS += -lGdi32
}
} #MINGW 32 bits

contains(QT_ARCH, x86_64):win32-g++ { #MINGW 64 bits
message("Using settings for Windows MinGW 64 bits")

CONFIG += KNOWNDEVICE

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/lib/x64/Debug/" -lAudioStreamingLibCore
CONFIG(release, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/lib/x64/Release/" -lAudioStreamingLibCore

WITH_OPUS:LIBS += $$MINGW_64BITS_OPUS_LIB

WITH_OPENSSL {
LIBS += $$MINGW_64BITS_OPENSSL_SSL_LIB
LIBS += $$MINGW_64BITS_OPENSSL_CRYPTO_LIB
LIBS += -lGdi32
}
} #MINGW 64 bits

contains(QT_ARCH, i386):win32-msvc* { #VISUAL STUDIO 32 bits
message("Using settings for Windows Visual Studio 32 bits")

CONFIG += KNOWNDEVICE

QMAKE_CXXFLAGS += /wd4482 #Supress typedef enum error
QMAKE_CFLAGS += /wd4482 #Supress typedef enum error
QMAKE_CXXFLAGS += /wd4267 #Supress loss of data error
QMAKE_CFLAGS += /wd4267 #Supress loss of data error
QMAKE_CXXFLAGS += /wd4244 #Supress loss of data error
QMAKE_CFLAGS += /wd4244 #Supress loss of data error

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB {
LIBS += -L"$$PWD/AudioStreamingLibCore/lib/x86/Debug/" -lAudioStreamingLibCore

WITH_OPUS {
LIBS += $$MSVC_32BITS_DEBUG_OPUS_LIB
LIBS += $$MSVC_32BITS_DEBUG_CELT_LIB
LIBS += $$MSVC_32BITS_DEBUG_SILK_COMMON_LIB
LIBS += $$MSVC_32BITS_DEBUG_SILK_FLOAT_LIB
}
}

CONFIG(release, debug|release):!AUDIOSTREAMINGLIB {
LIBS += -L"$$PWD/AudioStreamingLibCore/lib/x86/Release/" -lAudioStreamingLibCore

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

CONFIG += KNOWNDEVICE

QMAKE_CXXFLAGS += /wd4482 #Supress typedef enum error
QMAKE_CFLAGS += /wd4482 #Supress typedef enum error
QMAKE_CXXFLAGS += /wd4267 #Supress loss of data error
QMAKE_CFLAGS += /wd4267 #Supress loss of data error
QMAKE_CXXFLAGS += /wd4244 #Supress loss of data error
QMAKE_CFLAGS += /wd4244 #Supress loss of data error

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB {
LIBS += -L"$$PWD/AudioStreamingLibCore/lib/x64/Debug/" -lAudioStreamingLibCore

WITH_OPUS {
LIBS += $$MSVC_64BITS_DEBUG_OPUS_LIB
LIBS += $$MSVC_64BITS_DEBUG_CELT_LIB
LIBS += $$MSVC_64BITS_DEBUG_SILK_COMMON_LIB
LIBS += $$MSVC_64BITS_DEBUG_SILK_FLOAT_LIB
}
}

CONFIG(release, debug|release):!AUDIOSTREAMINGLIB {
LIBS += -L"$$PWD/AudioStreamingLibCore/lib/x64/Release/" -lAudioStreamingLibCore

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

CONFIG += KNOWNDEVICE

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/lib/x86/Debug/" -lAudioStreamingLibCore
CONFIG(release, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/lib/x86/Release/" -lAudioStreamingLibCore

WITH_OPUS:LIBS += $$LINUX_32BITS_OPUS_LIB
WITH_OPENSSL {
LIBS += $$LLINUX_32BITS_OPENSSL_SSL_LIB
LIBS += $$LINUX_32BITS_OPENSSL_CRYPTO_LIB
LIBS += -ldl
}
} #LINUX 32 bits

contains(QT_ARCH, x86_64):unix:!macx:!android { #LINUX 64 bits
message("Using settings for Linux 64 bits")

CONFIG += KNOWNDEVICE

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/lib/x64/Debug/" -lAudioStreamingLibCore
CONFIG(release, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/lib/x64/Release/" -lAudioStreamingLibCore

WITH_OPUS:LIBS += $$LINUX_64BITS_OPUS_LIB
WITH_OPENSSL {
LIBS += $$LINUX_64BITS_OPENSSL_SSL_LIB
LIBS += $$LINUX_64BITS_OPENSSL_CRYPTO_LIB
LIBS += -ldl
}
} #LINUX 64 bits

contains(QT_ARCH, x86_64):macx { #MACOS
message("Using settings for macOS 64 bits")

CONFIG += KNOWNDEVICE

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/lib/x64/Debug/" -lAudioStreamingLibCore
CONFIG(release, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/lib/x64/Release/" -lAudioStreamingLibCore

WITH_OPUS:LIBS += $$MACOS_OPUS_LIB

WITH_OPENSSL {
LIBS += $$MACOS_OPENSSL_SSL_LIB
LIBS += $$MACOS_OPENSSL_CRYPTO_LIB
}
} #MACOS

contains(ANDROID_TARGET_ARCH, armeabi):android { #ANDROID ARM
message("Using settings for Android ARM")

CONFIG += KNOWNDEVICE

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/android-lib/armeabi/Debug/" -lAudioStreamingLibCore
CONFIG(release, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/android-lib/armeabi/Release/" -lAudioStreamingLibCore

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

CONFIG += KNOWNDEVICE

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/android-lib/armeabi-v7a/Debug/" -lAudioStreamingLibCore
CONFIG(release, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/android-lib/armeabi-v7a/Release/" -lAudioStreamingLibCore

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

CONFIG += KNOWNDEVICE

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/android-lib/x86/Debug/" -lAudioStreamingLibCore
CONFIG(release, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/android-lib/x86/Release/" -lAudioStreamingLibCore

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

!KNOWNDEVICE { #Unknown device
message("Using settings for Unknown Device. OpenSsl and Opus codec not supported!")

WITH_OPUS:DEFINES -= OPUS
WITH_OPENSSL:DEFINES -= OPENSSL

CONFIG -= WITH_OPUS
CONFIG -= WITH_OPENSSL

!AUDIOSTREAMINGLIB:INCLUDEPATH += $$PWD/AudioStreamingLibCore/include

CONFIG(debug, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/unknown_device/Debug/" -lAudioStreamingLibCore
CONFIG(release, debug|release):!AUDIOSTREAMINGLIB:LIBS += -L"$$PWD/AudioStreamingLibCore/unknown_device/Release/" -lAudioStreamingLibCore
} #Unknown device
