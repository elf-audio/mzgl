#include "GLFWAppRunner.h"

#ifdef _WIN32
#	include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	int argc = 1;
	//	char *argv[1];
	//	argv[0] = "mzgl";
	//
	char arg0[] = "mzgl";
	char *argv[1];
	argv[0] = arg0;

#else
int main(int argc, char *argv[]) {
#endif

	GLFWAppRunner app;
	app.run(argc, argv);
	return 0;
}