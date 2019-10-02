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
#if ORYOL_EMSCRIPTEN
#include <HttpFS/HTTPFileSystem.h>
#include <stdio.h>
#include <sys/stat.h>
#else
#include <LocalFS/LocalFileSystem.h>
#endif

#include <sstream>

#include <config.h>

#include <git2.h>

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
static window_t *plugin_manager;
static widget_t *pm_group;

extern "C"
widget_t *get_plugin_manager() {
	return pm_group;
}
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
	GfxSetup gfxSetup = GfxSetup::WindowMSAA4(800, 600, "SIMPLE Platform (Native Edition) v" PROJECT_VERSION);
	gfxSetup.HtmlTrackElementSize = true;
#if ORYOL_EMSCRIPTEN
#ifdef FIPS_EMSCRIPTEN_LOCALIZE
	ioSetup.FileSystems.Add("http", HTTPFileSystem::Creator());
#else
	ioSetup.FileSystems.Add("https", HTTPFileSystem::Creator());
#endif
#else
	ioSetup.FileSystems.Add("file", LocalFileSystem::Creator());
#endif
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
#elif ORYOL_EMSCRIPTEN
#ifdef FIPS_EMSCRIPTEN_LOCALIZE
	IO::SetAssign("root:", "http://localhost:8000/");
#elif FIPS_EMSCRIPTEN_USE_WASM
	IO::SetAssign("root:", "https://zany80.github.io/native/wasm/");
#else
	IO::SetAssign("root:", "https://zany80.github.io/native/emscripten/");
#endif
#endif
	IO::SetAssign("plugins:", "root:plugins/");
	IO::SetAssign("lib:", "root:lib/");
#ifdef ORYOL_EMSCRIPTEN
    IO::SetAssign("root:", "https://pixelherodev.github.io/asl_build/");
#endif
	Log::Dbg("Application URL: %s\n", IO::ResolveAssigns("root:").AsCStr());
	Log::Dbg("Plugin URL: %s\n", IO::ResolveAssigns("plugins:").AsCStr());
	InputSetup inputSetup;
    inputSetup.PinchEnabled = false;
    inputSetup.PanEnabled = false;
    inputSetup.AccelerometerEnabled = false;
    inputSetup.GyrometerEnabled = false;
	Input::Setup(inputSetup);
	IMUI::Setup();
	this->tp = Clock::Now();
	plugin_manager = window_create("Plugin Manager");
	window_set_pos(plugin_manager, 0, 30);
	window_min_size(plugin_manager, 200, 50);
	window_auto_size(plugin_manager, true);
	pm_group = group_create();
	window_append(plugin_manager, pm_group);
	generate_plugin_manager();
	window_register(get_root());
	window_register(plugin_manager);
	git_libgit2_init();
	return App::OnInit();
}

static Duration delta;

extern "C"
void process_threads();

AppState::Code SIMPLE::OnRunning() {
	process_threads();
	delta = Clock::LapTime(this->tp);
	Gfx::BeginPass(PassAction::Clear(glm::vec4(0.1f, 0.8f, 0.6f, 1.0f)));
	IMUI::NewFrame(delta);
	render_windows();
	if (this->show_debug_window) {
		ImGui::Begin("Debug", NULL, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Native Edition");
		ImGui::Text("SIMPLE version: " PROJECT_VERSION ", ABI: %d", SIMPLE_ABI);
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
			show_debug_window = false;
		}
		ImGui::End();
	}
	list_t *perpetuals = h_get_plugins("Perpetual");
	list_foreach(perpetuals, [](void *plugin) {
		((plugin_t*)plugin)->perpetual->frame(delta.AsSeconds());
	});
	list_free(perpetuals);
	if (error != "") {
		ImGui::SetNextWindowFocus();
		ImGui::Begin("Error!", NULL, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("%s", error.AsCStr());
		if (ImGui::Button("Clear"))
			error = "";
		ImGui::End();
	}
	ImGui::Render();
	Gfx::EndPass();
	Gfx::CommitFrame();
	return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

AppState::Code SIMPLE::OnCleanup() {
	updater_cleanup();
	git_libgit2_shutdown();
	unload_all_plugins();
    menu_destroy_all(options_menu);
    menu_destroy(options_menu);
    group_clear(pm_group, true);
    widget_destroy(pm_group);
    window_destroy(plugin_manager);
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
