#include <scas/list.h>
#include <scas/stringop.h>

#include <Zany80/Plugin.h>
#include <Zany80/Zany80.h>

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

#include <Zany80/API.h>
#include <sstream>

#include <config.h>
#include <liblib/liblib.hpp>

//~ #include <scas/stringop.h>

unsigned long plugin_instances;

Zany80 *zany;

__declspec(dllexport) ImGuiContext *imguicontext;

OryolMain(Zany80);

Array<String> available_plugins;

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

AppState::Code Zany80::OnInit() {
	plugin_instances = 0;
	zany = this;
	this->is_fullscreen = false;
	this->error = "";
	IOSetup ioSetup;
	GfxSetup gfxSetup = GfxSetup::WindowMSAA4(800, 600, "Zany80 (Native Edition) v" PROJECT_VERSION);
#if ORYOL_EMSCRIPTEN
	gfxSetup.HtmlTrackElementSize = true;
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
	// Preload stdlib.o
	IO::Load("root:stdlib.o", [](IO::LoadResult res) {
		 mkdir("/lib", 0700);
		 FILE *file = fopen("/lib/stdlib.o", "w");
		 if (file) {
			 fwrite(res.Data.Data(), 1, res.Data.Size(), file);
			 fflush(file);
			 fclose(file);
			 puts("stdlib.o preloaded");
		 }
		 else {
	 		::report_error("Error opening /lib/stdlib.o for writing!");
		 }
	}, [](const URL& url, IOStatus::Code ioStatus) {
		::report_error("Error loading stdlib!");
	});
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
	load_plugin("plugins:z80cpp_core");
	load_plugin("plugins:debug_port");
	load_plugin("plugins:editor");
	load_plugin("plugins:simple_shell");
	load_plugin("plugins:display");
	show_debug_window = hub = true;
	this->tp = Clock::Now();
	return App::OnInit();
}

static Duration delta;

