#pragma once

#include <Tool.hpp>
#include <TGUI/TGUI.hpp>
#include <Text.hpp>

#include <map>
#include <string>
#include <vector>

class Shell;
class Zany80;

typedef void(*function)(Shell *shell,std::vector<std::string> arguments);


class Shell : public Tool {
public:
	Shell(tgui::Canvas::Ptr canvas);
	~Shell();
	virtual void run();
	virtual void event(sf::Event k);
	void addToHistory(std::string history_item);
private:
	std::map<std::string,function> commands;
	inline void map_commands();
	tgui::Canvas::Ptr canvas;
	Text command, history;
};
