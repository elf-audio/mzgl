#include <string>
#include <typeinfo>
#ifdef __GNUC__
#	include <cxxabi.h>
#	include <cstdlib>
#endif

template <typename T>
std::string className(const T &obj) {
#ifdef __GNUC__
	int status		 = 0;
	char *demangled	 = abi::__cxa_demangle(typeid(obj).name(), nullptr, nullptr, &status);
	std::string name = (status == 0 && demangled) ? demangled : typeid(obj).name();
	free(demangled);
	return name;
#else
	return typeid(obj).name();
#endif
}
