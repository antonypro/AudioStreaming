# Audio Streaming example for Qt
Cross platform audio streaming over TCP socket demo.

<br />
<br />
### Features:
* Both client and server are fully multithreaded.
* Audio can be streamed compressed using Opus codec (Not available by default.).
* Change main settings of streaming without need to recompile the application.
* Support multiple clients connected to the server.
* Clients can see a real time level meter.
* Clients can record the sound.
* Audio can be buffered to reduce the occurrence of buffer underrun in the client side.
* On the client side is possible to set a volume that are directly applied to audio samples.
<br />
<br />

### Compilation Info
This application has two modes of usage: 

First mode: Configured to compress audio using Opus codec and them stream it.
Second mode: Configured to transmit raw/uncompressed data directly. (default)

Both methods require Qt 5.x.x and Qt Creator.

If you're not intended to compress the audio and just play with raw/uncompressed audio, ignore these steps and go the second mode of usage.

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

### License:
This application is licensed under the MIT license.
