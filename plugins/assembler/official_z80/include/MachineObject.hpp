#pragma once

#include <instructions.hpp>

class MachineObject {
public:
	friend std::ostream& operator<<(std::ostream &stream, const MachineObject& o);
	friend std::istream& operator>>(std::istream &stream, MachineObject& o);
	void addInstruction(Instruction i);
	std::vector<Instruction> instructions;
};

std::ostream& operator<<(std::ostream &stream, const MachineObject& o);
std::istream& operator>>(std::istream &stream, MachineObject& o);
