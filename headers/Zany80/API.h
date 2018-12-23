#pragma once

#include <Core/Containers/Array.h>
#include <Core/String/String.h>

#include "Plugin.h"

Array<String> readDirectory(String path);
/// Resolves assigns and strips file:/// from beginning
String processFileURI(String path);

extern unsigned long plugin_instances;

Array<PPTR> getPlugins(String type);
String getPluginName(PPTR plugin);

extern "C" void reportError(const char *error_message);
