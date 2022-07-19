# MZGL

MZGL is a cross-platform application library and a collection of other useful of libraries. 

It works on mac, windows, iOS, android, linux etc.

lib/ - contains all the core libraries

lib/mzgl - the application framework

## third party libraries
* boost - used for std::filesystem replacement
* fontstash - OpenGL font rendering used by mzgl's Font class
* glm - vector maths for C++ - could probably replace this with something less humungous
* glew, glfw - cross-platform OpenGL windowing
* misc - several useful small libraries that don't need a whole folder
* nanovg - vector drawing in OpenGL - not really used
* portaudio - cross-platform audio-io
* rtmidi - cross-platform midi-io
* yoga - flexbox style layout for C++
* pugixml - xml parser - named as pu_gixml to avoid namespace clash with an Ableton SDK used in koala 
* ZipFile - simple zip file unloading (actually part of this distro, not a third party)
* AudioShare - iOS only framework for importing audio 
* MetalANGLE - Metal implementation of OpenGL - not used yet

There are submodules, be sure to 
```
git submodule init
git submodule update
```

## WINDOWS

Windows needs glfw and glew, both downloaded as binary versions
