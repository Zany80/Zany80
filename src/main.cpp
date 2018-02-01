#include <Zany80/Zany80.hpp>
#include <Zany80/Plugins.hpp>

#include <SFML/Graphics.hpp>
#include <iostream>

void Zany80::replaceRunner(){
	std::cerr << "[Zany80] Invalid runner, requesting replacement from plugin manager...\n";
	((void(*)(liblib::Library*))((*plugin_manager)["removePlugin"]))(runner);
	if ((runner = (liblib::Library*)(*plugin_manager)["getDefaultRunner"]()) == nullptr) {
		close("Unable to find suitable runner.\n");
	}
	else {
		try {
			(*runner)["activate"]();
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

std::string path,folder;

typedef void (*init_t)(liblib::Library *);

bool Zany80::attemptLoad(std::string name, liblib::Library **library) {
	try {
		*library = new liblib::Library(folder + name);
		((init_t)(**library)["init"])(*library);
	}
	catch (std::exception &e) {
		return false;
	}
	return true;
}

int main(int argc, const char **argv){
	path = argv[0];
	folder =  path.substr(0,path.find_last_of("/")+1);
	#ifdef linux
	folder += "../share/zany80/";
	#endif
	zany = new Zany80();
	zany->run();
	return 0;
}

Zany80::Zany80(){
	window = new sf::RenderWindow(sf::VideoMode(LCD_WIDTH,LCD_HEIGHT),"Zany80 IDE");
	if (!font.loadFromFile(folder + "font.png")) {
		// hack for development
		if (!font.loadFromFile("font.png")) {
			std::cerr << "Failed to load font!\n";
			exit(1);
		}
		else {
			// If the font is in this folder, everything lse should be as well
			// Note: this only happens when the font is here AND ALSO NOT where it should be
			std::cout << "[Zany80] " << folder << " setup invalid, falling back to ";
			folder =  path.substr(0,path.find_last_of("/")+1);
			std::cout << folder << "\n";
		}
	}
	if (!attemptLoad("plugins/plugin_manager",&plugin_manager)) {
		close("Error loading plugin manager!\n");
	}
	try {
		if ((std::vector<std::string>*)(*plugin_manager)["enumerate_plugins"]() == nullptr) {
			close("No plugins found! This is probably a problem with the installation.\n");
		}
	}
	catch (liblib::SymbolLoadingException &e) {
		close("Error detecting plugins!\n");
	}
	// At this point, all detected plugins have been validated. In addition, there is at minimum one plugin.
	// Get default runner from the plugin manager. The plugin manager decides what runner to choose
	try {
		if ((runner = (liblib::Library*)(*plugin_manager)["getDefaultRunner"]()) == nullptr) {
			close("Unable to find suitable runner.\n");
		}
		else {
			try {
				(*runner)["activate"]();
			}
			catch (std::exception &e) {}
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

void Zany80::frame(){
	sf::Event e;
	while (window->pollEvent(e)) {
		switch (e.type) {
			case sf::Event::Closed:
				close();
				break;
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

Zany80 * zany;
