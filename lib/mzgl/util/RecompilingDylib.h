//
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//
/*


 for this to work, you need to put this in your Other C++ flags
 -DSRCROOT=\"${SRCROOT}\"


 found some good nuggets here
 https://glandium.org/blog/?p=2764
 about how to do precompiled headers.

 What this does is precompile headers when the app starts if we're doing a LiveCodeApp.
 It shouldn't really be in here, this should be for a general RecompilingDylib

 */

#pragma once
#include "FileWatcher.h"
#include "Dylib.h"

#include "filesystem.h"

#include <sys/stat.h>
#include "util.h"
#ifndef SRCROOT
#	define SRCROOT "BUMBO"
#	pragma warning Must set SRCROOT if you want to do livecoding
#endif

#ifndef MZGL_LIBROOT
#	define MZGL_LIBROOT "BUMBO"
#	pragma error Must set LIBROOT if you want to do livecoding
#endif
class RecompilingDylib {
public:
	Dylib dylib;

	//mutex mut;
	FileWatcher watcher;
	std::string path;

	std::function<void(void *)> recompiled;
	std::function<void()> willCloseDylib;

	std::function<void()> successCallback;
	std::function<void(std::string)> failureCallback;

	void setup(std::string path) {
		this->path = findFile(path);

		precompileHeaders();

		watcher.watch(this->path);
		watcher.touched = [this]() { recompile(); };
	}

	// if you want to recompile this class when other files change
	// (they might be dependents for this class) you can add them
	// here manually.
	void addFileToWatch(std::string path) { watcher.watch(path); }

	void update() { watcher.tick(); }

	void precompileHeaders() {
		auto libroot = /*std::string(SRCROOT) + "/../" +*/ MZGL_LIBROOT;

		printf("pwd: %s\n", execute("pwd").c_str());
		auto cmd = "g++ -std=c++14 -DDEBUG -stdlib=libc++ " + getAllIncludes(libroot, macExcludes) + " " + libroot
				   + "/mzgl/App.h";
		printf("Precompiling headers: %s\n", cmd.c_str());
		execute(cmd);
	}

	void lock() { mut.lock(); }

	void unlock() { mut.unlock(); }

	void recompile() {
		printf("Need to recompile %s\n", path.c_str());
		float t = getSeconds();
		//mut.lock();
		auto dylibPath = cc();
		if (dylibPath != "") {
			loadDylib(dylibPath);
			//mut.unlock();
			printf("Reload took %.0fms\n", (getSeconds() - t) * 1000.f);
			if (successCallback != nullptr) {
				successCallback();
			}
		} else {
			if (failureCallback != nullptr) {
				failureCallback(lastErrorStr);
			}
		}
	}

private:
	std::vector<std::string> macExcludes = {"glfw", "MetalANGLE"};
	std::string lastErrorStr;

	std::string cc() {
		// call our makefile
		auto objectName = getObjectName(path);
		auto objFile	= "/tmp/" + objectName + ".o";
		auto cppFile	= "/tmp/" + objectName + ".cpp";
		auto dylibPath	= "/tmp/" + objectName + ".dylib";
		//printf("Obj name %s\n", objectName.c_str());
		makeCppFile(cppFile, objectName);

		std::string libroot = /*std::string(SRCROOT) + "/../" +*/ MZGL_LIBROOT;

		auto includes = getAllIncludes(SRCROOT) + " -include " + libroot + "/mzgl/App.h"
						+ getAllIncludes(libroot, macExcludes) + "/mzgl/ "
			//		+ getAllIncludes(libroot+"/../opt/imgui/")

			//		+ getAllIncludes(libroot+"/../opt/dsp/")
			;
		//+ " -I"+string(SRCROOT)+"/sf2cute/include";

		auto cmd = "g++ -std=c++14 -g -Wno-deprecated-declarations -stdlib=libc++ -c " + cppFile + " -o " + objFile
				   + " " + includes;

		int exitCode = 0;
		auto res	 = execute(cmd, &exitCode);

		//printf("Exit code : %d\n", exitCode);
		if (exitCode == 0) {
			cmd = "g++ -dynamiclib -g -undefined dynamic_lookup -o " + dylibPath + " " + objFile;
			execute(cmd);
			return dylibPath;
		} else {
			lastErrorStr = res;
			return "";
		}
	}

