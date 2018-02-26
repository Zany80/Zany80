#pragma once

#define NEEDED_PLUGINS ""
#include <Zany80/Generic.hpp>

extern "C" {
	bool isCategory(const char *);
	bool isType(const char *);
}

void run(std::vector<std::string> *args);
void messageShell(const char *message);
void assemble(std::string source, std::string destination);
