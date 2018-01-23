#pragma once

#include <Plugins.hpp>
#include <liblib/liblib.hpp>

PluginType type = Runner;
extern RunnerType runner_type;

extern "C" {
	PluginType *getType();
	RunnerType *getRunnerType();
	const char *neededPlugins();
	void run();
	void init(liblib::Library *plugin_manager);
	void cleanup();
}

PluginType *getType(){
	return &type;
}

RunnerType *getRunnerType() {
	return &runner_type;
}