	void recursivelyFindAllDirs(fs::path curr, std::vector<fs::path> &dirs) {
		dirs.push_back(curr);
		for (const auto &entry: fs::directory_iterator(curr)) {
			//		for(int i = 0; i < curr.size(); i++) {
			if (entry.path().filename().string() == "..") continue;
			printf(">> %s\n", entry.path().filename().string().c_str());
			if (fs::is_directory(entry.path())) {
				recursivelyFindAllDirs(entry.path(), dirs);
			}
		}
	}

	std::string getAllIncludes(std::string basePath, const std::vector<std::string> &excludes = {}) {
		std::string includes = "";
		std::vector<fs::path> dirs;
		fs::path dir(basePath);
		recursivelyFindAllDirs(dir, dirs);
		for (int i = 0; i < dirs.size(); i++) {
			//	printf("%s\n", dirs[i].path.c_str());
			bool exclude = false;
			for (const auto &ex: excludes) {
				if (dirs[i].string().find(ex) != -1) {
					exclude = true;
					printf("Excluding %s\n", ex.c_str());
					break;
				} else {
					printf("path does not contain %s: %s\n", ex.c_str(), dirs[i].string().c_str());
				}
			}
			if (!exclude) {
				printf("Include: %s\n", dirs[i].string().c_str());
				includes += " -I\"" + dirs[i].string() + "\"";
			} else {
				printf("Exclude: %s\n", dirs[i].string().c_str());
			}
		}
		return includes;
	}

	std::string getObjectName(std::string p) {
		// split on last '/'
		int indexOfLastSlash = p.rfind('/');
		if (indexOfLastSlash != -1) {
			p = p.substr(indexOfLastSlash + 1);
		}

		// split on last '.'
		int indexOfLastDot = p.rfind('.');
		if (indexOfLastDot != -1) {
			p = p.substr(0, indexOfLastDot);
		}
		return p;
	}

	virtual void makeCppFile(std::string path, std::string objName) {
		fs::ofstream outFile(fs::u8path(path));

		outFile << "#include \"" + objName + ".h\"\n\n";
		outFile << "extern \"C\" {\n\n";
		outFile << "\n\nvoid *getPluginPtr() {return new " + objName + "(); };\n\n";
		outFile << "}\n\n";

		outFile.close();
	}

	virtual void loadDylib(std::string dylibPath) {
		lock();
		if (dylib.isOpen()) {
			willCloseDylib();
			dylib.close();
		}
		if (dylib.open(dylibPath)) {
			void *dlib = dylib.get("getPluginPtr");
			if (recompiled) {
				recompiled(dlib);
			}
		} else {
			printf("Couldn't even open dylib\n");
		}
		unlock();
	}

	std::mutex mut;

	/////////////////////////////////////////////////////////

	std::string findFile(std::string file) {
		auto f = file;
		if (fileExists(f)) return f;
		f = "src/" + f;
		if (fileExists(f)) return f;
		f = "../" + f;
		if (fileExists(f)) return f;
		f = std::string(SRCROOT) + "/" + file;
		if (fileExists(f)) return f;
		printf("SRCROOT set to %s, looking for a file called '%s'\n", SRCROOT, file.c_str());
		printf(
			"Livecoding not gonna work, you may not have set your Other C++ flags to \"-DSRCROOT=\\\"${SRCROOT}\\\"\n");
		return f;
	}

	bool fileExists(std::string path) {
		struct stat fileStat;
		return !(stat(path.c_str(), &fileStat) < 0);
	}
};
