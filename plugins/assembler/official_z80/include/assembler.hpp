#pragma once

#define NEEDED_PLUGINS ""
#include <Zany80/Generic.hpp>

#include <instructions.hpp>

#include <iostream>

extern "C" {
	bool isCategory(const char *);
	bool isType(const char *);
}

void run(std::vector<std::string> *args);
void messageShell(const char *message);
void assemble(std::string source, std::string destination);
int parseNumber(std::string &s);

template<class T>
std::ostream &operator<<(std::ostream &stream, const std::vector<T> &v) {
	stream << v.size() << '\n';
	for (T t : v)
		stream << t << '\n';
	return stream;
}

std::istream &operator>>(std::istream &stream, std::vector<std::string> &v) {
	int size;
	stream >> size;
	v.resize(size);
	std::string buffer;
	// discard remainder of first line - simpler than specializing std::ostream
	// std::vector<std::string>.
	std:;getline(stream, buffer);
	for (int i = 0; i < size; i++) {
		std::getline(stream, buffer);
		v[i] = buffer;
	}
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

class MachineObject {
public:
	explicit MachineObject();
	std::vector<std::string> source_lines;
	std::vector<std::string> tokens;
	std::vector<int> mapped_tokens;
	std::vector<Instruction> instructions;
};

typedef enum {
	normal, character, string
} buffer_position;

std::ostream& operator<<(std::ostream &stream, const MachineObject& o);
std::istream& operator>>(std::istream &stream, MachineObject& o);
