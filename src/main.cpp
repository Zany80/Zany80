#include <Zany80/Zany80.hpp>
#include <Zany80/Plugins.hpp>

#include <SFML/Graphics.hpp>
#include <iostream>

int main(int argc, const char **argv){
	path = argv[0];
	folder =  path.substr(0,path.find_last_of("/")+1);
	#ifdef linux
	folder += "../share/zany80/";
	#endif
	zany = new Zany80();
	zany->run();
	return 0;
}
