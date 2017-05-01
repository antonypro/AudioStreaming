<<<<<<< HEAD
# Audio Streaming library for Qt5

Cross platform audio streaming and VoIP library for local networks.

### Features:

* Fully multi threaded.
* Audio can be compressed using Opus codec.
* Audio can be buffered to reduce the occurrence of buffer underflow in the client side.
* Can work on Broadcast or Walkie Talkie modes.
* Supports Windows Linux and Android.
* Low latency modes are supported.

### Needed:

Qt 5.x

You can choose to compress audio with Opus codec or not. If choose to compress you'll need to compile [Opus](https://www.opus-codec.org/) to your targets(Windows, Linux or Android). To compile Opus for android see the opus-android README, and need to download [r8brain](https://github.com/avaneev/r8brain-free-src)

If you want to static link [OpenSsl](https://www.openssl.org/),(independently if generating a shared or static library) just compile OpenSsl to fit your needs.

If not compressing audio and not directly dealing with OpenSsl the only requirement is Qt5.

### Compilation Info:

To compile the library you need to choose if you want to compress audio using Opus codec or not, and if you want to static link OpenSsl or not.

If you want to use Opus edit AudioStreamingLibSettings.pri setting `OPUS_ENABLED = TRUE`, with the appropriated version of Opus compiled just set the include folder of r8brain and Opus, and the Opus libraries location in AudioStreamingLibSettings.pri. If you don't want to use Opus leave `OPUS_ENABLED` blank or with any other value than `TRUE`.

The same applies to OpenSsl, if you want to static link OpenSsl set `OPENSSL_ENABLED = TRUE`.(Please not that even if OpenSsl won't be statically linked it may be available through dynamic linking at runtime.)
With the appropriated version of OpenSsl compiled just set the include folder and the libraries location of OpenSsl in AudioStreamingLibSettings.pri.

With the appropriate settings set (using Opus or not, static linking OpenSsl or not) build the buildlib.pro, if it's  already built and you are changing settings remember to re-run qmake and rebuild.

### Using the library:

Using the library is easy, just add `include(path/to/AudioStreamingLib.pri)` to your project file.

To get started coding add `#include <AudioStreamingLibCore>` to your header/source file.

This project comes with demos of usage, just build and test them.

For more information of how to use the APIs, see the Wiki.
=======
# Audio Streaming example for Qt

Cross platform audio streaming over TCP socket demo.

### Features:

* Both client and server are fully multithreaded.
* Audio can be streamed compressed using Opus codec (Not available by default.).
* Change main settings of streaming without need to recompile the application.
* Support multiple clients connected to the server.
* Clients can see a real time level meter.
* Clients can record the sound.
* Audio can be buffered to reduce the occurrence of buffer underrun in the client side.
* On the client side is possible to set a volume that are directly applied to audio samples.

### Compilation Info:

This application has two modes of usage: 

First mode: Configured to compress audio using Opus codec and them stream it.

Second mode: Configured to transmit raw/uncompressed data directly. (default)

Both methods require Qt 5.x.x and Qt Creator.

If you're not intended to compress the audio and just play with raw/uncompressed audio, ignore these steps and go to the second mode of usage.

### First mode: 

To enable Opus in your project you need to compile opus for each platform that you are planning to use it.

If you are using windows download it here: https://www.opus-codec.org/downloads/

And see the instructions to compile it: http://lists.xiph.org/pipermail/opus/2013-January/001901.html

Or the instructions to download and compile on Linux/Mac: https://trac.ffmpeg.org/wiki/CompilationGuide/Quick/libopus

Beyond Opus you will need r8brain-free-src (Sample rate converter designed by Aleksey Vaneev of Voxengo), just get it from this address: https://github.com/avaneev/r8brain-free-src

In common.pri file set the appropriate includes and libraries for Opus on your platform and put r8brain-free-src in a subdirectory of the server, like this:
AudioStreaming/Server/r8brain

Enable Opus by uncommenting the first line to CONFIG += WITH_OPUS.

Run qmake and rebuild it and you must be able to use it with Opus codec.

### Second mode:

In uncompressed mode no extra effort is needed, just get the sources and compile it.
>>>>>>> bd8e3c47dc64cb48ad94d596d2ab8aff3f468177

### License:
This application is licensed under the MIT license.
