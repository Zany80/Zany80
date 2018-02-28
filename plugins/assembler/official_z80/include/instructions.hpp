#pragma once

#include <ostream>

typedef enum {
	NOP
} InstructionType;

class Instruction {
public:
	Instruction();
private:
	InstructionType type;
};

std::ostream& operator<<(std::ostream& stream, const Instruction& i);
std::istream& operator>>(std::istream& stream, Instruction& i);
