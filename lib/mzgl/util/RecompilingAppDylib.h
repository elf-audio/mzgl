#pragma once

#include "RecompilingDylib.h"
#include "Graphics.h"
#include "log.h"

class RecompilingAppDylib : public RecompilingDylib {
public:
	Graphics &g;
	RecompilingAppDylib(Graphics &g)
		: RecompilingDylib()
		, g(g) {}

	void makeCppFile(const std::string &path, const std::string &objName) override {
		fs::ofstream outFile(fs::u8path(path));

		outFile << "#include \"" + objName + ".h\"\n\n";
		outFile << "extern \"C\" {\n\n";
		outFile << "\n\nApp *getPluginPtr(Graphics &g) {return new " + objName + "(g); };\n\n";
		outFile << "}\n\n";

		outFile.close();
	}

	void loadDylib(std::string dylibPath) override {
		lock();
		if (dylib.isOpen()) {
			willCloseDylib();
			dylib.close();
		}
		if (dylib.open(dylibPath)) {
			void *funcPtr = dylib.getFunctionPointer("getPluginPtr");
			if (funcPtr != nullptr) {
				void *dlib = ((App * (*) (Graphics &) ) funcPtr)(g);
				if (recompiled) {
					recompiled(dlib);
				}
			} else {
				Log::e() << "Couldn't reload dylib";
			}
		} else {
			Log::e() << "Couldn't even open dylib";
		}
		unlock();
	}
};
