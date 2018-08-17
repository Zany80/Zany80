#pragma once

#include <liblib/liblib.hpp>

class BIOSPlugin {
public:
	BIOSPlugin();
	BIOSPlugin(liblib::Library *);
	void initialize(liblib::Library *);
	~BIOSPlugin();
	function operator[](const char *);
	uint8_t operator[](uint16_t);
	uint16_t size();
private:
	liblib::Library *BIOSLibrary;
	int ROM_size;
};