AppState::Code Zany80::OnRunning() {
	delta = Clock::LapTime(this->tp);
	Gfx::BeginPass(PassAction::Clear(glm::vec4(0.1f, 0.8f, 0.6f, 1.0f)));
	IMUI::NewFrame(delta);
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Options")) {
			if (ImGui::BeginMenu("Set Log Level")) {
				ImGui::Text("Current: %d", (int)Log::GetLogLevel());
				if (ImGui::Button("Debug (All information)"))
					Log::SetLogLevel(Log::Level::Dbg);
				if (ImGui::Button("Info (Ignore debug info)"))
					Log::SetLogLevel(Log::Level::Info);
				if (ImGui::Button("Warn (Only show warnings and errors)"))
					Log::SetLogLevel(Log::Level::Warn);
				if (ImGui::Button("Error (Only show errors)"))
					Log::SetLogLevel(Log::Level::Error);
				if (ImGui::Button("Mute (Surpress all logging)"))
					Log::SetLogLevel(Log::Level::None);
				ImGui::EndMenu();
			}
			ImGui::Checkbox("Show Debug Window", &this->show_debug_window);
			ImGui::Checkbox("Show Plugin Hub", &this->hub);
			if (ImGui::Checkbox("Fullscreen Mode", &this->is_fullscreen))
				this->ToggleFullscreen();
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
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
	if (this->hub) {
		static bool show_loaded_plugins = true;
		static bool show_available_plugins = true;
		ImGui::SetNextWindowSizeConstraints(ImVec2(100,50), ImVec2(FLT_MAX, FLT_MAX));
		ImGui::SetNextWindowPos(ImVec2(400,300), ImGuiCond_FirstUseEver);
		ImGui::Begin("Plugin Hub", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("View")) {
				ImGui::Checkbox("Show loaded plugins", &show_loaded_plugins);
				ImGui::Checkbox("Show available plugins", &show_available_plugins);
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		if (show_loaded_plugins) {
			constexpr char *listed_tags[] = {
				"CPU", "Perpetual", "Shell", "z80", "Toolchain"
			};
			ImGui::Text("Plugins");
			list_t *plugins = get_all_plugins();
			for (int i = 0; i < plugins->length; i++) {
				plugin_t *plugin = (plugin_t*)(plugins->items[i]);
				ImGui::Text("\t%s", plugin->name);
				StringBuilder features;
				bool any = false;
				for (const char *tag : listed_tags) {
					if (plugin->supports(tag)) {
						features.AppendFormat(32, "%s, ", tag);
						any = true;
					}
				}
				if (any) {
					features.PopBack();
					features.PopBack();
					ImGui::Text("\t\tKnown features: %s", features.AsCStr());
				}
				if (plugin->supports("Shell")) {
					//~ Array<String> commands = dynamic_cast<ShellPlugin*>(plugins.ValueAtIndex(i))->getCommands();
					//~ StringBuilder c;
					//~ for (String _c : commands)
						//~ c.AppendFormat(512, "%s ", _c.AsCStr());
					//~ if (c.Length()) {
						//~ c.PopBack();
						//~ ImGui::Text("\t\t\tShell Commands: %s", c.AsCStr());
					//~ }
					//~ ImGui::Text("\t\tSend a command to the shell: ");
					//~ static char buf[128];
					//~ if (ImGui::InputText("", buf, 128, ImGuiInputTextFlags_EnterReturnsTrue)) {
						//~ dynamic_cast<ShellPlugin*>(plugins.ValueAtIndex(i))->execute(buf);
						//~ strcpy(buf, "");
					//~ }
				}
				if (plugin->supports("CPU")) {
					list_t *registers;
					if (plugin->cpu->get_registers && (registers = plugin->cpu->get_registers())) {
						ImGui::Text("\t\t\tRegisters:");
						int per_line = 1;
						for (int i = 5; i > 1; i--) {
							if (registers->length % i == 0) {
								per_line = i;
								break;
							}
						}
						StringBuilder line("\t\t\t\t");
						for (int i = 0; i < registers->length; i++) {
							register_value_t *reg = (register_value_t*)registers->items[i];
							line.AppendFormat(32, "%s = %d,", reg->name, reg->value);
							if (i % per_line == per_line - 1) {
								ImGui::Text("%s", line.AsCStr());
								line.Set("\t\t\t\t");
							}
						}
						list_free(registers);
					}
					else {
						ImGui::Text("\t\t\tThis CPU does not expose its registers");
					}
				}
				if (plugin->supports("Toolchain")) {
					//~ ImGui::Text("\t\t\tTransforms:");
					//~ Map<String, Array<String>> transforms = dynamic_cast<ToolchainPlugin*>(plugins.ValueAtIndex(i))->supportedTransforms();
					//~ for (int i = 0; i < transforms.Size(); i++) {
						//~ StringBuilder line("\t\t\t\t");
						//~ line.Append(transforms.KeyAtIndex(i));
						//~ line.Append(" -> ");
						//~ for (String s : transforms.ValueAtIndex(i)) {
							//~ line.Append(s);
							//~ line.Append(", ");
						//~ }
						//~ ImGui::Text("%s", line.GetSubString(0, line.Length() - 2).AsCStr());
					//~ }
				}
			}
		}
		ImGui::End();
	}
	list_t *perpetuals = get_plugins("Perpetual");
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

AppState::Code Zany80::OnCleanup() {
	//TODO: free plugins
	Input::Discard();
	Gfx::Discard();
	IO::Discard();
	return App::OnCleanup();
}

#if _DEBUG
static zany_loglevel current_level = ZL_DEBUG;
#else
static zany_loglevel current_level = ZL_INFO;
#endif

void zany_log(zany_loglevel level, const char *format, ...) {
	if (level < current_level) {
		return;
	}
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	fflush(stderr);
	va_end(args);
}

char *zany_root_folder() {
	StringBuilder s = IO::ResolveAssigns("root:");
	if (s.GetSubString(0, 4) == "file")
		s.Set(s.GetSubString(8, EndOfString));
	return strdup(s.AsCStr());
}