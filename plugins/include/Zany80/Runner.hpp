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
	// Called every time the runner is activated - this can be used to e.g. reset timers
	void activate();
	void event(sf::Event &e);
}

PluginType *getType(){
	return &type;
}

RunnerType *getRunnerType() {
	return &runner_type;
}
