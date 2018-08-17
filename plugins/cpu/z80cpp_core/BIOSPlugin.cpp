#include "BIOSPlugin.hpp"

BIOSPlugin::BIOSPlugin() : BIOSPlugin(nullptr) {}
BIOSPlugin::BIOSPlugin(liblib::Library *BIOS) {
	this->initialize(BIOS);
	this->ROM_size = -1;
}

BIOSPlugin::~BIOSPlugin(){}

void BIOSPlugin::initialize(liblib::Library *BIOS) {
	this->BIOSLibrary = BIOS;
}

function BIOSPlugin::operator[](const char *func) {
	return (*this->BIOSLibrary)[func];
}

uint16_t BIOSPlugin::size() {
	if (this->ROM_size == -1) {
		this->ROM_size = ((uint16_t(*)())(*this)["getSize"])();
	}
	return this->ROM_size;
}

uint8_t BIOSPlugin::operator[](uint16_t addr) {
	return ((uint8_t(*)(uint16_t))(*this)["read"])(addr);
}
