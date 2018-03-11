#pragma once

#include <instructions.hpp>

#include <iostream>
#include <vector>
#include <string>

extern "C" {
	bool isCategory(const char *);
	bool isType(const char *);
}

void run(std::vector<std::string> *args);
void link(std::vector<std::string> *args);
void messageShell(const char *message);
void assemble(std::string source, std::string destination);
int parseNumber(std::string &s);
void tokenizeLine(std::string line, std::vector<std::string> &tokens);
std::string workingDirectory();

template<class T>
std::ostream &operator<<(std::ostream &stream, const std::vector<T> &v) {
	stream << v.size() << ' ';
	for (T t : v)
		stream << t << ' ';
	return stream;
}

template<class T>
std::istream &operator>>(std::istream &stream, std::vector<T> &v) {
	int size;
	stream >> size;
	v.resize(size);
	for (int i = 0; i < size; i++) {
		stream >> v[i];
	}
}

// Has to be declared here or the above template will be used, resulting in
// incorrect stream parsing which can lead to allocation of >5GB!
std::istream &operator>>(std::istream &stream, std::vector<std::string> &v);

typedef enum {
	normal, character, string
} buffer_position;
