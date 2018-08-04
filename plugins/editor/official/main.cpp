#define NEEDED_PLUGINS ""

#include <Zany80/Plugins/Runner.hpp>
#include <cstring>

bool isCategory(const char *sig) {
	return !strcmp(sig, "Runner") || !strcmp(sig, "Editor");
}

bool isType(const char *sig) {
	return !strcmp(sig, "Editor");
}

const char *getName() {
	return "Code Editor";
}

liblib::Library *plugin_manager;

RunnerType runner_type = GenericRunner;

bool activate(const char *arg) {
	
}

/**
 * Used to post a message to this plugin
 */
void postMessage(PluginMessage m) {
	if (strcmp(m.data, "init") == 0) {
		plugin_manager = (liblib::Library*)m.context;
	}
	else if (strcmp(m.data, "cleanup") == 0) {
		plugin_manager = nullptr;
	}
}
