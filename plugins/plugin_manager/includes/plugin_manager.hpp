#pragma once

#include <string>
#include <map>
#include <vector>
#include <liblib/liblib.hpp>

#include <Zany80/Plugins.hpp>

extern "C" {
	std::vector<std::string> * enumerate_plugins();
	void cleanup();
	void init(liblib::Library *plugin_manager);
	liblib::Library *getDefaultRunner();
	std::vector<liblib::Library*> *getRunners();
	std::vector<liblib::Library*> *getHardware();
	std::vector<liblib::Library*> *getCPUs();
	liblib::Library *getCPU(const char *signature);
	liblib::Library *getRAM(const char *signature);
	liblib::Library *getPlugin(const char *signature);
	liblib::Library *getROMRunner(const char *signature);
	void removePlugin(liblib::Library *plugin);
	bool activateRunner(RunnerType runner, const char *arg);
	// sets sane defaults for PluginMessage for simple messages
	void textMessage(const char *string, const char *routing);
	// send a message to a specific target/targets
	void message(PluginMessage message, const char *target);
	// broadcast a message to all plugins
	void broadcast(PluginMessage message);

}

bool attemptLoad(std::string name, liblib::Library ** library);
bool prereqsLoaded(liblib::Library **library);
bool unneeded(liblib::Library *library);
extern std::map <std::string, liblib::Library*> *plugins;
extern std::vector <liblib::Library*> *runners;
extern std::vector <liblib::Library*> *hardware;
extern std::vector <liblib::Library*> *CPUs;
extern std::map <std::string,liblib::Library*> *offloaded;
extern std::vector <std::string> *plugin_paths;
extern liblib::Library *plugin_manager;

void finishLoading();
