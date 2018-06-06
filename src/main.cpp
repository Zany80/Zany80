#include <Zany80/Zany80.hpp>
#include <Zany80/Drawing.hpp>
#include <Zany80/LuaPlugin.hpp>

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

std::vector<Plugin*> plugins;

void close_state(lua_State **state) {
	std::cout << "[Cleanup] Closing Lua instance 0x" << hex((long)*state) << "...\n";
	lua_close(*state);
	*state = nullptr;
}

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
	if (!lua_state)
		close("Error loading Lua!");
	L = lua_state;
	std::cerr << "Loading the font - as there are multiple locations to search,"
	" you may see messages \"Failed to load image\", these messages are harmless.\n";
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
	// Load core Lua libs
	lua_pushcfunction(L, luaopen_base);
	lua_pushstring(L,"");
	lua_call(L, 1, 0);
	lua_pushcfunction(L, luaopen_table);
	lua_pushstring(L, LUA_TABLIBNAME);
	lua_call(L, 1, 0);
	if (luaL_dofile(L, (folder + "global_rt/global_initialization.lua").c_str())) {
		close(lua_tostring(L, -1));
	}
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

#define GLYPHS_PER_LINE (LCD_WIDTH / (GLYPH_WIDTH))

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
		if (message.size() <= GLYPHS_PER_LINE)
			text(message, 0, 0, sf::Color(0, 255, 255));
		else {
			std::string msg = message;
			int y = - GLYPH_HEIGHT;
			while (msg.size() > GLYPHS_PER_LINE) {
				text(msg.substr(0, GLYPHS_PER_LINE), 0, y += (GLYPH_HEIGHT + 2), sf::Color(0, 255, 255));
				msg = msg.substr(GLYPHS_PER_LINE);
			}
			text(msg, 0, y += (GLYPH_HEIGHT + 2), sf::Color(0, 255, 255));
		}
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
	std::vector<Plugin*> uninitialized_plugins = gatherPlugins();
	for (Plugin* p : uninitialized_plugins) {
		std::cout << "[PluginLoader] Initializing plugin: "<<p->getName()<<'\n';
		if (p->loadIntoRuntime()) {
			std::cout << "\t[PluginLoader] Loaded successfully!\n";
			plugins.push_back(p);
		}
		else {
			std::cerr << "\t[PluginLoader] Error: "<<p->getDescription()<<'\n';
			delete p;	
		}
	}
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

std::vector<Plugin*> gatherPlugins() {
	std::vector<Plugin*> plugins;
	for (LuaPlugin *l : gatherLuaPlugins()) {
		plugins.push_back(l);
	}
	return plugins;
}

std::vector<LuaPlugin*> gatherLuaPlugins() {
	std::vector<LuaPlugin*> plugins;
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
		try {
			plugins.push_back(new LuaPlugin(conf_lua, i));
		}
		catch (std::exception &e) {
			std::cerr << "Error initializing plugin #"<<i<<'\n';
		}
	}
	return plugins;
}
