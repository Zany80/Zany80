#pragma once

#ifndef NEEDED_PLUGINS
#error Must define NEEDED_PLUGINS !
#endif

#include <Zany80/Plugins.hpp>
#include <liblib/liblib.hpp>
#include <SFML/Window.hpp>

PluginType type = Runner;
extern RunnerType runner_type;

extern "C" {
	bool isSignatureCompatible(const char *sig);
	PluginType *getType();
	bool isCategory(const char *sig);
	bool isType(const char *sig);
	RunnerType *getRunnerType();
	const char *neededPlugins();
	void run();
	bool activate(const char *arg);
	void event(sf::Event &e);
	void postMessage(PluginMessage m);
}

PluginType *getType(){
	return &type;
}

RunnerType *getRunnerType() {
	return &runner_type;
}

const char *neededPlugins() {
	return NEEDED_PLUGINS;
}
