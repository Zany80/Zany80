#pragma once

class Z80;
class Z80peripheral {
public:
	friend class Z80;
	Z80peripheral(Z80 *cpu, Z80peripheral *next_device);
	virtual ~Z80peripheral();
	bool isActiveINT();
	uint8_t getData();
protected:
	void interrupt(uint8_t data = 0);
private:
	void acknowledgeInterrupt();
	void propagate();
	Z80peripheral *next_device, *previous_device;
	Z80 *cpu;
	bool *IEI, *IEO;
	bool interrupting;
	uint8_t data;
};
