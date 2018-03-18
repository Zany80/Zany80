/*
 * Important note: this plugin is very poorly written.
 * Written as quickly as possible to flesh out the main system and the plugin
 * interface as much as possible. Will be completely redone pending a stable
 * release of Zany80.
 *
 * TODO: actually rewrite this garbage.
 */
#include <plugin_manager.hpp>
#include <iostream>
#include <Zany80/Plugins.hpp>
#include <Zany80/GenericException.hpp>
#include <cstring>

typedef void(*post_t)(PluginMessage m);

void init(liblib::Library *pm) {
	plugin_manager = pm;
	offloaded = new std::map <std::string,liblib::Library*>;
	plugins = new std::map <std::string,liblib::Library*>;
	std::cout << "[Plugin Manager] Path: "<<folder<<"plugins/\n";
	bool runnerFound = false;
	for (std::string s : *enumerate_plugins()) {
		s = "plugins/" + s;
		std::cout << "[Plugin Manager] "<<"Loading \""<<s<<"\"...\n";
		liblib::Library * library;
		if (attemptLoad(s,&library)) {
				std::cout <<"[Plugin Manager] "<< s << " loaded successfully...\n";
				(*plugins)[s]=library;
		}
		else {
			try {
				offloaded->at(s);
				std::cerr << "[Plugin Manager] Missing prereqs for plugin: "<<s<<", offloading...\n";
			}
			catch (std::out_of_range) {
				std::cerr << "[Plugin Manager] Invalid plugin: "<<s<<"\n";
			}
		}
	}
	finishLoading();
}

std::vector<std::string> *enumerate_plugins() {
	//TODO: either use a file to track installed plugins or query all valid files in the folder recursively.
	if (plugin_paths == nullptr) {
		plugin_paths = new std::vector <std::string> {
			"BIOSLauncher",
			"cpu/z80cpp_core",
			//"rom_runner",
			"MMU",
			"simple_shell",
			"gpu/zany_old",
			"assembler/scas",
			"compiler/kcc",
			"editor/official"
		};
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
		int plength;
		do {
			plength = plugins->size();
			std::cout << "[Plugin Manager] "<<plength<<" plugins remaining for cleanup...\n";
			for (auto pair : *plugins) {
				try {
					if (unneeded(pair.second)) {
						std::cout << "[Plugin Manager] Cleaning up "<<pair.first<<"\n";
						try {
							((post_t)(*pair.second)["postMessage"])({
								0, "cleanup", sizeof(char) * 7, "PluginManager", nullptr
							});
						} catch(...){}
						delete pair.second;
						plugins->erase(pair.first);
						break;
					}
					else {
						std::cout << "[Plugin Manager] Plugin "<<pair.first<<" is still needed by other plugins; delaying cleanup...\n";
					}
				}
				catch (std::exception) {
					
				}
			}
		} while (plugins->size() != plength && plugins->size() > 0);
		delete plugins;
		plugins = nullptr;
	}
}

bool unneeded(liblib::Library *prereq) {
	for (auto pair : *plugins) {
		liblib::Library *library = pair.second;
		try {
			std::string needed = (const char *)(*library)["neededPlugins"]();
			if (needed == "") {
				continue;
			}
			needed += ';';
			while (needed.find(';') != std::string::npos) {
				std::string curr = needed.substr(0,needed.find(';'));
				needed = needed.substr(needed.find(';')+1,needed.length());
				if (curr.length() == 0 || curr.find('/') == std::string::npos)
					continue;
				std::string category = curr.substr(0,curr.find('/'));
				std::string id = curr.substr(curr.find('/')+1,curr.length());
				if (category == "CPU") {
					try {
						if (*(PluginType*)((*prereq)["getType"]()) == Hardware) {
							if (*(HardwareType*)((*prereq)["getHardwareType"]()) == CPU) {
								if (((bool(*)(const char *))(*prereq)["isSignatureCompatible"])(id.c_str())) {
									// The prereq is needed by another plugin
									return false;
								}
							}
						}
					}
					catch (std::exception e){}
				}
				else if (category == "RAM") {
					try {
						if (*(PluginType*)((*prereq)["getType"]()) == Hardware) {
							if (*(HardwareType*)((*prereq)["getHardwareType"]()) == RAM) {
								if (((bool(*)(const char *))(*prereq)["isSignatureCompatible"])(id.c_str())) {
									// The prereq is needed by another plugin
									return false;
								}
							}
						}
					}
					catch (std::exception e){}
				}
				else {
					if (((bool(*)(const char *))(*library)["isCategory"])(category.c_str())) {
						if (id == "*" || ((bool(*)(const char *))(*library)["isType"])(id.c_str())) {
							return false;
						}
					}
				}
			}
		}
		catch (std::exception) {
			return false;
		}
	}
	return true;
}

