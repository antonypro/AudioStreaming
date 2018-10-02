import qbs

Product {
    type: "staticlibrary"
    name: "AudioStreamingLibCore"
    Depends {name: 'cpp'}
    Depends {name: "Qt"; submodules: ["core", "network", "multimedia"]}

    property stringList INCLUDES: ["3rdparty"]

    property var DEFINE1: []
    property var DEFINE2: []
    property var DEFINE3: []

    property var LIB1: []
    property var LIB2: []
    property var LIB3: []

    Properties {
        condition: (DEBUG_LEVEL === 1)

        DEFINE1: ["IS_TO_DEBUG_VERBOSE_1"]
    }

    Properties {
        condition: (DEBUG_LEVEL === 2)

        DEFINE1: ["IS_TO_DEBUG_VERBOSE_2"]
    }

    Properties {
        condition: project.OPUS_ENABLED

        files : [
            "*.cpp", "*.h", project.R8BRAIN_INCLUDE_FILES + "/r8bbase.cpp"
        ]

        DEFINE2: ["OPUS"]

        cpp.includePaths: INCLUDES.concat(project.OPUS_INCLUDE_FILES).concat(project.R8BRAIN_INCLUDE_FILES)

        LIB1: project.OPUS_LIB_FILES
    }

    Properties {
        condition: !project.OPUS_ENABLED

        files : [
            "*.cpp", "*.h"
        ]

        excludeFiles: [
            "opusencoderclass.cpp",  "opusencoderclass.h",
            "opusdecoderclass.cpp", "opusdecoderclass.h",
            "r8brain.cpp", "r8brain.h"
        ]
    }

    Properties {
        condition: project.OPENSSL_ENABLED

        DEFINE3: ["OPENSSL"]

        LIB2: project.OPENSSL_LIB_FILES
    }

    Properties {
        condition: project.OPENSSL_ENABLED && qbs.targetOS.contains("windows")

        LIB3: ["Gdi32"]
    }

    cpp.includePaths: INCLUDES

    cpp.defines: base.concat(DEFINE1, DEFINE2, DEFINE3)

    cpp.staticLibraries: base.concat(LIB1, LIB2, LIB3)

    Export {
        Depends {name: "cpp"}
        cpp.includePaths: [product.sourceDirectory, "../include"]
    }
}
