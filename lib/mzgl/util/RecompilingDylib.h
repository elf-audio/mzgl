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

	FileWatcher watcher;
	std::string path;

	std::function<void(void *)> recompiled = [](void *) {};
	std::function<void()> willCloseDylib   = []() {};

	std::function<void()> successCallback			 = []() {};
	std::function<void(std::string)> failureCallback = [](std::string) {};
	std::string srcRoot;

	void setup(std::string path) {
		this->path = findFile(path);
		srcRoot	   = fs::path(path).parent_path().string();
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
		auto libroot = MZGL_LIBROOT;

		printf("pwd: %s\n", execute("pwd").c_str());
		auto cmd = "g++ -std=c++20 -DDEBUG -stdlib=libc++ " + getAllIncludes(libroot, macExcludes) + " " + libroot
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
		makeCppFile(cppFile, objectName);

		std::string libroot = MZGL_LIBROOT;

		auto includes = getAllIncludes(srcRoot) + " -include " + libroot + "/mzgl/App.h"
						+ getAllIncludes(libroot, macExcludes) + "/mzgl/ ";
		auto cmd = "g++ -std=c++20 -g -Wno-deprecated-declarations -stdlib=libc++ -c " + cppFile + " -o " + objFile
				   + " " + includes;

		int exitCode = 0;
		printf("%s\n", cmd.c_str());
		auto res = execute(cmd, &exitCode);

		if (exitCode != 0) {
			printf("Error compiling %s\n", cppFile.c_str());
			printf("%s\n", res.c_str());
			lastErrorStr = res;
			return "";
		}
		cmd = "g++ -dynamiclib -g -undefined dynamic_lookup -o " + dylibPath + " " + objFile;
		execute(cmd);
		return dylibPath;
	}

	void recursivelyFindAllDirs(fs::path curr, std::vector<fs::path> &dirs) {
		dirs.push_back(curr);
		for (const auto &entry: fs::directory_iterator(curr)) {
			if (fs::is_directory(entry.path())) {
				if (entry.path().filename().string() == "..") continue;
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
			bool exclude = false;
			for (const auto &ex: excludes) {
				if (dirs[i].string().find(ex) != -1) {
					exclude = true;
					break;
				}
			}
			if (!exclude) {
				includes += " -I\"" + dirs[i].string() + "\"";
			}
		}
		return includes;
	}

	std::string getObjectName(std::string p) {
		// split on last '/'
		auto indexOfLastSlash = p.rfind('/');
		if (indexOfLastSlash != -1) {
			p = p.substr(indexOfLastSlash + 1);
		}

		// split on last '.'
		auto indexOfLastDot = p.rfind('.');
		if (indexOfLastDot != -1) {
			p = p.substr(0, indexOfLastDot);
		}
		return p;
	}

	virtual void makeCppFile(const std::string &path, const std::string &objName) {
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
			if (willCloseDylib) willCloseDylib();
			dylib.close();
		}
		if (dylib.open(dylibPath)) {
			void *dlib = dylib.get("getPluginPtr");
			if (recompiled) {
				recompiled(dlib);
			}
		} else {
			printf("Couldn't open dylib\n");
		}
		unlock();
	}

	std::mutex mut;

	std::string findFile(std::string file) {
		auto f = file;
		if (fs::exists(f)) return f;
		f = "src/" + f;
		if (fs::exists(f)) return f;
		f = "../" + f;
		if (fs::exists(f)) return f;
		f = srcRoot + "/" + file;
		if (fs::exists(f)) return f;
		printf("srcRoot set to %s, looking for a file called '%s'\n", srcRoot.c_str(), file.c_str());
		return f;
	}
};
