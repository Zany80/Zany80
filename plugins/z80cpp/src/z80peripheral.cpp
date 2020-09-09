#include "z80.h"

Z80peripheral::Z80peripheral(Z80 *cpu, Z80peripheral *next_device) {
	this->next_device = next_device;
	this->IEI = new bool;
	*this->IEI = true;
	if (this->next_device) {
		this->next_device->previous_device = this;
		this->IEO = this->next_device->IEI;
	}
	else {
		this->IEO = new bool;
		*this->IEO = true;
		cpu->closest_peripheral = this;
	}
	this->cpu = cpu;
}

Z80peripheral::~Z80peripheral() {
	delete this->IEI;
	this->IEI = nullptr;
	if (!this->next_device) {
		delete this->IEO;
	}
	this->IEO = nullptr;
}

bool Z80peripheral::isActiveINT() {
	return !*IEO;
}

void Z80peripheral::acknowledgeInterrupt() {
	if (*this->IEO) {
		// Error!
		return;
	}
	if (*this->IEI) {
		this->cpu->data = this->data;
		*this->IEO = true;
		this->interrupting = false;
	}
	else {
		if (this->previous_device) {
			*((uint8_t*)0)=0;
			this->previous_device->acknowledgeInterrupt();
		}
	}
}

void Z80peripheral::propagate() {
	*this->IEO = *this->IEI ? !interrupting : false;
	if (this->next_device) {
		this->next_device->propagate();
	}
}

void Z80peripheral::interrupt(uint8_t data) {
	this->interrupting = true;
	this->data = data;
	*this->IEO = false;
	if (this->next_device) {
		this->next_device->propagate();
	}
}

uint8_t Z80peripheral::getData() {
	/* 
	 * Note: this uint8_t exists for a reason! If this peripheral isn't active,
	 * but is the first one (previous_device == nullptr), then data is undefined
	 */
	uint8_t data;
	if (*this->IEI && !this->IEO) {
		data = this->data;
	}
	else if (this->previous_device) {
		data = this->previous_device->getData();
	}
	return data;
}
