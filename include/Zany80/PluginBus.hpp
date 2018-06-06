#pragma once

#include "Plugin.hpp"

class PluginBus {
public:
	PluginBus();
	~PluginBus();
	void attachPlugin(Plugin*);
	void disconnectPlugin(Plugin*);
	std::vector<std::pair<Plugin*,std::string>> notifyPlugins(Plugin *source, std::string message);
private:
	std::vector<Plugin*> attached_plugins;
};
