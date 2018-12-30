#include "Editor.h"
#include "Identifiers.h"

#include <IMUI/IMUI.h>

#include <Zany80/API.h>
#include <editor_config.h>

#include <stdio.h>

#define start_text ".globl _main\n\
_main:\n\
	ld a, 0\n\
	call putchar\n\
	ld hl, msg\n\
	call serial_write\n\
.loop:\n\
	call _waitchar\n\
	ld a, l\n\
	call putchar\n\
	jr .loop\n\
\n\
msg: .asciiz \"ROM built by Zany80 Scas plugin...\\n\"\n\
received: .asciiz \"Character received: \"\n"

#ifdef ORYOL_EMSCRIPTEN
#include <emscripten.h>
EM_JS(void, download, (const char* name, const char* str), {
	var element = document.createElement('a');
	element.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(UTF8ToString(str).split("\n").join("\\r\n")));
	element.setAttribute('download', UTF8ToString(name));
	element.style.display = 'none';
	document.body.appendChild(element);
	element.click();
	document.body.removeChild(element);
});

Editor *editor_instance;

extern "C" void EMSCRIPTEN_KEEPALIVE EditorOpen(const char *name, const char *data) {
	editor_instance->ResetEditor();
	editor_instance->widget.SetText(data);
	editor_instance->has_file = true;
	editor_instance->path = name;
}
#endif

void Editor::ResetEditor() {
	widget = TextEditor();
	widget.SetLanguageDefinition(language);
}

Editor::Editor() {
	if (!requirePlugin("Toolchain"))
		return;
	language = TextEditor::LanguageDefinition::C();
	language.mIdentifiers = TextEditor::Identifiers();
	language.mName = "Zany80 Assembly";
	language.mSingleLineComment = ";";
	language.mPreprocChar = '.';
	for (Map<const char *, const char *> m : identifiers) {
		for (KeyValuePair<const char *,const char *> k : m) {
			 TextEditor::Identifier id;
			 id.mDeclaration = k.value;
			 language.mIdentifiers[k.key] = id;
		} 
	}
	#ifdef ORYOL_EMSCRIPTEN
	editor_instance = this;
	#endif
	sprintf(window_title, "Editor v" PROJECT_VERSION "##%lu", this->instance = plugin_instances++);
	buffer = nullptr;
	cpu = nullptr;
	assembler = nullptr;
	NewFile();
}

void Editor::NewFile() {
	ResetEditor();
	widget.SetText(start_text);
	path = "";
	has_file = false;
	needs_info = false;
	if (buffer) {
		delete[] buffer;
		buffer = nullptr;
	}
	markers.clear();
}

void Editor::DoSave() {
	o_assert_dbg(has_file);
	FILE *file = fopen(path.AsCStr(), "w");
	Map<int, String> error_strings = {
		{ EACCES, "Access to file denied!" },
		{ EISDIR, "That's a folder. Not a file." },
		{ ENOENT, "No such file!" },
		{ EPERM,  "Permission denied!" },
		{ ENOSPC, "Disk full!" },
		{ ETXTBSY,"File is being used by another application!" }
	};
	if (file) {
		fputs((UnmodifiedText = widget.GetText()).c_str(), file);
		fflush(file);
		fclose(file);
	}
	else {
		if (error_strings.FindIndex(errno) != InvalidIndex) {
			messages.Add(error_strings[errno]);
		}
	}
}

void Editor::SaveAs() {
	needs_info = true;
	buffer_size = 256;
	buffer = new char[256]{0};
	info_type = save;
	messages.Clear();
	messages.Add("Give the file a name...");
}

void Editor::Save() {
	has_file ? DoSave() : SaveAs();
}

void Editor::Open() {
	needs_info = true;
	buffer_size = 256;
	buffer = new char[256]{0};
	info_type = open;
	messages.Clear();
	messages.Add("Select a file to open...");
}

