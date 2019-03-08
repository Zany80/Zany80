#pragma once

#include <Zany80/Plugin.h>

#include <Core/Containers/Array.h>
#include <Core/Containers/Map.h>
#include <Core/String/String.h>
#include <Core/String/StringBuilder.h>
using namespace Oryol;

class scas {
public:
	int transform(Array<String> sources, String destination, StringBuilder *out);
	Map<String, Array<String>> supportedTransforms();
	bool supports(String type);
	void addIncludeDirectory(String dir);
	String getChain();
	void setVerbosity(int verbosity);
private:
	StringBuilder include_dirs;
};
