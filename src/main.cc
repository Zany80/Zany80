#include <SIMPLE/Plugin.h>
#include <SIMPLE/API.h>
#include <Zany80.h>

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
#include <liblib/liblib.hpp>

//~ #include <scas/stringop.h>

unsigned long plugin_instances;

Zany80 *zany;

OryolMain(Zany80);

Array<String> available_plugins;

static void tf() {
    zany->ToggleFullscreen();
}

extern "C" void zany_report_error(const char *error_message) {
	zany->report_error(error_message);
}

void Zany80::report_error(const char *error) {
	StringBuilder s(this->error);
	if (s.Length())
		s.Append('\n');
	s.Append(error);
	this->error = s.GetString();
}

static TimePoint start;
static menu_t *options_menu;

Zany80::Zany80() {}

AppState::Code Zany80::OnInit() {
    this->is_fullscreen = false;
	this->error = "";
	this->show_debug_window = this->hub = false;
	start = Clock::Now();
	this->tp = start;
    window_t *w = get_root();
    options_menu = menu_create("Options");
    window_append_menu(w, options_menu);
    menu_append(options_menu, checkbox_create("Show Debug Window", &this->show_debug_window, NULL));
    menu_append(options_menu, checkbox_create("Show Plugin Hub", &this->hub, NULL));
    menu_append(options_menu, checkbox_create("Toggle Fullscreen", &this->is_fullscreen, tf));
	//~ if (ImGui::BeginMainMenuBar()) {
		//~ if (ImGui::BeginMenu("Options")) {
			//~ if (ImGui::BeginMenu("Set Log Level")) {
				//~ ImGui::Text("Current: %d", (int)Log::GetLogLevel());
				//~ if (ImGui::Button("Debug (All information)"))
					//~ Log::SetLogLevel(Log::Level::Dbg);
				//~ if (ImGui::Button("Info (Ignore debug info)"))
					//~ Log::SetLogLevel(Log::Level::Info);
				//~ if (ImGui::Button("Warn (Only show warnings and errors)"))
					//~ Log::SetLogLevel(Log::Level::Warn);
				//~ if (ImGui::Button("Error (Only show errors)"))
					//~ Log::SetLogLevel(Log::Level::Error);
				//~ if (ImGui::Button("Mute (Surpress all logging)"))
					//~ Log::SetLogLevel(Log::Level::None);
				//~ ImGui::EndMenu();
			//~ }
			//~ ImGui::Checkbox("Show Debug Window", &this->show_debug_window);
			//~ ImGui::Checkbox("Show Plugin Hub", &this->hub);
			//~ if (ImGui::Checkbox("Fullscreen Mode", &this->is_fullscreen))
				//~ this->ToggleFullscreen();
			//~ ImGui::EndMenu();
		//~ }
		//~ ImGui::EndMainMenuBar();
	//~ }
	plugin_instances = 0;
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
	IO::SetAssign("lib:", "/");
#elif FIPS_EMSCRIPTEN_USE_WASM
	IO::SetAssign("root:", "https://zany80.github.io/native/wasm/");
	IO::SetAssign("lib:", "/native/wasm/");
#else
	IO::SetAssign("root:", "https://zany80.github.io/native/emscripten/");
	IO::SetAssign("lib:", "/lib/");
#endif
#endif
	IO::SetAssign("plugins:", "root:plugins/");
	IO::SetAssign("lib:", "root:lib/");
#ifdef ORYOL_EMSCRIPTEN
    IO::SetAssign("root:", "https://pixelherodev.github.io/asl_build/");
	// Preload stdlib.o
	//~ IO::Load("root:stdlib.o", [](IO::LoadResult res) {
		 //~ mkdir("/lib", 0700);
		 //~ FILE *file = fopen("/lib/stdlib.o", "w");
		 //~ if (file) {
			 //~ fwrite(res.Data.Data(), 1, res.Data.Size(), file);
			 //~ fflush(file);
			 //~ fclose(file);
			 //~ puts("stdlib.o preloaded");
		 //~ }
		 //~ else {
	 		//~ ::zany_report_error("Error opening /lib/stdlib.o for writing!");
		 //~ }
	//~ }, [](const URL& url, IOStatus::Code ioStatus) {
		//~ ::zany_report_error("Error loading stdlib!");
	//~ });
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
	return App::OnInit();
}

static Duration delta;

AppState::Code Zany80::OnRunning() {
	delta = Clock::LapTime(this->tp);
	Gfx::BeginPass(PassAction::Clear(glm::vec4(0.1f, 0.8f, 0.6f, 1.0f)));
	IMUI::NewFrame(delta);
	render_window(get_root());
	if (this->show_debug_window) {
		ImGui::Begin("Debug", NULL, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Native Edition");
		ImGui::Text("Zany80 version: " PROJECT_VERSION);
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

void unload_all_plugins();

AppState::Code Zany80::OnCleanup() {
	unload_all_plugins();
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
