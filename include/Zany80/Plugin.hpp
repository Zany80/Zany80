#pragma once

#include <string>
#include <vector>

#define NOTIF_UNACCEPTED	"Plugin doesn't accept notifications."
#define NOTIF_ERROR 		"Error sending message to plugin."

class Plugin {
public:
	virtual ~Plugin(){};
	virtual std::string getName() = 0;
	virtual std::string getUpdateUrl() = 0;
	virtual std::string getDescription() = 0;
	virtual std::string getPath() = 0;
	virtual bool loadIntoRuntime() = 0;
	virtual bool provides(std::string cap) = 0;
	virtual std::string notify(Plugin *source, std::string message) = 0;
};
