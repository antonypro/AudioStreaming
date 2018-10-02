import qbs

Project {
    Product {
        type: "application"
        name: "WalkieTalkie"
        Depends {name: 'cpp'}
        Depends {name: "Qt"; submodules: ["core", "gui", "widgets", "multimedia"]}
        Depends {name: "AudioStreamingLibCore"}
        cpp.includePaths: ["../../AudioStreamingLibCore/AudioStreamingLibCore"]
		files: ["*.cpp", "*.h"]
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
