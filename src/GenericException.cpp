#include <Zany80/GenericException.hpp>

const char *GenericException::what() {
	return s.c_str();
}

GenericException::GenericException(std::string s) {
	this->s = s;
}

GenericException::~GenericException(){}