bool Editor::Open(String path) {
	bool success = false;
	needs_info = false;
	FILE *file = fopen(path.AsCStr(), "r");
	Map<int, String> error_strings = {
		{ EACCES, "Access to file denied!" },
		{ EISDIR, "That's a folder. Not a file." },
		{ ENOENT, "No such file!" },
		{ EPERM,  "Permission denied!" }
	};
	if (file == NULL) {
		if (error_strings.FindIndex(errno) != InvalidIndex) {
			messages.Add(error_strings[errno]);
		}
	}
	else {
		// Get file size
		long size;
		if (fseek(file, 0, SEEK_END) == -1 || (size = ftell(file)) == -1) {
			messages.Add("Error obtaining file size!");
		}
		else {
			rewind(file);
			char buffer[size + 1];
			if (fread(buffer, 1, size, file) != (size_t)size) {
				// Failed to read whole file!
				messages.Clear();
				messages.Add("Error loading in file!");
			}
			else {
				buffer[size] = 0;
				ResetEditor();
				widget.SetText(buffer);
				has_file = true;
				this->path = path;
				success = true;
			}
		}
		fclose(file);
	}
	return success;
}

void Editor::Save(String path) {
	needs_info = false;
	FILE *file = fopen(path.AsCStr(), "w");
	Map<int, String> error_strings = {
		{ EACCES, "Access to file denied!" },
		{ EISDIR, "That's a folder. Not a file." },
		{ ENOENT, "No such file!" },
		{ EPERM,  "Permission denied!" },
		{ ENOSPC, "Disk full!" },
		{ ETXTBSY,"File is being used by another application!" }
	};
	if (file == NULL) {
		if (error_strings.FindIndex(errno) != InvalidIndex) {
			messages.Add(error_strings[errno]);
		}
	}
	else {
		fclose(file);
		has_file = true;
		this->path = path;
		DoSave();
	}
}

void Editor::CheckError(String s) {
	StringBuilder str = StringBuilder(s);
	const char *msg = "Error on line ";
	if ((size_t)str.Length() > strlen(msg) + 4 && str.GetSubString(0, strlen(msg)) == String(msg)) {
		const char *ptr = s.AsCStr() + strlen(msg);
		char *nptr;
		long line = strtol(ptr, &nptr, 10);
		markers[line] = (nptr + 2);
	}
}

void Editor::Build() {
	if (has_file) {
		DoSave();
		if (!assembler) {
			SelectASM();
		}
		if (assembler) {
			assembler->setVerbosity(1);
			StringBuilder s = StringBuilder(path);
			s.Append(".rom");
			String target = s.GetString();
			s.Set("");
			int i = assembler->transform({processFileURI("lib:libc.o"), path}, target, &s);
			messages.Clear();
			markers.clear();
			if (i == 0) {
				messages.Add("Assembled successfully!");
			}
			else {
				Array<String> newMsgs;
				s.Tokenize("\n", newMsgs);
				for (int i = 0; i < newMsgs.Size(); i++) {
					messages.Add(newMsgs[i]);
					CheckError(newMsgs[i]);
				}
			}
			widget.SetErrorMarkers(markers);
		}
	}
	else {
		SaveAs();
	}
}

void Editor::Execute() {
	if (has_file) {
		// TODO: make sure it has been built first
		if (!cpu) {
			Array<PPTR> cpus = getPlugins("CPU");
			if (cpus.Size() == 1) {
				cpu = CAST_PPTR(cpus[0], CPUPlugin);
				messages.Clear();
				messages.Add("One CPU detected, automatically selecting...");
			}
			else {
				messages.Add("Try selecting a CPU (Build -> Select CPU) first!");
			}
		}
		if (cpu) {
			StringBuilder rom = path;
			rom.Append(".rom");
			messages.Add("Instructing CPU to run this program...");
			cpu->loadROM(rom.AsCStr());
			cpu->reset();
		}
	}
	else {
		SaveAs();
	}
}

void Editor::SelectCPU(String s) {
	const char *str = s.AsCStr();
	char *end;
	errno = 0;
	long c = strtol(str, &end, 10);
	if (errno != 0 || str == end) {
		messages.Clear();
		messages.Add("Invalid number!");
	}
	else {
		Array<PPTR> cpus = getPlugins("CPU");
		if (c < cpus.Size()) {
			this->cpu = CAST_PPTR(cpus[c], CPUPlugin);
			messages.Add("Attached to CPU.");
		}
		else {
			messages.Clear();
			messages.Add("No such CPU");
		}
	}
}

