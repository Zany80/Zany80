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
	// Called every time the runner is activated - this can be used to e.g. reset timers
	void activate();
}

PluginType *getType(){
	return &type;
}

RunnerType *getRunnerType() {
	return &runner_type;
}
