

#include "Dylib.h"

#ifndef _WIN32
#	include <dlfcn.h>
bool Dylib::open(const std::string &path) {
	close();

	dylib = dlopen(path.c_str(), RTLD_LAZY);

	if (dylib == nullptr) {
		// report error ...
		printf(
			"\xE2\x9D\x8C\xE2\x9D\x8C\xE2\x9D\x8C Error: No dice loading dylib \xE2\x9D\x8C\xE2\x9D\x8C\xE2\x9D\x8C\n");
		return false;
	} else {
		// use the result in a call to dlsym
		printf("\xE2\x9C\x85\xE2\x9C\x85\xE2\x9C\x85 Success loading \xE2\x9C\x85\xE2\x9C\x85\xE2\x9C\x85\n");
		return true;
	}
}

void *Dylib::getFunctionPointer(const std::string &funcName) {
	return dlsym(dylib, funcName.c_str());
}
void *Dylib::get(const std::string &funcName) {
	void *ptrFunc = dlsym(dylib, funcName.c_str());
	if (ptrFunc != nullptr) {
		return ((void *(*) ()) ptrFunc)();
	} else {
		printf("Couldn't find the %s() function\n", funcName.c_str());
		return nullptr;
	}
}

void Dylib::close() {
	if (dylib != nullptr) {
		dlclose(dylib);
		dylib = nullptr;
	}
}

bool Dylib::isOpen() {
	return dylib != nullptr;
}
#endif
