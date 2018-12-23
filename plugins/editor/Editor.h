#pragma once

#include <Zany80/Plugin.h>
#include <TextEditor.h>

#ifdef ORYOL_EMSCRIPTEN
class Editor;
extern "C" void EditorOpen(const char *name, const char *path);
#endif

class Editor : public PerpetualPlugin {
#ifdef ORYOL_EMSCRIPTEN
	friend void EditorOpen(const char *name, const char *path);
#endif
	OryolClassDecl(Editor);
	OryolTypeDecl(Editor, PerpetualPlugin);
public:
	Editor();
	virtual void frame(float delta);
	virtual bool supports(String type);
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
	// Plugin instance, used to disassociate windows of multiple instances of editors
	unsigned long instance;
	char window_title[64];
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
	CPUPlugin *cpu;
	ToolchainPlugin *assembler;
	// Used to check for modifications
	std::string UnmodifiedText;
	// Mark errors
	TextEditor::ErrorMarkers markers;
	// Set up language etc
	void ResetEditor();
	TextEditor::LanguageDefinition language;
};
