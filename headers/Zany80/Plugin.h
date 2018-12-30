#pragma once

#include <Core/Core.h>
#include <Core/RefCounted.h>
#include <Core/String/String.h>
#include <Core/String/StringBuilder.h>
#include <Core/Containers/Map.h>
#include <IO/IOTypes.h>
using namespace Oryol;

#define ORYOL_PPTRS
#define FORCE_DC_PPTR
#define INHERIT_TYPE public virtual

#define DC_CAST_PPTR(PTR, TYPE) dynamic_cast<TYPE*>(GET_PPTR(PTR))

#ifdef ORYOL_PPTRS
#define PPTR Ptr<Plugin>
#define GET_PPTR(P) P.p
#define CLEANUP_PPTR(P)
#define CAST_PPTR(PTR, TYPE) (PTR->IsA<TYPE>() ? GET_PPTR(PTR->DynamicCast<TYPE>()) : nullptr)
#else
#define PPTR Plugin*
#define GET_PPTR(P) P
#define CLEANUP_PPTR(P) delete P;P = nullptr
#define CAST_PPTR(PTR, TYPE) DC_CAST_PPTR(PTR, TYPE)
#endif

#ifdef FORCE_DC_PPTR
#undef CAST_PPTR
#define CAST_PPTR(PTR, TYPE) DC_CAST_PPTR(PTR, TYPE)
#endif

class Plugin : public RefCounted {
	OryolClassDecl(Plugin);
	OryolBaseTypeDecl(Plugin);
public:
	virtual ~Plugin(){}
	virtual bool supports(String type) = 0;
	/** 
	 * This function can be used to call known functions without casts.
	 * Its primary purpose is future-compatibility - using this function allows
	 * a newer plugin to be added to an older host.
	 * A default implementation exists mostly to ease the burden on plugins.
	 */
	virtual void* named_function(String functionName, ...) {
		if (functionName == "supports") {
			va_list args;
			va_start(args, functionName);
			bool supported = this->supports(va_arg(args, const char *));
			va_end(args);
			return (void*)supported;
		}
		return 0;
	}
};

class PerpetualPlugin : INHERIT_TYPE Plugin {
	OryolClassDecl(PerpetualPlugin);
	OryolTypeDecl(PerpetualPlugin, Plugin);
public:
	virtual ~PerpetualPlugin(){}
	virtual void frame(float delta) = 0;
};

class ShellPlugin : INHERIT_TYPE Plugin {
	OryolClassDecl(ShellPlugin);
	OryolTypeDecl(ShellPlugin, Plugin);
public:
	virtual ~ShellPlugin(){}
	virtual void output(const char *fmt, ...) __attribute__((format(printf, 2, 3))) = 0;
	virtual int execute(String command) = 0;
	virtual Array<String> getCommands() = 0;
};

class CPUPlugin;
typedef std::function<uint8_t()> read_handler_t;
typedef std::function<void(uint8_t)> write_handler_t;

class CPUPlugin : INHERIT_TYPE Plugin {
	OryolClassDecl(CPUPlugin);
	OryolTypeDecl(CPUPlugin, Plugin);
public:
	virtual ~CPUPlugin(){}
	virtual Map<String, uint32_t> getRegisters() = 0;
	virtual uint64_t executedCycles() = 0;
	virtual void execute(uint32_t cycles) = 0;
	virtual Array<uint32_t> getBreakpoints() = 0;
	virtual void setBreakpoint(uint32_t address, bool active) = 0;
	virtual void attachToPort(uint32_t port, read_handler_t read_handler) = 0;
	virtual void attachToPort(uint32_t port, write_handler_t write_handler) = 0;
	virtual void fireInterrupt(uint8_t IRQ) = 0;
	virtual void loadROM(String path) = 0;
	virtual void reset() = 0;
};

class ToolchainPlugin : INHERIT_TYPE Plugin {
	OryolClassDecl(ToolchainPlugin);
	OryolTypeDecl(ToolchainPlugin, Plugin);
public:
	virtual ~ToolchainPlugin() {}
	virtual Map<String, Array<String>> supportedTransforms() = 0;
	virtual int transform(Array<String> sources, String destination, StringBuilder *out) = 0;
	virtual void addIncludeDirectory(String dir) = 0;
	virtual String getChain() = 0;
	virtual void setVerbosity(int verbosity) = 0;
};

extern Map<String, PPTR> plugins;

#ifndef ORYOL_EMSCRIPTEN
void spawnPlugin(String plugin);
#endif
