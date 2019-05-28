#pragma once

#include <TextEditor.h>
#include <Core/String/String.h>
#include <Core/String/StringBuilder.h>
#include <Core/Containers/Array.h>
using namespace Oryol;

#include <Zany80/Plugin.h>

#ifdef ORYOL_EMSCRIPTEN
extern "C" void EditorOpen(const char *name, const char *path);
#endif

class Editor {
#ifdef ORYOL_EMSCRIPTEN
	friend void EditorOpen(const char *name, const char *path);
#endif
public:
	Editor();
	void frame(float delta);
private:
	// GUI routines
	void NewFile();
	void Save();
	void Save(String path);
	void DoSave();
	void SaveAs();
	void Open();
	bool Open(String path);
	void Build();
	void Execute();
	void SelectCPU();
	void SelectCPU(String s);
	void SelectASM();
	void SelectASM(String s);
	void CheckError(String s);
	TextEditor widget;
	// File info
	String path;
	bool has_file;
	// Used for info entry (URL, CPU, etc)
	bool needs_info;
	char *buffer;
	size_t buffer_size;
	Array<String> messages;
	enum {
		open, save, set_cpu, set_asm
	} info_type;
	// Target plugins
	cpu_plugin_t *cpu;
	toolchain_plugin_t *assembler;
	// Used to check for modifications
	std::string UnmodifiedText;
	// Mark errors
	TextEditor::ErrorMarkers markers;
	// Set up language etc
	void ResetEditor();
	bool stdlib;
	TextEditor::LanguageDefinition language;
};
