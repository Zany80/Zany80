#include "Display.h"

#include <IMUI/IMUI.h>

#include <Zany80/API.h>

Display::Display() {
	requirePlugin("CPU");
}

void* Display::named_function(String functionName, ...) {
	if (functionName == "clear") {
		output_buffer.Clear();
	}
	return 0;
}

void Display::frame(float delta) {
	// Window starts at 300x200, minimal size of 200x160
	ImGui::SetNextWindowSizeConstraints(ImVec2(200, 160), ImVec2(FLT_MAX, FLT_MAX));
	ImGui::Begin("Serial Monitor", NULL, ImGuiWindowFlags_MenuBar);
	ImGui::SetWindowSize(ImVec2(300,200), ImGuiCond_FirstUseEver);
	if (ImGui::BeginMenuBar()) {
		if (ImGui::MenuItem("Clear")) {
			named_function("clear");
		}
		ImGui::EndMenuBar();
	}
	if (!connected) {
		Array<Plugin*> cpus = getPlugins("z80");
		if (cpus.Size()) {
			connected = true;
			cpu = dynamic_cast<CPUPlugin*>(cpus[0]);
			cpu->attachToPort(1, [this](uint8_t value) {
				if (value == 0)
					output_buffer.Clear();
				else {
					output_buffer.Append((char)value);
					int index = output_buffer.FindLastOf(0, EndOfString, "\n");
					if (index != InvalidIndex && index + 1 < output_buffer.Length()) {
						String lastLine = output_buffer.GetSubString(index + 1, EndOfString);
						if ((lastLine.Length() % 80) == 0)
							output_buffer.Append((char)10);
					}
				}
			});
			cpu->attachToPort(1, [this]() -> uint8_t {
				if (input_buffer.Empty()) {
					return 0;
				}
				return input_buffer.PopFront();
			});
		}
		else {
			ImGui::Text("No CPU loaded!");
		}
	}
	// TODO: remove this InputText and use direct inputs
	else {
		static char buf[256];
		if (ImGui::InputText("", buf, 255, ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_EnterReturnsTrue)) {
			strcpy(buf + strlen(buf), "\n");
			for (size_t i = 0; i < strlen(buf); i++) {
				input_buffer.Add(buf[i]);
				cpu->fireInterrupt(0);
			}
			strcpy(buf, "");
		}
		ImGui::Text("%s", output_buffer.AsCStr());
	}
	ImGui::End();
}

bool Display::supports(String type) {
	return (Array<String>{"Perpetual", "Serial Monitor", "Display"}).FindIndexLinear(type) != InvalidIndex;
}

#ifndef ORYOL_EMSCRIPTEN

extern "C" const char *getName() {
	return "Serial Monitor";
}

extern "C" Plugin *make() {
	return new Display;
}

#endif
