#pragma once

#ifndef NEEDED_PLUGINS
#error Must define NEEDED_PLUGINS !
#endif

#include <Zany80/Plugins.hpp>
#include <liblib/liblib.hpp>
#include <string.h>

#define _t(x) uint ## x ## _t
#define t(x) _t(x)

#ifdef ADDRESS_BUS_SIZE
#define ADDRESS_BUS_SIZE_T t(ADDRESS_BUS_SIZE)
#else
#error ADDRESS_BUS_SIZE must be defined!
#endif

#ifdef DATA_BUS_SIZE
#define DATA_BUS_SIZE_T t(DATA_BUS_SIZE)
#else
#error DATA_BUS_SIZE must be defined!
#endif

#define _s(x) #x
#define s(x) _s(x)

#define _sig(x,y) x ## _ ## y
#define sig(x,y) _sig(x,y)

typedef void(*write_t)(ADDRESS_BUS_SIZE_T,DATA_BUS_SIZE_T);
typedef DATA_BUS_SIZE_T(*read_t)(ADDRESS_BUS_SIZE_T);

PluginType type = Hardware;
HardwareType hw_type = RAM;

extern "C" {
	PluginType *getType();
	HardwareType *getHardwareType();
	bool isSignatureCompatible(const char *sig);
	const char *neededPlugins();
	void postMessage(PluginMessage m);
	void write(ADDRESS_BUS_SIZE_T address,DATA_BUS_SIZE_T value);
	void polywrite(ADDRESS_BUS_SIZE_T address, DATA_BUS_SIZE_T *value_start, ADDRESS_BUS_SIZE_T length);
	DATA_BUS_SIZE_T read(ADDRESS_BUS_SIZE_T address);
}

PluginType *getType(){
	return &type;
}

HardwareType *getHardwareType() {
	return &hw_type;
}

bool isSignatureCompatible(const char *sig) {
	return strcmp(s(sig(ADDRESS_BUS_SIZE,DATA_BUS_SIZE)), sig) == 0;
}

const char *neededPlugins() {
	return NEEDED_PLUGINS;
}

#undef t
#undef _t
