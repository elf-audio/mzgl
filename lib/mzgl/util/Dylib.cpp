

#include "Dylib.h"

using namespace std;
#ifndef _WIN32
#include <dlfcn.h>
bool Dylib::open(string path) {
	close();
	
	dylib = dlopen(path.c_str(), RTLD_LAZY);
	
	if (dylib == nullptr) {
		// report error ...
		printf("\xE2\x9D\x8C\xE2\x9D\x8C\xE2\x9D\x8C Error: No dice loading dylib \xE2\x9D\x8C\xE2\x9D\x8C\xE2\x9D\x8C\n");
		return false;
	} else {
		// use the result in a call to dlsym
		printf("\xE2\x9C\x85\xE2\x9C\x85\xE2\x9C\x85 Success loading \xE2\x9C\x85\xE2\x9C\x85\xE2\x9C\x85\n");
		return true;
	}
}

void *Dylib::getFunctionPointer(string funcName) {
	return dlsym(dylib, funcName.c_str());
}
void *Dylib::get(string funcName) {
	void *ptrFunc = dlsym(dylib, funcName.c_str());
	if(ptrFunc!=NULL) {
		return ((void *(*)())ptrFunc)();
	} else {
		printf("Couldn't find the %s() function\n", funcName.c_str());
		return NULL;
	}
}

void Dylib::close() {
	if(dylib!=NULL) {
		dlclose(dylib);
		dylib = NULL;
	}
}

bool Dylib::isOpen() {
	return dylib != NULL;
}
#endif