bool prereqsLoaded(liblib::Library **library) {
	// let the exception fall through
	std::string needed = (const char *)(**library)["neededPlugins"]();
	try {
		if (needed == "") {
			std::cout << "[Plugin Manager] No prereqs\n";
			return true;
		}
		needed += ';';
		std::cout << "[Plugin Manager] Prereqs: "<<needed<<"\n";
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
							if (((bool(*)(const char *))(*cpu)["isSignatureCompatible"])(id.c_str())) {
								cur_found = true;
								break;
							}
						}
						// Invalid CPU plugin, will be destroyed later
						catch (std::exception) {}
					}
				}
			}
			else if (category == "RAM") {
				if (getHardware() != nullptr) {
					for (liblib::Library *ram : *hardware) {
						try {
							if (*((HardwareType*)((*ram)["getHardwareType"]())) == RAM) {
								if (((bool(*)(const char *))(*ram)["isSignatureCompatible"])(id.c_str())) {
									cur_found = true;
									break;
								}
							}
						}
						catch (std::exception) {}
					}
				}
			}
			else {
				for (auto pair : *plugins) {
					liblib::Library *library = pair.second;
					try {
						if (((bool(*)(const char *))(*library)["isCategory"])(category.c_str())) {
							if (id == "*" || ((bool(*)(const char *))(*library)["isType"])(id.c_str())) {
								cur_found = true;
							}
						}
					}
					catch (...) {
						
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
	catch (std::exception) {
		return false;
	}
}

bool attemptLoad(std::string name, liblib::Library **library) {
	try {
		*library = new liblib::Library(folder + name);
		// Before initializing it, verify that all needed plugins are loaded
		if (prereqsLoaded(library)) {
			((post_t)(**library)["postMessage"])({
				0, "init", sizeof(char) * 4, "PluginManager", (char *)plugin_manager
			});
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
	vector->clear();
	for (auto pair : *plugins) {
		liblib::Library *library = pair.second;
		try {
			if (*((PluginType*)(*library)["getType"]()) == type) {
				vector->push_back(library);
			}
		}
		catch (std::exception) {
			std::cout << "[Plugin Manager] Invalid plugin detected: "<<pair.first<<", removing...\n";
			try {
				(*pair.second)["cleanup"]();
			}
			catch (std::exception){}
			delete pair.second;
			plugins->erase(pair.first);
		}
	}
}

void gatherPlugins(HardwareType type, std::vector<liblib::Library*> *vector) {
	vector->clear();
	for (liblib::Library *library : *hardware) {
		try {
			if (*((HardwareType*)(*library)["getHardwareType"]()) == type) {
				vector->push_back(library);
			}
		}
		catch (std::exception) {
			
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
			if (((bool(*)(const char *))(*library)["activate"])("")) {
				return library;
			}
		}
	}
	// No usable shell, but there *are* other runners. Just go with the first valid one.
	for (liblib::Library *library : *runners) {
		if (((bool(*)(const char *))(*library)["activate"])("")) {
			return library;
		}
	}
}

liblib::Library *getCPU(const char *signature) {
	if(getCPUs() != nullptr) {
		for (liblib::Library *cpu : *CPUs) {
			if (((bool(*)(const char *))(*cpu)["isSignatureCompatible"])(signature)) {
				return cpu;
			}
		}
	}
	return nullptr;
}

liblib::Library *getRAM(const char *signature) {
	if(getHardware() != nullptr) {
		for (liblib::Library *ram : *hardware) {
			if (((bool(*)(const char *))(*ram)["isSignatureCompatible"])(signature)) {
				return ram;
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
			try {
				liblib::Library *library = pair.second;
				std::cout << "[Plugin Manager] Attempting late-load of " << pair.first << "\n";
				if (prereqsLoaded(&library)) {
					(*plugins)[pair.first] = library;
					((post_t)(*library)["postMessage"])({
						0, "init", sizeof(char) * 4, "PluginManager", (char *)plugin_manager
					});
					offloaded->erase(pair.first);
					std::cout << "[Plugin Manager] Late-load of " << pair.first << " successful!\n";
					break;
				}
				else {
					std::cerr << "[Plugin Manager] Late-load of " << pair.first << " unsuccessful.\n";
				}
			}
			catch (std::exception &e) {
				std::cerr << "[Plugin Manager] Late-load of " << pair.first << " unsuccessful (E).\n";
			}
		}
	}
	while (plength != offloaded->size() && !offloaded->empty());
	delete offloaded;
	offloaded = nullptr;
}

void removePlugin(liblib::Library *plugin) {
	for (auto pair : *plugins) {
		if (pair.second == plugin) {
			std::cout << "[Plugin Manager] Removing "<<pair.first<<"\n";
			plugins->erase(pair.first);
			return;
		}
	}
	std::cerr << "[Plugin Manager] Plugin removal requested but plugin not found!\n";
}

bool activateRunner(RunnerType type, const char *arg) {
	// if there's no runners, something is really, *really* wrong.
	if (getRunners() == nullptr)
		throw;
	bool none_found = true;
	for (liblib::Library *l : *runners) {
		if (*(RunnerType*)(*l)["getRunnerType"]() != type)
			continue;
		none_found = false;
		if (((bool(*)(const char *))((*l)["activate"]))(arg)) {
			zany->setRunner(l);
			return true;
		}
	}
	if (none_found) {
		throw GenericException("Incompatible ROM!");
	}
	return false;
}

void broadcast(PluginMessage m) {
	for (auto pair : *plugins) {
		liblib::Library *plugin = pair.second;
		try {
			((post_t)(*plugin)["postMessage"])(m);
		}
		catch (...){}
	}
}

void message(PluginMessage m, const char *_target) {
	std::string target = _target;
	std::string cur;
	while (target.size() > 0) {
		if (target.find_first_of(';') != std::string::npos) {
			cur = target.substr(0,target.find_first_of(';'));
			target = target.substr(target.find_first_of(';') + 1, target.size());
		}
		else {
			cur = target;
			target = "";
		}
		if (cur.find_first_of('/') == std::string::npos) {
			// scratch(this->head); aka: invalid requirement
			return;
		}
		std::string category = cur.substr(0, cur.find_first_of('/'));
		std::string secondary = cur.substr(cur.find_first_of('/') + 1, cur.size());
		if (category == "Runner") {
			if (getRunners() == nullptr)
				throw;
			if (secondary == "ROM") {
				for (liblib::Library *runner : *runners) {
					try {
						if (*((RunnerType*)(*runner)["getRunnerType"]()) == ROMRunner) {
							((post_t)(*runner)["postMessage"])(m);
						}
					}
					catch (...) {}					
				}
			}
			else if (secondary == "Shell") {
				for (liblib::Library *runner : *runners) {
					try {
						if (*((RunnerType*)(*runner)["getRunnerType"]()) == Shell) {
							((post_t)(*runner)["postMessage"])(m);
						}
					}
					catch (...) {}					
				}
			}
			else if (secondary == "*") {
				for (liblib::Library *runner : *runners) {
					try {
						((post_t)(*runner)["postMessage"])(m);
					}
					catch (...) {}
				}
			}
			else {
				// unrecognized type
				message((PluginMessage){
					100, (char*)"unrecognized", (int)strlen("unrecognized"), "PluginManager", (char*)&m
				},m.source);
			}
		}
		else if (category == "CPU") {
			if (getCPUs() == nullptr)
				throw;
			for (liblib::Library *CPU : *CPUs) {
				try {
					if (secondary == "*" || ((bool(*)(const char *))(*CPU)["isSignatureCompatible"])(secondary.c_str())) {
						((post_t)(*CPU)["postMessage"])(m);
					}
				}
				catch (...){}
			}
		}
		else {
			// TODO: remove all checks, have plugins decide whether to accept
			// the message (make the following the default)
			for (auto pair : *plugins) {
				liblib::Library *library = pair.second;
				try {
					if (((bool(*)(const char *))(*library)["isCategory"])(category.c_str())) {
						if (secondary == "*" || ((bool(*)(const char *))(*library)["isType"])(secondary.c_str())) {
							((post_t)(*library)["postMessage"])(m);
						}
					}
				}
				catch (liblib::SymbolLoadingException &e) {
					//std::cerr << "[Plugin Manager] Error querying "<<pair.first << " for messaging: "<<e.what()<<"\n";
				}
				catch (std::exception &e) {
					std::cerr << "[Plugin Manager] Plugin "<<pair.first<<" threw an exception after receiving a message! Error: "<<e.what()<<"\n";
				}
			}
		}
	}
}

void textMessage(const char *string, const char *routing) {
	std::string r = routing;
	message({
		0, (char*)string, (int)strlen(string), (char*)r.substr(0,r.find_first_of(';')).c_str(),(char*)""
	}, (char*)r.substr(r.find_first_of(';')+1,r.size()).c_str());
}

liblib::Library *plugin_manager;

std::vector<std::string> *plugin_paths;
std::map <std::string, liblib::Library*> *plugins;
std::map <std::string,liblib::Library*> * offloaded;
std::vector <liblib::Library*> *runners;
std::vector <liblib::Library*> *hardware;
std::vector <liblib::Library*> *CPUs;
