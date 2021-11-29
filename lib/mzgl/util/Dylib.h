#pragma once
#include <string>

class Dylib {
public:
	
	bool open(std::string path);
	
	// this gets a void pointer object by
	// calling the method in the parameter,
	// or returns null if method can't be found.
	void *get(std::string methodName);
	
	// This returns the raw function pointer
	// - you need to cast it approprately to call it.
	// e.g. ((void (*)())ptrFunc)(); will be for void func();
	// 		((void *(*)())ptrFunc)(); will be fore void *func(); - i.e return void pointer
	// 		((int(*)(float))ptrFunc)(a); will be for int func(float a);
	void *getFunctionPointer(std::string funcName);
	void close();
	
	bool isOpen();
private:
	
	void *dylib = NULL;
};
