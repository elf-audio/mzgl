//
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//
/*


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
#include "pathUtil.h"

#include <sys/stat.h>
#include "util.h"

template <class T>
class RecompilingDylib {
public:
	Dylib dylib;

	FileWatcher watcher;
	std::string path;

	std::function<void(std::shared_ptr<T>)> recompiled = [](std::shared_ptr<T>) {};
	std::function<void()> willCloseDylib			   = []() {};

	std::function<void()> successCallback			 = []() {};
	std::function<void(std::string)> failureCallback = [](std::string) {};
	std::string srcRoot;

	std::string libRoot;
	std::string hFileName;
	std::vector<std::string> userIncludes;
	void setup(const std::string &path,
			   const std::string &mzglRoot,
			   const std::vector<std::string> &includes = {}) {
		this->userIncludes = includes;
		hFileName		   = fs::path(path).filename().string();
		libRoot			   = mzglRoot;
		this->path		   = findFile(path);

		if (!fs::exists(path)) {
			throw std::runtime_error("File doesn't exist: " + path);
		}

		srcRoot = fs::path(path).parent_path().string();
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
		printf("pwd: %s\n", execute("pwd").c_str());
		auto cmd = "g++ -std=c++20 -DDEBUG -stdlib=libc++ " + getAllIncludes(libRoot, macExcludes) + " " + libRoot
				   + "/mzgl/App.h";
		printf("Precompiling headers: %s\n", cmd.c_str());
		execute(cmd);
	}

	bool tryLock() { return mut.try_lock(); }
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
	std::vector<std::string> macExcludes = {"glfw"};
	std::string lastErrorStr;

	std::string makeUserIncludes() {
		if (userIncludes.empty()) return "";
		std::string inc = "";
		for (const auto &i: userIncludes) {
			inc += " -I" + i;
		}
		inc += " ";
		return inc;
	}
	std::string cc() {
		// call our makefile
		auto objectName = getObjectName(path);
		auto objFile	= "/tmp/" + objectName + ".o";
		auto cppFile	= "/tmp/" + objectName + ".cpp";
		auto dylibPath	= "/tmp/" + objectName + ".dylib";
		makeCppFile(cppFile, objectName);

		auto includes = makeUserIncludes() +
						//			getAllIncludes(srcRoot)
						"-I" + srcRoot + " " + " -include " + libRoot + "/mzgl/App.h"
						+ getAllIncludes(libRoot, macExcludes) + "/mzgl/ ";

		auto cmd = "g++ -std=c++20 -g -Wno-deprecated-declarations -stdlib=libc++ -c " + cppFile + " -o " + objFile
				   + " " + includes;

		int exitCode = 0;
		printf("cc: %s\n", cmd.c_str());
		auto res = execute(cmd, &exitCode);

		if (exitCode != 0) {
			printf("Error compiling %s\n", cppFile.c_str());
			printf("%s\n", res.c_str());
			lastErrorStr = res;
			//			setWindowTitle(hFileName + " ☠\uFE0F❌☠\uFE0F");
			return "";
		}
		//		setWindowTitle(hFileName + "OK"); //\uD83C\uDD97");
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
		std::ofstream outFile(u8path(path));

		outFile << "#include \"" + objName + ".h\"\n\n";
		outFile << "extern \"C\" {\n\n";
		outFile << "\n\nvoid *getPluginPtr() {return new " + objName + "(); };\n\n";
		outFile << "}\n\n";

		outFile.close();
	}

	virtual void loadDylib(const std::string &dylibPath) {
		lock();
		if (dylib.isOpen()) {
			if (willCloseDylib) willCloseDylib();
			dylib.close();
		}
		if (dylib.open(dylibPath)) {
			auto *dlib = static_cast<T *>(dylib.get("getPluginPtr"));
			if (recompiled) {
				recompiled(std::shared_ptr<T>(dlib));
			}
		} else {
			printf("Couldn't open dylib\n");
		}
		unlock();
	}

	std::mutex mut;

	std::string findFile(const std::string &file) {
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
