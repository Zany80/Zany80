#include <Zany80/Zany80.hpp>

#include <iostream>
#include <cstring>

#include <stdio.h>  /* defines FILENAME_MAX */
#ifdef _WIN32
	#include <direct.h>
	#define GetCurrentDir _getcwd
#else
	#include <unistd.h>
	#define GetCurrentDir getcwd
#endif

std::vector<liblib::Library*> runner_stack;

std::string absolutize(std::string relativish) {
	size_t pos;
	// /usr/bin/../
	while ((pos = relativish.find("../")) != std::string::npos) {
		if (pos < 3)
			break;
		std::string parent = relativish.substr(0, pos - 3);
		parent = parent.substr(0, parent.rfind("/") + 1);
		std::string child = relativish.substr(pos + 3);
		relativish = parent + child;
	}
	return relativish;
}

void Zany80::replaceRunner(){
	std::cerr << "[Zany80] Invalid runner, attempting recovery...\n";
	((void(*)(liblib::Library*))((*plugin_manager)["removePlugin"]))(runner);
	if (runner_stack.size()) {
		runner = runner_stack.back();
		runner_stack.pop_back();
		((void(*)(PluginMessage))(*this->runner)["postMessage"])({
			0, "activate", (int)strlen("activate"), "Zany80", nullptr
		});
	}
	else if ((runner = (liblib::Library*)(*plugin_manager)["getDefaultRunner"]()) == nullptr) {
		close("Unable to find suitable runner.\n");
	}
	else {
		try {
			((void(*)(PluginMessage))(*this->runner)["postMessage"])({
				0, "activate", (int)strlen("activate"), "Zany80", nullptr
			});
		}
		catch (std::exception &e) {}
	}
}

void Zany80::close(std::string message) {
	std::cerr << "[Crash Handler] "<<message<<
		"[Crash Handler] Notifying user and shutting down...\n";
	sf::Clock c;
	while (c.getElapsedTime().asSeconds() < 5 && window->isOpen()) {
		window->clear(sf::Color(255,0,0));
		// TODO: Render error message
		sf::Event e;
		while (window->pollEvent(e)) {
			switch (e.type) {
				case sf::Event::Closed:
					window->close();
					break;
				default:
					break;
			}
		}
		
		window->display();
	}
	delete this;
	exit(0);
}

void Zany80::close() {
	delete this;
	exit(0);
}

std::string path,folder,true_folder;

typedef void (*init_t)(liblib::Library *);

bool Zany80::attemptLoad(std::string name, liblib::Library **library) {
	if (*library != nullptr) {
		delete *library;
		*library = nullptr;
	}
	try {
		*library = new liblib::Library(folder + name);
		((init_t)(**library)["init"])(*library);
	}
	catch (std::exception &e) {
		if (*library != nullptr) {			
			delete *library;
			*library = nullptr;
		}
		return false;
	}
	return true;
}

Zany80::Zany80(){
	zany = this;
	window = new sf::RenderWindow(sf::VideoMode(LCD_WIDTH,LCD_HEIGHT),"Zany80 IDE");
	window->setFramerateLimit(60);
	if (!font.loadFromFile(folder + "font.png")) {
		// tries a few different options for the path
		if (!font.loadFromFile(absolutize(folder + "/../font.png")) && !font.loadFromFile(absolutize(folder + "../../font.png")) &&
		!font.loadFromFile("font.png") && !font.loadFromFile("../font.png")) {
			std::cerr << "Failed to load font!\n";
			exit(1);
		}
		else {
			// If the font is in this folder, everything else should be as well
			// Note: this only happens when the font is here AND ALSO NOT where it should be
			std::cout << "[Zany80] " << folder << " setup invalid, falling back to ";
			folder =  path.substr(0,path.find_last_of("/")+1);
			std::cout << folder << "\n";
		}
	}
	char *working_directory = new char[FILENAME_MAX];
	if (GetCurrentDir(working_directory, FILENAME_MAX)) {
		true_folder = absolutize(working_directory + (std::string)"/" + folder);
	}
	else {
		exit(1);
	}
	if (!attemptLoad("plugins/plugin_manager",&plugin_manager)) {
		close("Error loading plugin manager!\n");
	}
	try {
		if ((std::vector<std::string>*)(*plugin_manager)["enumerate_plugins"]() == nullptr) {
			close("No plugins found! This is probably a problem with the installation.\n");
		}
	}
	catch (std::exception &e) {
		close("Error detecting plugins!\n");
	}
	// At this point, all detected plugins have been validated. In addition, there is at minimum one plugin.
	// Get default runner from the plugin manager. The plugin manager decides what runner to choose
	try {
		if ((runner = (liblib::Library*)(*plugin_manager)["getDefaultRunner"]()) == nullptr) {
			close("Unable to find suitable runner.\n");
		}
	}
	catch (std::exception &e) {
		close("Invalid plugin manager.\n");
	}
}

Zany80::~Zany80(){
	delete window;
	try {
		(*plugin_manager)["cleanup"]();
	}
	catch (std::exception &e) {
		std::cerr << "[Crash handler] Error cleaning up plugins\n";
	}
	delete plugin_manager;
}

void Zany80::run(){
	while (window->isOpen()) {
		frame();
	}
}

void Zany80::popRunner() {
	// Deactivate the current runner
	if (this->runner != nullptr) {
		try {
			((void(*)(PluginMessage))(*this->runner)["postMessage"])({
				0, "deactivate", (int)strlen("deactivate"), "Zany80", nullptr
			});
		}
		catch (...){}
		this->runner = nullptr;
	}
	// Pop a runner from the stack. If no runners remain, GET one!
	if (runner_stack.size()) {
		// This won't add anything to the stack, as this->runner is already nullptr
		this->pushRunner(runner_stack.back());
		runner_stack.pop_back();
	}
	else {
		// This won't add anything to the stack, as this->runner is already nullptr
		this->pushRunner((liblib::Library *)(*plugin_manager)["getDefaultRunner"]());
	}
}

void Zany80::frame(){
	sf::Event e;
	while (window->pollEvent(e)) {
		switch (e.type) {
			case sf::Event::Closed:
				close();
				break;
			case sf::Event::KeyPressed:
				if (e.key.code == sf::Keyboard::Escape
						&& e.key.control) {
					this->pushRunner((liblib::Library *)(*plugin_manager)["getDefaultRunner"]());
					return;
				}
			default:
				try {
					((void(*)(sf::Event&))((*runner)["event"]))(e);
				}
				catch (std::exception) {
					replaceRunner();
				}
				break;
		}
	}
	try {
		(*runner)["run"]();
	}
	catch (std::exception) {
		replaceRunner();
	}
	window->display();
}

void Zany80::pushRunner(liblib::Library *runner) {
	if (this->runner != nullptr) {
		try {
			((void(*)(PluginMessage))(*this->runner)["postMessage"])({
				0, "deactivate", (int)strlen("deactivate"), "Zany80", nullptr
			});
		}
		catch (...){}
		runner_stack.push_back(this->runner);
	}
	if (runner != nullptr) {
		this->runner = runner;
		((void(*)(PluginMessage))(*this->runner)["postMessage"])({
			0, "activate", (int)strlen("activate"), "Zany80", nullptr
		});
	}
}

Zany80 *zany;
