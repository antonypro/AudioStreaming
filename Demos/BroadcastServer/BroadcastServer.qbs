import qbs

Project {
    Product {
        type: "application"
        name: "BroadcastServer"
        Depends {name: 'cpp'}
        Depends {name: "Qt"; submodules: ["core", "gui", "widgets", "multimedia", "concurrent"]}
        Depends {name: "AudioStreamingLibCore"}
        cpp.includePaths: ["../../AudioStreamingLibCore/AudioStreamingLibCore", "3rdparty/kiss_fft130"]

        Properties {
            condition: qbs.targetOS.contains("windows")
            files: ["*.cpp", "*.h", "3rdparty/kiss_fft130/kiss_fft.c"]
            cpp.staticLibraries: ["Ole32", "winmm", "ksuser", "uuid"]
        }

        Properties {
            condition: !qbs.targetOS.contains("windows")
            files: ["*.cpp", "*.h", "3rdparty/kiss_fft130/kiss_fft.c"]
            excludeFiles: ["qwinloopback.cpp", "qwinloopback.h"]
        }
    }

    property int DEBUG_LEVEL: 1 //2 maximum verbose, 1 less verbose, 0 no debug messages

    property bool OPUS_ENABLED: false
    property bool OPENSSL_ENABLED: false

    property stringList OPUS_LIB_FILES: []
    property stringList OPUS_INCLUDE_FILES: []
    property stringList R8BRAIN_INCLUDE_FILES: []

    property stringList OPENSSL_LIB_FILES: []

    references: ["../../AudioStreamingLibCore/AudioStreamingLibCore/AudioStreamingLibCore.qbs"]
}
