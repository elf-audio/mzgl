# MZGL

MZGL is a cross-platform application library and a collection of other useful of libraries. 

It works on mac, windows, iOS, android, linux etc.

There is a template project for using the library here: https://github.com/elf-audio/mzgl-starter

`lib/` - contains all the core libraries

`lib/mzgl` - the application framework

App is the main class (in [src/App.h](src/App.h)) but if you're making a plugin there is a subclass called PluginEditor which has more of a plugin API.

The graphics are all drawn with OpenGL, the backend residing in src/app and src/graphics. There should be no need to call actual OpenGL API in your app. You may need to write some GLSL shaders if you want to do things other than flat shapes though.

## UI
The UI part of the framework is in src/ui - everything inherits from Layer - which can be assembled in a tree structure.

It's a bit ugly but the tree is raw pointers of Layers and owned by the tree, so when the root Layer is deleted it will delete any children. The main reason for this is brevity in the code, rather than having to have shared pointer syntax everywhere.

As a rule, all layout must happen in doLayout() - this is a hierarchical call that starts at the root node and bubbles down depth-first. A layer must never set its own bounds - that is the responsibility of the parent (this rule is not enforced but saves headaches).

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
