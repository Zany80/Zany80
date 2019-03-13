#include "Editor.h"
#include "Identifiers.h"

#include <IMUI/IMUI.h>
#include <IO/IO.h>

#include <Zany80/Plugin.h>
#include <editor_config.h>

#include <stdio.h>

#define start_text ".export _main\n\
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
msg: .asciiz \"ROM built by Zany80 Scas plugin...\\n\"\n"

const char *validate_cpu(cpu_plugin_t *cpu) {
	if (cpu == nullptr) {
		return "CPU information structure absent";
	}
	if (cpu->load_rom == nullptr) {
		return "CPU lacks ROM loading functionality";
	}
	if (cpu->reset == nullptr) {
		return "CPU lacks external reset functionality";
	}
	return nullptr;
}

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
	require_plugin("Assembler");
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
		{ EACCES, "File access denied!" },
		{ EISDIR, "That's a folder, which is not a file." },
		{ ENOENT, "No such file or directory!" },
		{ EPERM,  "Permission denied by the operating system!" },
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
			StringBuilder s = StringBuilder(path);
			s.Append(".rom");
			String target = s.GetString();
			char *buf = NULL;
			list_t *sources = create_list();
			String libc = StringBuilder(IO::ResolveAssigns("lib:stdlib.o")).GetSubString(8, EndOfString);
			list_add(sources, (void*)libc.AsCStr());
			list_add(sources, (void*)path.AsCStr());
			int i = assembler->convert(sources, target.AsCStr(), &buf);
			messages.Clear();
			markers.clear();
			if (i == 0) {
				messages.Add("Assembled successfully!");
			}
			else {
				Array<String> newMsgs;
				StringBuilder(buf).Tokenize("\n", newMsgs);
				for (int i = 0; i < newMsgs.Size(); i++) {
					messages.Add(newMsgs[i]);
					CheckError(newMsgs[i]);
				}
			}
			free(buf);
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
			list_t *cpus = get_plugins("z80cpp_core");
			if (cpus->length == 1) {
				const char *error = validate_cpu(((plugin_t*)cpus->items[0])->cpu);
				if (error == nullptr) {
					cpu = ((plugin_t*)cpus->items[0])->cpu;
					messages.Clear();
					messages.Add("One CPU detected, automatically selecting...");
				}
				else {
					messages.Clear();
					messages.Add("Insufficient CPU plugin detected. Error:");
					messages.Add(error);
				}
			}
			else {
				messages.Add("Try selecting a CPU (Build -> Select CPU) first!");
			}
			list_free(cpus);
		}
		if (cpu) {
			StringBuilder rom = path;
			rom.Append(".rom");
			messages.Add("Instructing CPU to run this program...");
			cpu->load_rom(rom.AsCStr());
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
		list_t *cpus = get_plugins("CPU");
		if (c < cpus->length) {
			const char *error = validate_cpu(((plugin_t*)cpus->items[c])->cpu);
			if (error != nullptr) {
				messages.Add("Attached to CPU.");
				cpu = ((plugin_t*)cpus->items[c])->cpu;
			}
			else {
				messages.Clear();
				messages.Add("Insufficient CPU plugin detected. Error:");
				messages.Add(error);
			}
		}
		else {
			messages.Clear();
			messages.Add("No such CPU");
		}
		list_free(cpus);
	}
}

void Editor::SelectCPU() {
	list_t *cpus = get_plugins("CPU");
	if (cpus->length == 1) {
		const char *error = validate_cpu(((plugin_t*)cpus->items[0])->cpu);
		if (error != nullptr) {
			messages.Add("One CPU detected, automatically selecting...");
			cpu = ((plugin_t*)cpus->items[0])->cpu;
		}
		else {
			messages.Clear();
			messages.Add("Insufficient CPU plugin detected. Error:");
			messages.Add(error);
		}
	}
	else if (cpus->length > 0) {
		info_type = set_cpu;
		needs_info = true;
	}
	else {
		messages.Add("No CPUs loaded!");
	}
	list_free(cpus);
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
		list_t *asms = get_plugins("Assembler");
		if (t < asms->length) {
			this->assembler = ((plugin_t*)asms->items[t])->toolchain;
			messages.Add("Assembler selected");
		}
		else {
			messages.Clear();
			messages.Add("No such assembler!");
		}
	}
}

void Editor::SelectASM() {
	// TODO: ensure selected tool is an assembler
	list_t *asms = get_plugins("Assembler");
	if (asms->length == 1) {
		assembler = ((plugin_t*)asms->items[0])->toolchain;
		messages.Clear();
		messages.Add("One assembler detected, automatically selecting...");
	}
	else if (asms->length > 0) {
		info_type = set_asm;
		needs_info = true;
	}
	else {
		messages.Add("No assemblers loaded!");
	}
	list_free(asms);
}

void Editor::frame(float delta) {
	ImGui::SetNextWindowSizeConstraints(ImVec2(400, 300), ImVec2(FLT_MAX, FLT_MAX));
	ImGui::Begin("Editor", NULL, ImGuiWindowFlags_MenuBar);
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
