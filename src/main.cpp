#include <Zany80.hpp>
#include <Plugins.hpp>

#include <SFML/Graphics.hpp>
#include <iostream>

void Zany80::close(std::string message) {
	std::cerr << "[Crash handler] "<<message<<
		"[Crash handler] Notifying user and shutting down...\n";
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
			}
		}
		
		window->display();
	}
	exit(0);
}

void Zany80::close() {
	exit(1);
}

std::string path,folder;

bool attemptLoad(std::string name, liblib::Library **library) {
	try {
		*library = new liblib::Library(folder + name);
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
	return zany->run();
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
	if (!attemptLoad("plugins/plugin_manager", &plugins["plugin_manager"])) {
		close("Error loading plugin manager!\n");
	}
	std::vector<std::string> * plugin_paths;
	try {
		plugin_paths = (std::vector<std::string>*)(*plugins["plugin_manager"])["enumerate_plugins"]();
	}
	catch (liblib::SymbolLoadingException &e) {
		close("Error enumerating plugins!\n");
	}
	if (plugin_paths == nullptr) {
		close("No plugins found! This is probably a problem with the installation.\n");
	}
	else {
		runners = new std::vector<liblib::Library*>;
		bool runnerFound = false;
		for (std::string s : *plugin_paths) {
			std::cout << "[Plugin Loader] "<<"Loading \""<<s<<"\"...\n";
			if (attemptLoad(s,&plugins[s])) {
				try {
					if (*(PluginType*)(*plugins[s])["getType"]() == Runner) {	
						runnerFound = true;
						runners->push_back(plugins[s]);
						if (*(RunnerType*)(*plugins[s])["getRunnerType"]() == Shell && this->runner == nullptr) {
							this->runner = plugins[s];
						}
					}
				}
				catch (std::exception e) {
					delete plugins[s];
					std::cerr << "[Plugin Loader] Invalid plugin: "<<s<<"\n";
				}
			}
			else {
				std::cerr << '\t'<<"[Plugin Loader] "<<"Error!\n";
			}
		}
		if (runnerFound) {
			if (this->runner == nullptr) {
				this->runner = (*runners)[0];
			}
		}
		else {
			close("No runners found!\n");
		}
	}
}

Zany80::~Zany80(){
	delete window;
	if (runners != nullptr) {
		delete runners;
	}
	for (auto pair : plugins) {
		liblib::Library *library = pair.second;
		try {
			(*library)["cleanup"]();
		}
		catch (std::exception e) {
			
		}
		delete library;
	}
}

int Zany80::run(){
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
				//if (tool != nullptr)
					//tool->event(e);
				break;
		}
	}
	((*runner)["run"])();
	window->display();
}

Zany80 * zany;
