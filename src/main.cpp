#include <Zany80/Zany80.hpp>
#include <Zany80/Drawing.hpp>

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>

#include <lua.hpp>

#ifdef _WIN32
#include <shlobj.h>
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#define GetCurrentDir getcwd
#endif

#ifdef linux
#include <sys/resource.h>
// limits memory usage to 512MB. This is a bit generous, but leaves room
// for shared libraries et al without risking rapid consumption of all
// available RAM.
constexpr struct rlimit limit = { 1024 * 1024 * 768, 1024 * 1024 * 768 };
#endif

std::string path,folder;
sf::RenderWindow window;
sf::Texture font;
sf::Color background;

lua_State *L;

static void close_state(lua_State **state) {
	std::cout << "[Cleanup] Closing Lua instance 0x" << hex((long)*state) << "...\n";
	lua_close(*state);
	L = *state = nullptr;
}

#define autocl __attribute__((cleanup(close_state)))

int main(int argc, const char **argv) {
	path = absolutize(argv[0]);
	folder = absolutize(path.substr(0,path.find_last_of("/")+1));
	#ifdef linux
	setrlimit(RLIMIT_STACK, &limit);
	folder = absolutize(folder + "../share/zany80/");
	#endif
	window.create(sf::VideoMode(LCD_WIDTH, LCD_HEIGHT), "Zany80");
	window.setFramerateLimit(60);
	autocl lua_State *lua_state = luaL_newstate();
	if (!lua_state) {
		close("Error loading Lua!");
	}
	L = lua_state;
	if (!font.loadFromFile(folder + "font.png")) {
		// tries a few different options for the path
		if (!font.loadFromFile(absolutize(folder + "../font.png")) && !font.loadFromFile(absolutize(folder + "../../font.png")) &&
		!font.loadFromFile("font.png") && !font.loadFromFile("../font.png")) {
			std::cerr << "Failed to load font!\n";
			exit(1);
		}
		else {
			// If the font is in this folder, everything else should be as well
			// Note: this only happens when the font is here AND ALSO NOT where it should be
			std::cout << "[Zany80] " << folder << " setup invalid, falling back to ";
			folder = absolutize(path.substr(0,path.find_last_of("/")+1));
			std::cout << folder << "\n";
		}
	}
	background = sf::Color::White;
	initializePlugins();
	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			switch(event.type) {
				case sf::Event::Closed:
					window.close();
					break;
			}
		}
		window.clear(background);
		window.display();
	}
	return 0;
}

std::string absolutize(std::string relativish) {
	size_t pos;
	while ((pos = relativish.find("../")) != std::string::npos && pos > 3) {
		std::string parent = relativish.substr(0, pos - 3);
		parent = parent.substr(0, parent.rfind("/") + 1);
		std::string child = relativish.substr(pos + 3);
		relativish = parent + child;
	}
	while ((pos = relativish.find("./")) != std::string::npos) {
		std::string parent = relativish.substr(0, pos);
		std::string child = relativish.substr(pos + 2);
		relativish = parent + child;
	}
	if (relativish[0] != '/' && relativish[1] != ':') {
		char *working_directory = new char[FILENAME_MAX];
		if (GetCurrentDir(working_directory, FILENAME_MAX)) {
			relativish = working_directory + (std::string)"/" + relativish;
		}
		delete[] working_directory;
	}
	return relativish;
}

void close(std::string message) {
	std::cerr << "[Crash Handler] " << message << "\n"
				 "[Crash Handler] Notifying user...\n";
	while (window.isOpen()) {
		sf::Event e;
		while (window.pollEvent(e)) {
			switch (e.type) {
				case sf::Event::Closed:
					window.close();
					break;
				default:
					break;
			}
		}
		window.clear(sf::Color(255,0,0));
		text(message, 0, 0, sf::Color(0, 255, 255));
		window.display();
	}
	exit(0);
}

std::string getConfigDirectory() {
	#ifdef _WIN32
	char buffer[MAX_PATH];
	if (::SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, buffer) != S_OK) {
    	close("Error retrieving configuration directory!");
	}
	std::string conf = std::string(buffer) + "/Zany80";
	_mkdir(conf.c_str());
	#else
	const char *homedir;

	if ((homedir = getenv("HOME")) == NULL) {
		homedir = getpwuid(getuid())->pw_dir;
	}
	std::string conf = std::string(homedir) + "/.config/Zany80";
	if (mkdir(conf.c_str(), 0755) != 0 && errno != EEXIST) {
		close("Config directory " + conf + " is inaccessible!");
	}
	return conf;
	#endif
}

void invalid_conf(int i) {
	close("Invalid configuration! Error code: " + hex(i, 2)+"\n"
		  "Please include this number and the file config.lua in the error report.");
}

void initializePlugins() {
	std::vector<PluginDescriptor> plugins = gatherPlugins();
	
}

std::string hex(long a, int length){
	length--;
	static char hex[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	std::string val = "";
	for (int i = length; i >= 0; i--) {
		val += hex[((a & (0xF << (4*i))) >> 4 * i)];
	}
	return val;
}

/*
 * Copies over the default configuration
 */
void default_config(std::string config_file) {
	std::cout << "Writing default configuration...\n";
	std::ifstream default_conf(folder + "default_config.lua");
	std::ofstream conf_file(config_file);
	if (!default_conf.is_open() || !conf_file.is_open())
		close("Error writing default configuration!");
	conf_file << default_conf.rdbuf();
	conf_file.close();
	default_conf.close();
}

std::string fix_path(std::string value) {
	size_t var;
	while ((var = value.find("$ZANY")) != std::string::npos) {
		std::string first_part = value.substr(0, var);
		std::string end_part = value.substr(var + 5, value.size() - var - 5);
		value = first_part + folder.substr(0, folder.size() - 1) + end_part;
	}
	return value;
}

std::string getField(lua_State *L, const char *field) {
	lua_getfield(L, -1, field);
	if (!lua_isstring(L, -1))
		invalid_conf(2);
	std::string value = lua_tostring(L, -1);
	lua_remove(L, -1);
	if (field == "path")
		value = fix_path(value);
	return value;
}

std::vector<PluginDescriptor> gatherPlugins() {
	std::vector<PluginDescriptor> plugins;
	autocl lua_State *conf_lua = luaL_newstate();
	if (!conf_lua)
		close("Error opening Lua instance for configuration parsing!");
	std::string config_file = getConfigDirectory() + "/config.lua";
	std::ifstream conf_file(config_file);
	if (conf_file.is_open())
		conf_file.close();
	else
		default_config(config_file);
	if (luaL_loadfile(conf_lua, config_file.c_str()) || lua_pcall(conf_lua, 0, 0, 0))
		close("Error gathering configuration!");
	lua_getglobal(conf_lua, "plugins");
	if (!lua_istable(conf_lua, -1))
		invalid_conf(0);
	for (int i = 1; i <= lua_objlen(conf_lua, -1); i++) {
		lua_pushnumber(conf_lua, i);
		lua_gettable(conf_lua, -2);
		if (!lua_istable(conf_lua, -1))
			invalid_conf(1);
		plugins.push_back({.name = getField(conf_lua, "name"), .description = getField(conf_lua, "description"), .path = getField(conf_lua, "path")});
		lua_remove(conf_lua, -1);
	}
	return plugins;
}
