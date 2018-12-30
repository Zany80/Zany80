#pragma once

#include <Zany80/Plugin.h>

class Display : public PerpetualPlugin {
	OryolClassDecl(Display);
	OryolTypeDecl(Display, PerpetualPlugin);
public:
	Display();
	virtual ~Display() {}
	virtual void frame(float delta);
	virtual bool supports(String type);
	virtual void* named_function(String functionName, ...);
private:
	bool connected = false;
	StringBuilder output_buffer;
	Array<char> input_buffer;
	CPUPlugin *cpu;
};
