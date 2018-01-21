#pragma once

#include <string>
#include <vector>

extern "C" {
	std::vector<std::string> * enumerate_plugins();
	void *cleanup();
}

extern std::vector<std::string> * plugins;
