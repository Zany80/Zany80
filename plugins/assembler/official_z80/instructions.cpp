#include <instructions.hpp>

#include <Zany80/GenericException.hpp>
#include <iostream>

Instruction::Instruction(std::string file, int line, InstructionType type) {
	this->type = type;
	this->file = file;
	this->line = line;
}

Instruction::Instruction(std::string file, int line, InstructionType type, Argument arg1, Argument arg2) {
	this->type = type;
	this->arg1 = arg1;
	this->arg2 = arg2;
	this->file = file;
	this->line = line;
}

std::ostream& operator<<(std::ostream &stream, const Instruction& i) {
	stream << i.file << std::endl << i.line << " ";
	stream << (int)i.type;
	switch (i.type) {
		case LDR8R8:
			stream << ' '<<(int)i.arg1.r8 << ' ' << (int)i.arg2.r8;
			break;
		case META:
			stream << ' ' << (int)i.arg1.m << ' ' << i.arg2.i;
		default:
			break;
	}
	return stream;
}

std::istream& operator>>(std::istream &stream, Instruction& i) {
	std::getline(stream, i.file);
	int _i;
	stream >> _i;
	i.type = (InstructionType)_i;
	switch (i.type) {
		case LDR8R8:
			stream >> _i;
			i.arg1.r8 = (Register8)_i;
			stream >> _i;
			i.arg2.r8 = (Register8)_i;
			break;
		case META:
			stream >> _i;
			i.arg1.m = (MetaInstruction)_i;
			stream >> _i;
			i.arg2.i = _i;
		default:
			break;
	}
	return stream;
}
