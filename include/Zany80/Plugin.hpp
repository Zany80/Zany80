#pragma once

#include <string>

class Plugin {
public:
	virtual ~Plugin(){};
	virtual std::string getName() = 0;
	virtual std::string getUpdateUrl() = 0;
	virtual std::string getDescription() = 0;
	virtual std::string getPath() = 0;
	virtual void loadIntoRuntime() = 0;
};
