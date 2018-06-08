# Audio Streaming library for Qt5

Cross platform audio streaming library for local networks and VOIP for the internet too.

### Features:

* Fully multi threaded.
* Audio can be compressed using Opus codec.
* Audio can be buffered to reduce the occurrence of buffer underrun in the client side.
* Can work on Broadcast or Walkie Talkie modes.
* Can be compiled targeting any Qt5 supported platform.
* Low latency modes are supported.

### Needed:

Qt 5.x

C++11 compiler.

You can choose to compress audio with Opus codec or not. If choose to compress you'll need to compile [Opus](https://www.opus-codec.org/) to your targets(Windows, Linux, macOS or Android), and need to download [r8brain](https://github.com/avaneev/r8brain-free-src). To compile Opus for android see the opus-android README.

If you want to static link [OpenSsl](https://www.openssl.org/),(independently if generating a shared or static library) just compile OpenSsl to fit your needs.

If not compressing audio and not directly dealing with OpenSsl the only requirement is Qt5 and a C++11 compiler.

### Compilation Info:

To compile the library you need to choose if you want to compress audio using Opus codec or not, and if you want to static link OpenSsl or not.

If you want to use Opus edit AudioStreamingLibSettings.pri setting `OPUS_ENABLED = TRUE`, with the appropriated version of Opus compiled just set the include folder of r8brain and Opus, and the Opus libraries location in AudioStreamingLibSettings.pri. If you don't want to use Opus leave `OPUS_ENABLED` blank or with any other value than `TRUE`.

The same applies to OpenSsl, if you want to static link OpenSsl set `OPENSSL_ENABLED = TRUE`.(Please not that even if OpenSsl won't be statically linked it may be available through dynamic linking at runtime.)
With the appropriated version of OpenSsl compiled just set the include folder and the libraries location of OpenSsl in AudioStreamingLibSettings.pri.

With the appropriate settings set (using Opus or not, static linking OpenSsl or not) build the buildlib.pro, if it's already built and you are changing settings remember to re-run qmake and rebuild.

### Using the library:

Using the library is easy, just add `include(path/to/AudioStreamingLib.pri)` to your project file.

To get started coding add `#include <AudioStreamingLibCore>` to your header/source file.

This project comes with demos of usage, just build and test them.

For more information of how to use the APIs, please see the [Wiki](https://github.com/antonypro/AudioStreaming/wiki).

### Using the server WebServer:

This repository also includes a small server for who wants a server managing the connections, on local networks or the internet!

It's just a Server that manages users with ID and password on a SQLite database thus do not require a SQL server. When one user connects to other, they are directly connected using the TCP hole punching technique, which means that the server load is reduced because the data is transferred directly from one peer to another, called connection Peer to Peer or P2P.

What this Server do is very basic to demonstrate the concept! Modify it to fit your needs! Also modify the client that are inside the library `webclient.cpp` and `webclient.h`!

### Third party libraries:

[Eigen](http://eigen.tuxfamily.org/)

[Opus](http://opus-codec.org/)

[r8brain](https://github.com/avaneev/r8brain-free-src)

[OpenSsl](https://www.openssl.org/)

[Kiss FFT](http://kissfft.sourceforge.net/)
