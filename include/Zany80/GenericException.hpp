#pragma once

#include <exception>
#include <string>

class GenericException : public std::exception {
public:
	GenericException(std::string s);
	virtual ~GenericException();
	virtual const char *what();
private:
	std::string s;
};
