#define NEEDED_PLUGINS "CPU/z80;RAM/16_8"
#include <Zany80/Runner.hpp>

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>

#include <SFML/System.hpp>

typedef void (*post_t)(PluginMessage m);

//Run at 10Hz for now. This makes output smaller and easier to debug during emulator implementation.
#define SPEED 10

RunnerType runner_type = ROMRunner;
liblib::Library *z80, *ram;
liblib::Library *plugin_manager;
void (*emulate)(uint64_t cycles);

uint8_t *ROM = nullptr;

typedef struct {
	union {
		struct {
			uint8_t zany[4];
			uint16_t virtual_title;
			uint16_t PC;
		};
		uint8_t data[0x10000];
	};
} ROMMetadata;

float to_run;
// timer is used to calculate how many cycles have passes, precision is needed to determine clock precision
sf::Clock timer, precision;
float time_passed;
bool activated;

void postMessage(PluginMessage m) {
	if (!strcmp(m.data, "boot_flash")) {
		if (ROM != nullptr) {
			std::cout << "[ROM Runner] Booting up, flashing RAM...\n";
			try {
				((void(*)(uint16_t,uint8_t *,uint16_t))(*ram)["polywrite"])(0,ROM,0xFFFF);
				std::cout << "[ROM Runner] RAM flashed successfully!\n";
			}
			catch (std::exception) {
				std::cout << "[ROM Runner] Error flashing RAM!\n";
			}
		}
		else {
			std::cout << "[ROM Runner] No flash detected, unable to flash RAM!\n";
			try {
				((textMessage_t)(*plugin_manager)["textMessage"])("no_rom",((std::string)"Runner/ROM;"+m.source).c_str());
			}
			catch (std::exception){}
		}
	}
	else if (!strcmp(m.data, "deactivate")) {
		time_passed += precision.restart().asSeconds();
		activated = false;
	}
	else if(!strcmp(m.data, "init")) {
		init((liblib::Library*)m.context);
	}
	else if(!strcmp(m.data, "cleanup")) {
		if (ROM != nullptr) {
			delete[] ROM;
			ROM = nullptr;
		}
		if (activated) {
			time_passed += precision.restart().asSeconds();
		}
		uint64_t cycles = (uint64_t)((*z80)["getCycles"]());
		uint64_t target = SPEED * time_passed;
		z80 = nullptr;
		if (cycles == 0)
			return;
		std::cout << "[Rom Runner] After "<<time_passed<<" seconds: Cycles: "<<cycles<<"; Target: "<<target<<"\n";
		std::cout << "[ROM Runner] CCA: "<<100*(double)cycles/(double)target<< "%\n";
	}
}

void init(liblib::Library *pm) {
	plugin_manager = pm;
	to_run = 0;
	activated = false;
	time_passed = 0;
	std::cout << "[ROM Runner] Retrieving z80 plugin...\n";
	try {
		z80 = ((liblib::Library*(*)(const char *))((*plugin_manager)["getCPU"]))("z80");
		if (z80 == nullptr) {
			throw std::exception();
		}
		ram = ((liblib::Library *(*)(const char *))(*plugin_manager)["getRAM"])("16_8");
		if (ram == nullptr) {
			throw std::exception();
		}
		((void (*)(liblib::Library*))((*z80)["setRAM"]))(ram);
		((textMessage_t)(*plugin_manager)["textMessage"])("reset","Runner/ROM;CPU/z80");
		emulate=(void(*)(uint64_t))((*z80)["emulate"]);
	}
	catch (liblib::SymbolLoadingException) {
		throw std::exception();
	}
}

bool loadROM(const char *path) {
	if (ROM != nullptr) {
		delete[] ROM;
	}
	std::ifstream ROMFile(path,std::ios::binary | std::ios::ate);
	if (!ROMFile.is_open()) {
		return false;
	}
	int size = ROMFile.tellg();
	ROM = new uint8_t[0x10000]();
	ROMFile.seekg(0);
	ROMFile.read((char*)ROM,size);
	ROMFile.close();
	return true;
}

void run() {
	to_run += SPEED * timer.restart().asSeconds();
	try {
		emulate((uint64_t)to_run);
	}
	catch (std::exception e) {
		std::cerr << "[ROM Runner] Emulation error!\n";
		((message_t)(*plugin_manager)["message"])({
			0, (char *)"history", (int)strlen("history"), "Runner/ROM", "[ROM Runner] Emulation Error!"
		}, "Runner/Shell");
		throw e;
	}
	to_run -= (uint64_t)to_run;
}

int isROMValid(const char *path) {
	std::ifstream ROMFile(path,std::ios::binary | std::ios::ate);
	if (!ROMFile.is_open()) {
		return -1;
	}
	int size = ROMFile.tellg();
	uint8_t *data = new uint8_t[size];
	ROMFile.seekg(0);
	ROMFile.read((char*)data,size);
	ROMFile.close();
	// Proper Zany ROMs have the first four characters as ASCII "ZANY" (no null-terminator).
	bool valid = strncmp((const char *)data, "ZANY",4) == 0;
	delete[] data;
	return valid ? 1 : 0;
}

void event(sf::Event &e) {
	switch (e.type) {
		case sf::Event::KeyPressed:
			if (e.key.code == sf::Keyboard::Escape) {
				((void(*)(RunnerType,const char *))(*plugin_manager)["activateRunner"])(Shell,"");
			}
			break;
	}
}

bool activate(const char *arg) {
	timer.restart();
	precision.restart();
	activated = true;
	int valid = isROMValid(arg);
	switch (valid) {
	case -1:
		// not even a file
		((message_t)(*plugin_manager)["message"])({
			0, (char *)"history", (int)strlen("history"), "Runner/ROM", ((std::string)"No such file: "+arg).c_str()
		}, "Runner/Shell");
		break;
	case 0:
		((message_t)(*plugin_manager)["message"])({
			0, (char *)"history", (int)strlen("history"), "Runner/ROM", ((std::string)"Invalid ROM: "+arg).c_str()
		}, "Runner/Shell");
		break;
	case 1:
		if (loadROM(arg)) {
			// valid ROM! yay!
			((textMessage_t)(*plugin_manager)["textMessage"])("reset","Runner/ROM;CPU/z80");
			// TODO: have ZanyOS (running in the z80 CPU) boot up the ROM instead
			((message_t)(*plugin_manager)["message"])({
				0, (char *)"setPC", (int)strlen("setPC"), "Runner/ROM", (char*)&((ROMMetadata*)ROM)->PC
			}, "CPU/z80");
			return true;
		}
		else {
			((message_t)(*plugin_manager)["message"])({
				0, (char *)"history", (int)strlen("history"), "Runner/ROM", "Unable to load valid ROM. Weird."
			}, "Runner/Shell");
		}
		break;
	}
	return false;
}
