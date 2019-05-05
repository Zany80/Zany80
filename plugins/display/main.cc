#include <IMUI/IMUI.h>
#include <Zany80/Plugin.h>
#include <Core/String/String.h>
#include <Core/String/StringBuilder.h>
#include <Core/Containers/Array.h>
#include <Input/Input.h>
using namespace Oryol;

bool connected;
bool focused = false;
StringBuilder *output_buffer;
Array<uint8_t> *input_buffer;
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
				return -1;
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
		focused = ImGui::IsWindowFocused();
		//~ static char buf[256];
		//~ if (ImGui::InputText("", buf, 255, ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_EnterReturnsTrue)) {
			//~ strcpy(buf + strlen(buf), "\n");
			//~ for (size_t i = 0; i < strlen(buf); i++) {
				//~ input_buffer->Add(buf[i]);
				//~ cpu->fire_interrupt(0);
			//~ }
			//~ strcpy(buf, "");
		//~ }
		ImGui::Text("%s", output_buffer->AsCStr());
	}
	ImGui::End();
}

/*
;; Key layout:
;;  0-25 are a-z
;; 26-35 are 0-9
;; 36-39 are up,left,down,right
;; 40 is spacebar
;; 41 is backspace
;; 42 is escape
;; 43-52 are []{}()<>/\
;; 53 is |
;; 54-55 are " and '
;; 56-60 are `~:;?
;; 61-63 is Caps,Alt,Ctrl
;; 64 is tab
;; 65-69 are -_+=
;; 70-77 are !@#$%^&*
;; 78 is delete
;; 79-83 are PgUp PgDown Home End Enter
;; Caps isn't actually a key; it's effectively (CAPS_LOCK ^ SHIFT).
;; Bit seven is 0 on press and 1 on release
*/

const Map<Key::Code, uint8_t> keymap = {
	{ Key::A, 0 },
	{ Key::B, 1 },
	{ Key::C, 2 },
	{ Key::D, 3 },
	{ Key::E, 4 },
	{ Key::F, 5 },
	{ Key::G, 6 },
	{ Key::H, 7 },
	{ Key::I, 8 },
	{ Key::J, 9 },
	{ Key::K, 10 },
	{ Key::L, 11 },
	{ Key::M, 12 },
	{ Key::N, 13 },
	{ Key::O, 14 },
	{ Key::P, 15 },
	{ Key::Q, 16 },
	{ Key::R, 17 },
	{ Key::S, 18 },
	{ Key::T, 19 },
	{ Key::U, 20 },
	{ Key::V, 21 },
	{ Key::W, 22 },
	{ Key::X, 23 },
	{ Key::Y, 24 },
	{ Key::Z, 25 },
	{ Key::N0, 26 },
	{ Key::N1, 27 },
	{ Key::N2, 28 },
	{ Key::N3, 29 },
	{ Key::N4, 30 },
	{ Key::N5, 31 },
	{ Key::N6, 32 },
	{ Key::N7, 33 },
	{ Key::N8, 34 },
	{ Key::N9, 35 },
	
};

bool pcaps = false;

perpetual_plugin_t perpetual;
plugin_t plugin;

extern "C" {
	
	PLUGIN_EXPORT void init() {
		perpetual.frame = frame;
		plugin.name = "Serial Monitor";
		plugin.supports = [](const char *type) -> bool {
			return (!strcmp(type, "Perpetual")) || (!strcmp(type, "Display")) || (!strcmp(type, "Serial Monitor"));
		};
		plugin.perpetual = &perpetual;
		connected = false;
		output_buffer = new StringBuilder;
		input_buffer = new Array<uint8_t>;
		require_plugin("z80cpp_core");
		attach_cpu();
		// Input::SubscribeEvents([](InputEvent e) {
		// 	if (focused) {
		// 		if (e.Type == InputEvent::KeyDown) {
		// 			if (e.KeyCode == Key::LeftShift || e.KeyCode == Key::RightShift || e.KeyCode == Key::CapsLock) {
		// 				bool caps = (Input::KeyPressed(Key::LeftShift) || Input::KeyPressed(Key::RightShift)) != Input::KeyPressed(Key::CapsLock);
		// 				if (caps != pcaps) {
		// 					pcaps = caps;
		// 					input_buffer->Add(caps ? 0xBD : 0x3D);
		// 					cpu->fire_interrupt(1);
		// 				}
		// 			}
		// 			else {
		// 				//~ Log::Dbg("Adding %d to input buffer\n", keymap.Contains(e.KeyCode) ? keymap[e.KeyCode] : -1);
		// 				input_buffer->Add(keymap.Contains(e.KeyCode) ? keymap[e.KeyCode] : -1);
		// 				cpu->fire_interrupt(1);
		// 			}
		// 		}
		// 		else if(e.Type == InputEvent::KeyUp) {
		// 			if (e.KeyCode == Key::LeftShift || e.KeyCode == Key::RightShift || e.KeyCode == Key::CapsLock) {
		// 				bool caps = (Input::KeyPressed(Key::LeftShift) || Input::KeyPressed(Key::RightShift)) != Input::KeyPressed(Key::CapsLock);
		// 				if (caps != pcaps) {
		// 					pcaps = caps;
		// 					input_buffer->Add(caps ? 0xBD : 0x3D);
		// 					cpu->fire_interrupt(2);
		// 				}
		// 			}
		// 			else {
		// 				input_buffer->Add(keymap.Contains(e.KeyCode) ? keymap[e.KeyCode] | 0x80 : -1);
		// 				cpu->fire_interrupt(2);
		// 			}
		// 		}
		// 	}
		// });
	}
	
	PLUGIN_EXPORT void cleanup() {
		delete input_buffer;
		delete output_buffer;
	}
	
	PLUGIN_EXPORT plugin_t *get_interface() {
		return &plugin;
	}
	
}
