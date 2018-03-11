#include <assembler.hpp>
#include <MachineObject.hpp>

void MachineObject::addInstruction(Instruction i) {
	instructions.push_back(i);
}

std::ostream& operator<<(std::ostream &stream, const MachineObject& o) {
	stream << o.instructions;
	return stream;
}

std::istream& operator>>(std::istream &stream, MachineObject& o) {
	stream >> o.instructions;
	return stream;
}
