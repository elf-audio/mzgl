#pragma once

#include "RecompilingDylib.h"
#include <mzgl/gl/Graphics.h>
#include <mzgl/util/log.h>

class RecompilingAppDylib : public RecompilingDylib<App> {
public:
	Graphics &g;
	RecompilingAppDylib(Graphics &g)
		: RecompilingDylib<App>()
		, g(g) {}

	void makeCppFile(const std::string &path, const std::string &objName) override {
		fs::ofstream outFile(fs::u8path(path));

		outFile << "#include \"" + objName + ".h\"\n\n";
		outFile << "extern \"C\" {\n\n";
		outFile << "\n\nApp *getPluginPtr(Graphics &g) {return new " + objName + "(g); };\n\n";
		outFile << "}\n\n";

		outFile.close();
	}

	void loadDylib(const std::string &dylibPath) override {
		lock();
		if (dylib.isOpen()) {
			willCloseDylib();
			dylib.close();
		}
		if (dylib.open(dylibPath)) {
			void *funcPtr = dylib.getFunctionPointer("getPluginPtr");
			if (funcPtr != nullptr) {
				App *app = static_cast<App *>(((App * (*) (Graphics &) ) funcPtr)(g));
				if (recompiled) {
					recompiled(std::shared_ptr<App>(app));
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
