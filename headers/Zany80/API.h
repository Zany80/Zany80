#pragma once

#include <Core/Containers/Array.h>
#include <Core/String/String.h>

#include "Plugin.h"

Array<String> readDirectory(String path);
/// Resolves assigns and strips file:/// from beginning
String processFileURI(String path);

extern unsigned long plugin_instances;

Array<Plugin*> getPlugins(String type);
String getPluginName(Plugin *plugin);
bool requirePlugin(String type);

extern "C" void reportError(const char *error_message);
