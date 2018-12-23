#pragma once

#include <Core/Core.h>
#include <Core/Creator.h>
#include <Core/String/String.h>
#include <Core/String/StringBuilder.h>
#include <Core/Containers/Array.h>
#include <Core/Containers/Map.h>
#include <IMUI/IMUI.h>

#include <Zany80/Plugin.h>

using namespace Oryol;

class SimpleShell;
struct shell_command {
	int(*function)(Array<String>, SimpleShell *shell);
	String help;
	String detailed_help;
};

class SimpleShell : public PerpetualPlugin, public ShellPlugin {
	OryolClassDecl(SimpleShell);
	OryolTypeDecl(SimpleShell, ShellPlugin);
public:
	SimpleShell();
	virtual ~SimpleShell();
	virtual void frame(float delta);
	virtual bool supports(String type);
	void output(String);
	virtual void output(const char *fmt, ...) __attribute__((format(printf, 2, 3)));
	virtual int execute(String command);
	virtual Array<String> getCommands();
private:
	unsigned long instance;
	char command_string[256];
	Map<String, shell_command> commands;
	Array<String> history;
	Map<String, String(*)(String, SimpleShell *shell)> autocompletion_map;
	String working_directory;
	void updateWorkingDirectory();
	int processCommand();
	int last_result;
	bool scroll_on_output = true;
	int outputted = 0;
};
