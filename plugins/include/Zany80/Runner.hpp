#pragma once

#include <Zany80/Plugins.hpp>
#include <liblib/liblib.hpp>
#include <SFML/Window.hpp>

PluginType type = Runner;
extern RunnerType runner_type;

extern "C" {
	bool isSignatureCompatible(const char *sig);
	PluginType *getType();
	RunnerType *getRunnerType();
	const char *neededPlugins();
	void run();
	void init(liblib::Library *plugin_manager);
	void cleanup();
	bool activate(const char *arg);
	void event(sf::Event &e);
}

PluginType *getType(){
	return &type;
}

RunnerType *getRunnerType() {
	return &runner_type;
}
