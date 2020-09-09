#ifndef Z80SIM_H
#define Z80SIM_H

#include <iostream>
#include <fstream>

#include "z80.h"
#include "z80operations.h"

class Z80sim : public Z80operations
{
private:
    uint64_t tstates;
    Z80 cpu;
    uint8_t z80Ram[0x10000];
    uint8_t z80Ports[0x10000];
    bool finish;

public:
    Z80sim(void);
    virtual ~Z80sim() override;

    uint8_t fetchOpcode(uint16_t address) override;
    uint8_t peek8(uint16_t address) override;
    void poke8(uint16_t address, uint8_t value) override;
    uint16_t peek16(uint16_t address) override;
    void poke16(uint16_t address, uint16_t word) override;
    uint8_t inPort(uint16_t port) override;
    void outPort(uint16_t port, uint8_t value) override;
    void addressOnBus(uint16_t address, int32_t tstates) override;
    void interruptHandlingTime(int32_t tstates) override;
    bool isActiveINT(void) override;
    uint8_t breakpoint(uint16_t address, uint8_t opcode) override;
    void execDone(void) override;

    void runTest(std::ifstream* f);
};
#endif // Z80SIM_H
