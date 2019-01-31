#include <IMUI/IMUI.h>
#include <Zany80/Plugin.h>
#include <Core/String/String.h>
#include <Core/String/StringBuilder.h>
#include <Core/Containers/Array.h>
using namespace Oryol;

bool connected;
StringBuilder *output_buffer;
Array<char> *input_buffer;
cpu_plugin_t *cpu;

void clear() {
	output_buffer->Clear();
}

bool attach_cpu() {
	list_t *cpus = get_plugins("z80");
	if (cpus->length > 0) {
		connected = true;
		cpu = ((plugin_t*)cpus->items[0])->cpu;
		cpu->attach_output(1, [](uint8_t value) {
			if (value == 0)
				clear();
			else {
				output_buffer->Append((char)value);
				int index = output_buffer->FindLastOf(0, EndOfString, "\n");
				if (index != InvalidIndex && index + 1 < output_buffer->Length()) {
					String lastLine = output_buffer->GetSubString(index + 1, EndOfString);
					if ((lastLine.Length() % 80) == 0)
						output_buffer->Append((char)10);
				}
			}
		});
		cpu->attach_input(1, []() -> uint8_t {
			if (input_buffer->Empty()) {
				return 0;
			}
			return input_buffer->PopFront();
		});
	}
	return connected;
}

void frame(float delta) {
	// Window starts at 300x200, minimal size of 200x160
	ImGui::SetNextWindowSizeConstraints(ImVec2(200, 160), ImVec2(FLT_MAX, FLT_MAX));
	ImGui::Begin("Serial Monitor", NULL, ImGuiWindowFlags_MenuBar);
	ImGui::SetWindowSize(ImVec2(300,200), ImGuiCond_FirstUseEver);
	if (ImGui::BeginMenuBar()) {
		if (ImGui::MenuItem("Clear")) {
			clear();
		}
		ImGui::EndMenuBar();
	}
	if (!connected) {
		if (!attach_cpu()) {
			ImGui::Text("No CPU loaded!");
		}
	}
	// TODO: remove this InputText and use direct inputs
	else {
		static char buf[256];
		if (ImGui::InputText("", buf, 255, ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_EnterReturnsTrue)) {
			strcpy(buf + strlen(buf), "\n");
			for (size_t i = 0; i < strlen(buf); i++) {
				input_buffer->Add(buf[i]);
				cpu->fire_interrupt(0);
			}
			strcpy(buf, "");
		}
		ImGui::Text("%s", output_buffer->AsCStr());
	}
	ImGui::End();
}

perpetual_plugin_t perpetual = {
	.frame = frame
};

plugin_t plugin;

extern "C" {
	
	void init() {
		plugin.name = "Serial Monitor";
		plugin.supports = [](const char *type) -> bool {
			return (!strcmp(type, "Perpetual")) || (!strcmp(type, "Display")) || (!strcmp(type, "Serial Monitor"));
		};
		plugin.perpetual = &perpetual;
		connected = false;
		require_plugin("z80cpp_core");
		output_buffer = new StringBuilder;
		input_buffer = new Array<char>;
		attach_cpu();
	}
	
	void cleanup() {
		delete input_buffer;
		delete output_buffer;
	}
	
	plugin_t *get_interface() {
		return &plugin;
	}
	
}
