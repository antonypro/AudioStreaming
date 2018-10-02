import qbs

Product {
	type: "application"
    consoleApplication: true
	name: "WebServer"
	Depends {name: 'cpp'}
    Depends {name: "Qt"; submodules: ["core", "network", "sql"]}
    cpp.includePaths: ["3rdparty"]
	files: ["*.cpp", "*.h"]
}
