#pragma once

#include <ostream>
#include <istream>

typedef enum {
	META,
	NOP, // nop
	LDR8R8 // ld r, r'
} InstructionType;

typedef enum {
	SET_PC
} MetaInstruction;

typedef enum {
	A, B, C, D, E, H, L
} Register8;

typedef enum {
	AF, BC, DE, HL
} Register16;

typedef const char * Symbol;

typedef union {
	Register8 r8;
	Register16 r16;
	Symbol s;
	MetaInstruction m;
	int i;
} Argument;

class Instruction {
friend std::ostream& operator<<(std::ostream &stream, const Instruction& i);
friend std::istream& operator>>(std::istream &stream, Instruction& i);
public:
	Instruction() : Instruction("", 0){}
	Instruction(std::string file, int line) : Instruction(file, line, NOP){}
	Instruction(std::string file, int line, InstructionType type);
	Instruction(std::string file, int line, InstructionType type, Argument arg1, Argument arg2);
private:
	InstructionType type;
	Argument arg1, arg2;
	std::string file;
	int line;
};

std::ostream& operator<<(std::ostream& stream, const Instruction& i);
std::istream& operator>>(std::istream& stream, Instruction& i);
