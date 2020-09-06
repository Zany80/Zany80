#include "SIMPLE/Plugin.h"
#include "SIMPLE/API.h"
#include "SIMPLE.h"
#include "SIMPLE/internal/graphics.h"
extern "C" {
#include "SIMPLE/internal/cleanup.h"
}

#include <Gfx/Gfx.h>
#include <IMUI/IMUI.h>
#include <Input/Input.h>
#include <IO/IO.h>
#include <LocalFS/LocalFileSystem.h>

#include <sstream>

#include <config.h>

SIMPLE *zany;

OryolMain(SIMPLE);

Array<String> available_plugins;

static void tf() {
	zany->ToggleFullscreen();
}

extern "C" void simple_report_error(const char *error_message) {
	zany->report_error(error_message);
}

void SIMPLE::report_error(const char *error) {
	StringBuilder s(this->error);
	if (s.Length())
		s.Append('\n');
	s.Append(error);
	this->error = s.GetString();
}

static TimePoint start;
static menu_t *options_menu;

SIMPLE::SIMPLE() {}

AppState::Code SIMPLE::OnInit() {
	this->is_fullscreen = false;
	this->error = "";
	this->show_debug_window = true;
	start = Clock::Now();
	this->tp = start;
	options_menu = menu_create("Options");
	window_append_menu(get_root(), options_menu);
	menu_append(options_menu, checkbox_create("Show Debug Window", &this->show_debug_window, NULL));
	menu_append(options_menu, checkbox_create("Toggle Fullscreen", &this->is_fullscreen, tf));
	zany = this;
	IOSetup ioSetup;
	GfxSetup gfxSetup = GfxSetup::WindowMSAA4(800, 600, "Zany80 Fantasy Computer - Native Edition");
	gfxSetup.HtmlTrackElementSize = true;
	ioSetup.FileSystems.Add("file", LocalFileSystem::Creator());
	IO::Setup(ioSetup);
	Gfx::Setup(gfxSetup);
	SetupIcon();
#if ORYOL_LINUX
	StringBuilder s(IO::ResolveAssigns("root:"));
	if (!s.Contains("fips-deploy")) {
		Log::Error("Reconfiguring from '%s'\n", s.AsCStr());
		for (int i = 0; i < 2; i++) {
			int j = s.FindLastOf(0, EndOfString, "/");
			o_assert(j != InvalidIndex);
			s.Set(s.GetSubString(0, j));
		}
		s.Append("/share/zany80/");
		IO::SetAssign("root:", s.GetString());
	}
	else {
		Log::Dbg("Running locally...\n");
	}
#endif
	IO::SetAssign("lib:", "root:lib/");
	Log::Dbg("Application URL: %s\n", IO::ResolveAssigns("root:").AsCStr());
	InputSetup inputSetup;
	inputSetup.PinchEnabled = false;
	inputSetup.PanEnabled = false;
	inputSetup.AccelerometerEnabled = false;
	inputSetup.GyrometerEnabled = false;
	Input::Setup(inputSetup);
	IMUI::Setup();
	this->tp = Clock::Now();
	window_register(get_root());
	return App::OnInit();
}

AppState::Code SIMPLE::OnRunning() {
	static Duration delta = Clock::LapTime(zany->tp);
	Gfx::BeginPass(PassAction::Clear(glm::vec4(0.1f, 0.8f, 0.6f, 1.0f)));
	IMUI::NewFrame(delta);
	render_windows();
	if (zany->show_debug_window) {
		ImGui::Begin("Debug", NULL, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Native Edition");
    		ImGui::Text("Zany80 version: " PROJECT_VERSION ", ABI: %d", SIMPLE_ABI);
		ImGui::Text("Framerate: %.1f", 
		//~ ImGui::GetIO().Framerate > 60 ? 60 : 
				ImGui::GetIO().Framerate);
		#if FIPS_GLFW_WAYLAND
		ImGui::Text("Backend: Wayland/OpenGL");
		#elif ORYOL_GLFW
		ImGui::Text("Backend: OpenGL");
		#elif ORYOL_D3D11
		ImGui::Text("Backend: D3D11");
		#elif ORYOL_EMSCRIPTEN
		ImGui::Text("Backend: Emscripten");
		#else
		ImGui::Text("Unknown backend. This platform may not be fully supported.");
		#endif
		if (ImGui::Button("Hide")) {
			zany->show_debug_window = false;
		}
		ImGui::End();
	}
	if (zany->error != "") {
		ImGui::SetNextWindowFocus();
		ImGui::Begin("Error!", NULL, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("%s", zany->error.AsCStr());
		if (ImGui::Button("Clear"))
			zany->error = "";
		ImGui::End();
	}
	ImGui::Render();
	Gfx::EndPass();
	Gfx::CommitFrame();
	return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

AppState::Code SIMPLE::OnCleanup() {
	menu_destroy_all(options_menu);
	menu_destroy(options_menu);
	window_destroy(get_root());
	IMUI::Discard();
	Input::Discard();
	Gfx::Discard();
	IO::Discard();
	return App::OnCleanup();
}

#if _DEBUG
static int current_level = SL_DEBUG;
#else
static int current_level = SL_INFO;
#endif

void simple_log(int level, const char *format, ...) {
	if (level < current_level) {
		return;
	}
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	fflush(stderr);
	va_end(args);
}

char *simple_root_folder() {
	StringBuilder s = IO::ResolveAssigns("root:");
	if (s.GetSubString(0, 4) == "file")
		s.Set(s.GetSubString(8, EndOfString));
	return strdup(s.AsCStr());
}

double s_simple_elapsed() {
	return Clock::Since(start).AsSeconds();
}