void Editor::SelectCPU() {
	Array<PPTR> cpus = getPlugins("CPU");
	if (cpus.Size() == 1) {
		cpu = CAST_PPTR(cpus[0], CPUPlugin);
		messages.Clear();
		messages.Add("One CPU detected, automatically selecting...");
	}
	else if (cpus.Size()) {
		info_type = set_cpu;
		needs_info = true;
	}
	else {
		messages.Add("No CPUs loaded!");
	}
}

void Editor::SelectASM(String s) {
	const char *str = s.AsCStr();
	char *end;
	errno = 0;
	long t = strtol(str, &end, 10);
	if (errno != 0 || str == end) {
		messages.Clear();
		messages.Add("Invalid number!");
	}
	else {
		Array<PPTR> tools = getPlugins("Toolchain");
		if (t < tools.Size()) {
			this->assembler = CAST_PPTR(tools[t], ToolchainPlugin);
			messages.Add("Assembler selected");
		}
		else {
			messages.Clear();
			messages.Add("No such assembler!");
		}
	}
}

void Editor::SelectASM() {
	// TODO: ensure selected tool is assembler
	Array<PPTR> tools = getPlugins("Toolchain");
	if (tools.Size() == 1) {
		assembler = CAST_PPTR(tools[0], ToolchainPlugin);
		messages.Clear();
		messages.Add("One assembler detected, automatically selecting...");
	}
	else if (tools.Size()) {
		info_type = set_asm;
		needs_info = true;
	}
	else {
		messages.Add("No assemblers loaded!");
	}
}

void Editor::frame(float delta) {
	ImGui::SetNextWindowSizeConstraints(ImVec2(400, 300), ImVec2(FLT_MAX, FLT_MAX));
	ImGui::Begin(window_title, NULL, ImGuiWindowFlags_MenuBar);
	ImGui::SetWindowSize(ImVec2(600,480), ImGuiCond_FirstUseEver);
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File", !needs_info)) {
			if (ImGui::MenuItem("New")) {
				NewFile();
			}
			if (ImGui::MenuItem("Open")) {
				Open();
			}
			if (ImGui::MenuItem("Save")) {
				Save();				
			}
			#ifdef ORYOL_EMSCRIPTEN
			ImGui::Separator();
			if (ImGui::MenuItem("Download", NULL, false, has_file)) {
				download(path.AsCStr(), UnmodifiedText.c_str());
			}
			#endif
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Build", !needs_info)) {
			if (ImGui::MenuItem("Build")) {
				Build();
			}
			if (ImGui::MenuItem("Execute")) {
				Execute();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Select CPU")) {
				SelectCPU();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
	if (!needs_info) {
		widget.Render("TextEditor", ImVec2(0, ImGui::GetWindowSize().y - 140));
		ImGui::Text(has_file ? "%sFile: \"%s\"" : "%sUntitled", widget.GetText() != UnmodifiedText ? "* " : "", has_file ? path.AsCStr() : "");
		ImGui::Text("%6d/%-6d %6d lines", widget.GetCursorPosition().mLine + 1, widget.GetCursorPosition().mColumn + 1, widget.GetTotalLines());
		ImGui::Text(widget.IsOverwrite() ? "Overwrite" : "Insert");
	}
	else {
		if (ImGui::InputText("", buffer, buffer_size - 1, ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_EnterReturnsTrue)) {
			String b = String(buffer);
			delete[] buffer;
			buffer = nullptr;
			switch (info_type) {
				case open:
					Open(b);
					break;
				case save:
					Save(b);
					break;
				case set_cpu:
					SelectCPU(b);
					break;
				case set_asm:
					SelectASM(b);
					break;
			}
		}
	}
	ImGui::Separator();
	ImGui::BeginChild("Messages", ImVec2(ImGui::GetWindowWidth() - 20, 30));
	for (String s : messages) {
		ImGui::Text("%s", s.AsCStr());
	}
	ImGui::EndChild();
	ImGui::End();
}

bool Editor::supports(String type) {
	return (Array<String>{"Perpetual"}).FindIndexLinear(type) != InvalidIndex;
}

extern "C" const char *getName() {
	return "Editor";
}

#ifndef ORYOL_EMSCRIPTEN

extern "C" Plugin *make() {
	return new Editor;
}

#endif
