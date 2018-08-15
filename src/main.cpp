#include <Zany80/Zany80.hpp>
#include <Zany80/Plugins.hpp>

#include <SFML/Graphics.hpp>
#include <iostream>

#ifdef linux
#include <unistd.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, const char **argv){
	char buf[256];
	#ifdef linux
	readlink("/proc/self/exe", buf, 256);
	#endif
	#ifdef _WIN32
	GetModuleFileName(NULL, buf, 256);
	#endif
	path = buf;
	folder = path.substr(0,path.find_last_of("/")+1);
	#ifdef linux
	folder += "../share/zany80/";
	#endif
	new Zany80();
	zany->run();
	return 0;
}
