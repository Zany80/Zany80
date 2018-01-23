#include <plugin_manager.hpp>
#include <iostream>
#include <Plugins.hpp>
#include <cstring>

extern std::string folder;

void init(liblib::Library *pm) {
	plugin_manager = pm;
	offloaded = new std::map <std::string,liblib::Library*>;
	plugins = new std::map <std::string,liblib::Library*>;
	std::cout << "[Plugin Manager] Path: "<<folder<<"plugins/\n";
	bool runnerFound = false;
	for (std::string s : *enumerate_plugins()) {
		std::cout << "[Plugin Manager] "<<"Loading \""<<s<<"\"...\n";
		if (attemptLoad(s,&((*plugins)[s]))) {
				std::cout <<"[Plugin Manager] "<< s << " loaded successfully...\n";
		}
		else {
			if (offloaded->at(s)) {
				plugins->erase(s);
				std::cerr << "[Plugin Manager] Missing prereqs for plugin: "<<s<<", offloading...\n";
			}
			else {
				std::cerr << "[Plugin Manager] Invalid plugin: "<<s<<"\n";
			}
		}
	}
	finishLoading();
}

std::vector<std::string> *enumerate_plugins() {
	//TODO: either use a file to track installed plugins or query all valid files in the folder recursively.
	if (plugin_paths == nullptr) {
		plugin_paths = new std::vector <std::string>;
		plugin_paths->push_back("plugins/rom_runner");
		plugin_paths->push_back("plugins/cpu/z80");
	}
	return plugin_paths;
}

void cleanup() {
	if (plugin_paths != nullptr) {
		plugin_paths->clear();
		delete plugin_paths;
		plugin_paths = nullptr;
	}
	if (plugins != nullptr) {
		for (auto pair : *plugins) {
			liblib::Library *library = pair.second;
			try {
				(*library)["cleanup"]();
			}
			catch (std::exception e) {
				std::cerr << "[Plugin Manager] Error cleaning up plugin " << pair.first << '\n';
			}
			delete library;
		}
		delete plugins;
		plugins = nullptr;
	}
}

bool prereqsLoaded(liblib::Library **library) {
	std::string needed = (const char *)(**library)["neededPlugins"]();
	needed += ';';
	bool all_found = true;
	while (needed.find(';') != std::string::npos) {
		std::string curr = needed.substr(0,needed.find(';'));
		needed = needed.substr(needed.find(';')+1,needed.length());
		if (curr.length() == 0 || curr.find('/') == std::string::npos)
			continue;
		std::string category = curr.substr(0,curr.find('/'));
		std::string id = curr.substr(curr.find('/')+1,curr.length());
		bool cur_found = false;
		if (category == "CPU") {
			if (getCPUs() != nullptr) {
				// Check if any of the loaded CPUs match the signature specified
				for (liblib::Library *cpu : *CPUs) {
					try {
						std::string signature = (const char *)(*cpu)["getSignature"]();
						if (signature == id) {
							cur_found = true;
							break;
						}
					}
					// Invalid CPU plugin, will be destroyed later
					catch (std::exception &e) {}
				}
			}
		}
		if (!cur_found) {
			all_found = false;
			break;
		}
	}
	return all_found;
}

bool attemptLoad(std::string name, liblib::Library **library) {
	try {
		*library = new liblib::Library(folder + name);
		// Before initializing it, verify that all needed plugins are loaded
		if (prereqsLoaded(library)) {
			((init_t)(**library)["init"])(plugin_manager);
			return true;
		}
		else {
			(*offloaded)[name]=(*library);
			return false;
		}
	}
	catch (std::exception &e) {
		return false;
	}
}

void gatherPlugins(PluginType type, std::vector<liblib::Library*> *vector) {
	if (vector->empty()) {
		for (auto pair : *plugins) {
			liblib::Library *library = pair.second;
			if (*((PluginType*)(*library)["getType"]()) == type) {
				vector->push_back(library);
			}
		}
	}
}

void gatherPlugins(HardwareType type, std::vector<liblib::Library*> *vector) {
	if (vector->empty()) {
		for (liblib::Library *library : *hardware) {
			if (*((HardwareType*)(*library)["getHardwareType"]()) == type) {
				vector->push_back(library);
			}
		}
	}
}

std::vector<liblib::Library*> *getRunners() {
	if (runners == nullptr) {
		runners = new std::vector <liblib::Library*>;
	}
	gatherPlugins(Runner,runners);
	if (!runners->empty()) {
		return runners;
	}
	return nullptr;
}

std::vector<liblib::Library*> *getHardware() {
	if (hardware == nullptr) {
		hardware = new std::vector <liblib::Library*>;
	}
	gatherPlugins(Hardware,hardware);
	if (!hardware->empty()) {
		return hardware;
	}
	return nullptr;
}

std::vector<liblib::Library*> *getCPUs() {
	if (CPUs == nullptr) {
		CPUs = new std::vector <liblib::Library*>;
	}
	getHardware();
	gatherPlugins(CPU,CPUs);
	if (!CPUs->empty()) {
		return CPUs;
	}
	return nullptr;
}

liblib::Library *getDefaultRunner() {
	getRunners();
	if (runners->empty()) {
		return nullptr;
	}
	for (liblib::Library *library : *runners) {
		if (*(RunnerType*)((*library)["getRunnerType"]()) == Shell) {
			return library;
		}
	}
	// No shell, but there *are* other runners. Just go with the first one.
	return (*runners)[0];
}

liblib::Library *getCPU(const char *signature) {
	if(getCPUs() != nullptr) {
		for (liblib::Library *cpu : *CPUs) {
			const char *sig = (const char *)((*cpu)["getSignature"]());
			if (strcmp(signature,sig) == 0) {
				return cpu;
			}
		}
	}
	return nullptr;
}

void finishLoading() {
	if (offloaded == nullptr)
		return;
	int plength;
	do {
		plength = offloaded->size();
		std::cout << "[Plugin Manager] "<<plength<<" offloaded plugins remaining...\n";
		for (auto pair : *offloaded) {
			if (offloaded->empty()) {
				break;
			}
			try {
				liblib::Library *library = pair.second;
				std::cout << "[Plugin Manager] Attempting late-load of " << pair.first << "\n";
				if (prereqsLoaded(&library)) {
					((init_t)(*library)["init"])(plugin_manager);
					offloaded->erase(pair.first);
					(*plugins)[pair.first] = library;
					std::cout << "[Plugin Manager] Late-load of " << pair.first << " successful!\n";
				}
				else {
					std::cerr << "[Plugin Manager] Late-load of " << pair.first << " unsuccessful.\n";
				}
			}
			catch (std::exception &e) {
				
			}
		}
	}
	while (plength != offloaded->size() && !offloaded->empty());
	delete offloaded;
	offloaded = nullptr;
}

liblib::Library *plugin_manager;

std::vector<std::string> *plugin_paths;
std::map <std::string, liblib::Library*> *plugins;
std::map <std::string,liblib::Library*> * offloaded;
std::vector <liblib::Library*> *runners;
std::vector <liblib::Library*> *hardware;
std::vector <liblib::Library*> *CPUs;
