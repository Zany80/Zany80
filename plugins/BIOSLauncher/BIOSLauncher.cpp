#define NEEDED_PLUGINS "CPU/z80;Hardware/MMU;Runner/Shell"
#include <Zany80/Runner.hpp>
#include <Zany80/ClockSpeed.hpp>

#include <SFML/System.hpp>

#include <cstring>
#include <fstream>
#include <iostream>

#define SPEED 4 MHz

RunnerType runner_type = ROMRunner;
liblib::Library *z80, *ram;
liblib::Library *plugin_manager;
void (*emulate)(uint64_t cycles);

uint8_t *ROM = nullptr;
uint8_t *stack = nullptr;

typedef struct __attribute__((__packed__)) {
	uint8_t banks[3][0x4000];
} BIOS_ROM_t;

sf::Clock timer, precision;
float time_passed;
bool activated;

const uint8_t vsync_reset = 0xD7;
const uint8_t key_reset = 0xDF;

void run() {
	((message_t)(*plugin_manager)["message"])({
		0, "interrupt", (int)strlen("interrupt"), "Runner/BIOSLauncher", (char*)&vsync_reset
	}, "CPU/z80");
	static float to_run = 0;
	to_run += SPEED * timer.restart().asSeconds();
	try {
		emulate((uint64_t)to_run);
	}
	catch (std::exception e) {
		((message_t)(*plugin_manager)["message"])({
			0, "history", (int)strlen("history"), "Runner/BIOSLauncher", "[ROM Runner] Emulation Error!"
		}, "Runner/Shell");
		throw e;
	}
	to_run -= (uint64_t)to_run;
}

void event(sf::Event &e) {
	switch (e.type) {
		case sf::Event::KeyPressed:
			switch (e.key.code){
				case sf::Keyboard::Escape:
					((void(*)(RunnerType,const char *))(*plugin_manager)["activateRunner"])(Shell,"");
					break;
				case sf::Keyboard::S:
				case sf::Keyboard::A:
				case sf::Keyboard::Z:
				case sf::Keyboard::X:
					((message_t)(*plugin_manager)["message"])({
						0, "interrupt", (int)strlen("interrupt"), "Runner/BIOSLauncher", (char*)&key_reset
					}, "CPU/z80");
					break;
			}
			break;
	}
}

bool activate(const char *arg) {
	((textMessage_t)(*plugin_manager)["textMessage"])("unhalt","Runner/BIOSLauncher;CPU/z80");
	return ROM != nullptr;
}

void postMessage(PluginMessage m) {
	if (!strcmp(m.data, "deactivate")) {
		time_passed += precision.restart().asSeconds();
		activated = false;
	}
	else if(!strcmp(m.data, "init")) {
		plugin_manager = (liblib::Library*)m.context;
		activated = false;
		time_passed = 0;
		try {
			liblib::Library*(*getCPU)(const char *) = (liblib::Library*(*)(const char *))((*plugin_manager)["getCPU"]);
			z80 = getCPU("z80");
			if (z80 == nullptr) {
				throw std::exception();
			}
			ram = ((liblib::Library *(*)(const char *))(*plugin_manager)["getRAM"])("16_8");
			if (ram == nullptr) {
				throw std::exception();
			}
			((void (*)(liblib::Library*))((*z80)["setRAM"]))(ram);
			emulate=(void(*)(uint64_t))((*z80)["emulate"]);
			// Load in the BIOS
			std::ifstream BIOS(true_folder + "/BIOS.rom", std::ios::binary | std::ios::ate);
			if (BIOS.is_open()) {
				int size = BIOS.tellg();
				BIOS.seekg(0);
				if (size > sizeof(BIOS_ROM_t)) {
					size = sizeof(BIOS_ROM_t);
				}
				std::cout << sizeof(BIOS_ROM_t)<<'\n';
				if (ROM != nullptr) {
					delete[] ROM;
				}
				ROM = new uint8_t[size];
				BIOS.read((char*)ROM, size);
				BIOS.close();
				((message_t)(*plugin_manager)["message"])({
					0, "history", (int)strlen("history"), "Runner/BIOSLauncher", "[BIOSLauncher] BIOS loaded successfully!"
				}, "Runner/Shell");
			}
			else {
				((message_t)(*plugin_manager)["message"])({
					0, "history", (int)strlen("history"), "Runner/BIOSLauncher", "[BIOSLauncher] Error loading BIOS!"
				}, "Runner/Shell");
			}
			for (int i = 0; i < 3; i++) {
				((message_t)(*plugin_manager)["message"])({
					i, "map_bank", (int)strlen("map_bank"), "Runner/BIOSLauncher", (char*)ROM + 0x4000 * i
				}, "Hardware/MMU");
			}
			((textMessage_t)(*plugin_manager)["textMessage"])("reset","Runner/BIOSLauncher;CPU/z80");
		}
		catch (liblib::SymbolLoadingException) {
			throw std::exception();
		}
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
