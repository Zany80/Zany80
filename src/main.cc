#include <Zany80/Zany80.h>

#include <Gfx/Gfx.h>
#include <Gfx/private/displayMgr.h>
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
#if ORYOL_GLFW
#include <GLFW/glfw3.h>
#include <Zany80/glfw_icon.h>
#endif

#include <Zany80/Plugin.h>
#include <Zany80/API.h>
#include <sstream>

#include <config.h>

#if ORYOL_EMSCRIPTEN
#include "../plugins/simple_shell/simple_shell.h"
#include "../plugins/editor/Editor.h"
#include "../plugins/assembler/scas/scas.h"
#include "../build/z80cpp_core/src/oryolized/ZanyCore.h"
#endif

unsigned long plugin_instances;

Zany80 *zany;

OryolMain(Zany80);

Array<String> available_plugins;

#ifndef ORYOL_EMSCRIPTEN
void updateAvailablePlugins() {
	available_plugins = readDirectory(processFileURI("plugins:"));
	for (int i = 0; i < available_plugins.Size(); i++) {
		StringBuilder s = available_plugins[i];
		int index = s.FindLastOf(0, EndOfString, ".");
		if (index != InvalidIndex) {
			available_plugins[i] = s.GetSubString(0, index);
		}
	}
}
#endif

#if ORYOL_GLFW
void Zany80::Fullscreen() {
	int monitor_count;
	GLFWmonitor **monitors = glfwGetMonitors(&monitor_count);
	o_assert (monitor_count != 0);
	const GLFWvidmode* mode = glfwGetVideoMode(monitors[0]);
	if (this->fullscreen) {
		int width, height, xpos, ypos;
		glfwGetWindowSize(Oryol::_priv::glfwDisplayMgr::glfwWindow, &width, &height);
		glfwGetWindowPos(Oryol::_priv::glfwDisplayMgr::glfwWindow, &xpos, &ypos);
		windowed_position = glm::vec4(xpos, ypos, width, height);
		glfwSetWindowMonitor(Oryol::_priv::glfwDisplayMgr::glfwWindow, monitors[0], 0, 0, mode->width, mode->height, mode->refreshRate);
	}
	else {
		glfwSetWindowMonitor(Oryol::_priv::glfwDisplayMgr::glfwWindow, NULL, windowed_position.x, windowed_position.y, windowed_position.z, windowed_position.w, mode->refreshRate);
	}
}
#endif

extern "C" void reportError(const char *error_message) {
	zany->reportError(error_message);
}

void Zany80::reportError(const char *error) {
	StringBuilder s(this->error);
	if (s.Length())
		s.Append('\n');
	s.Append(error);
	this->error = s.GetString();
}

AppState::Code Zany80::OnInit() {
	plugin_instances = 0;
	zany = this;
#if ORYOL_GLFW
	this->fullscreen = false;
#endif
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
#if ORYOL_GLFW
	glfwSetWindowIcon(Oryol::_priv::glfwDisplayMgr::glfwWindow, 1, &icon);
#endif
#if ORYOL_LINUX
	StringBuilder s = IO::ResolveAssigns("root:");
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
	#ifndef ORYOL_EMSCRIPTEN
	IO::SetAssign("lib:", "root:lib/");
	updateAvailablePlugins();
	#else
	plugins.Add("SimpleShell", new SimpleShell);
	plugins.Add("ZanyCore", new ZanyCore);
	plugins.Add("Official Assembler", new scas);
	plugins.Add("Editor", new Editor);
	IO::Load("root:libc.o", [](IO::LoadResult res) {
		 mkdir("/lib", 0700);
		 FILE *file = fopen("/lib/libc.o", "w");
		 if (file) {
			 fwrite(res.Data.Data(), 1, res.Data.Size(), file);
			 fflush(file);
			 fclose(file);
	 		(CAST_PPTR(plugins["SimpleShell"], ShellPlugin))->output("/lib/libc.o saved!");
		 }
		 else {
	 		(CAST_PPTR(plugins["SimpleShell"], ShellPlugin))->output("Error opening /lib/libc.o for writing!");
	 		::reportError("Error opening /lib/libc.o for writing!");
		 }
	}, [](const URL& url, IOStatus::Code ioStatus) {
		(CAST_PPTR(plugins["SimpleShell"], ShellPlugin))->output("Error loading libc into emscripten!");
		::reportError("Error loading libc!");
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
	debug_window = hub = true;
	this->tp = Clock::Now();
	// This doesn't work - more accurately, ImGui ignores it.
	// Furthermore, it crashes the GCC 4.8 build, so it's not worth including.
	//~ Gfx::Subscribe([](GfxEvent event) {
		//~ switch (event.Type) {
			//~ case GfxEvent::Type::DisplayModified:
				//~ Log::Dbg("New size: %d x %d\n",event.DisplayAttrs.WindowWidth,event.DisplayAttrs.WindowHeight);
				//~ ImGui::GetIO().DisplaySize = ImVec2(event.DisplayAttrs.WindowWidth,event.DisplayAttrs.WindowHeight);
				//~ break;
			//~ default:
				//~ break;
		//~ }
	//~ });
	return App::OnInit();
}

AppState::Code Zany80::OnRunning() {
	Duration delta = Clock::LapTime(this->tp);
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
			ImGui::Checkbox("Show Debug Window", &this->debug_window);
			ImGui::Checkbox("Show Plugin Hub", &this->hub);
			#if ORYOL_GLFW
			if (ImGui::Checkbox("Fullscreen Mode", &this->fullscreen))
				this->Fullscreen();
			#endif
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
	if (this->debug_window) {
		ImGui::Begin("Debug", NULL, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Native Edition");
		ImGui::Text("Zany80 version: " PROJECT_VERSION);
		ImGui::Text("Framerate: %.1f", ImGui::GetIO().Framerate > 60 ? 60 : ImGui::GetIO().Framerate);
		#if ORYOL_GLFW
		ImGui::Text("Backend: OpenGL");
		#elif ORYOL_D3D11
		ImGui::Text("Backend: D3D11");
		#elif ORYOL_EMSCRIPTEN
		ImGui::Text("Backend: Emscripten");
		#else
		ImGui::Text("Unknown backend. This platform may not be fully supported.");
		#endif
		if (ImGui::Button("Hide")) {
			debug_window = false;
		}
		ImGui::End();
	}
	if (error != "") {
		ImGui::Begin("Error!", NULL, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("%s", error.AsCStr());
		if (ImGui::Button("Clear"))
			error = "";
		ImGui::End();
	}
	if (this->hub) {
		static bool loaded_plugins = true;
		#ifndef ORYOL_EMSCRIPTEN
		static bool show_plugins = true;
		#endif
		ImGui::Begin("Plugin Hub", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("View")) {
				ImGui::Checkbox("Loaded plugins", &loaded_plugins);
				#ifndef ORYOL_EMSCRIPTEN
				ImGui::Checkbox("Show available plugins", &show_plugins);
				#endif
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		#ifndef ORYOL_EMSCRIPTEN
		if (show_plugins) {
			ImGui::Text("Available plugins: %d", available_plugins.Size() - plugins.Size());
			for (int i = 0, c = 1; i < available_plugins.Size(); i++) {
				StringBuilder s = "plugins:";
				s.Append(available_plugins[i]);
				String path = s.GetString();
				if (plugins.FindIndex(path) == InvalidIndex) {
					ImGui::Text("%d: %s", c++, available_plugins[i].AsCStr());
					s.Set("Load ");
					s.Append(available_plugins[i]);
					if (ImGui::Button(s.GetString().AsCStr())) {
						Log::Dbg("Spawning %s\n", path.AsCStr());
						spawnPlugin(path);
					}
				}
			}
		}
		#endif
		if (loaded_plugins) {
			constexpr const char *listed_tags[] = {
				"CPU", "Perpetual", "Shell", "z80", "z80/im0", "z80/im1", "z80/im2", "Toolchain"
			};
			ImGui::Text("Plugins");
			for (int i = 0; i < plugins.Size(); i++) {
				ImGui::Text("\t%s", plugins.KeyAtIndex(i).AsCStr());
				StringBuilder features	;
				bool any = false;
				for (const char *tag : listed_tags) {
					if (GET_PPTR(plugins.ValueAtIndex(i))->supports(tag)) {
						features.AppendFormat(32, "%s, ", tag);
						any = true;
					}
				}
				if (any) {
					features.PopBack();
					features.PopBack();
					ImGui::Text("\t\tKnown features: %s", features.AsCStr());
				}
				if (GET_PPTR(plugins.ValueAtIndex(i))->supports("Shell")) {
					Array<String> commands = CAST_PPTR(plugins.ValueAtIndex(i), ShellPlugin)->getCommands();
					StringBuilder c;
					for (String _c : commands)
						c.AppendFormat(512, "%s ", _c.AsCStr());
					if (c.Length()) {
						c.PopBack();
						ImGui::Text("\t\t\tShell Commands: %s", c.AsCStr());
					}
					ImGui::Text("\t\tSend a command to the shell: ");
					static char buf[128];
					if (ImGui::InputText("", buf, 128, ImGuiInputTextFlags_EnterReturnsTrue)) {
						CAST_PPTR(plugins.ValueAtIndex(i), ShellPlugin)->execute(buf);
						strcpy(buf, "");
					}
				}
				if (GET_PPTR(plugins.ValueAtIndex(i))->supports("CPU")) {
					ImGui::Text("\t\t\tRegisters:");
					int per_line = 1;
					Map<String, uint32_t> registers = CAST_PPTR(plugins.ValueAtIndex(i), CPUPlugin)->getRegisters();
					for (int i = 5; i > 1; i--) {
						if (registers.Size() % i == 0) {
							per_line = i;
							break;
						}
					}
					StringBuilder line("\t\t\t\t");
					for (int i = 0; i < registers.Size(); i++) {
						String key = registers.KeyAtIndex(i);
						uint32_t value = registers.ValueAtIndex(i);
						line.AppendFormat(32, "%s = %d,", key.AsCStr(), value);
						if (i % per_line == per_line - 1) {
							ImGui::Text("%s", line.AsCStr());
							line.Set("\t\t\t\t");
						}
					}
				}
				if (GET_PPTR(plugins.ValueAtIndex(i))->supports("Toolchain")) {
					ImGui::Text("\t\t\tTransforms:");
					Map<String, Array<String>> transforms = CAST_PPTR(plugins.ValueAtIndex(i), ToolchainPlugin)->supportedTransforms();
					for (int i = 0; i < transforms.Size(); i++) {
						StringBuilder line("\t\t\t\t");
						line.Append(transforms.KeyAtIndex(i));
						line.Append(" -> ");
						for (String s : transforms.ValueAtIndex(i)) {
							line.Append(s);
							line.Append(", ");
						}
						ImGui::Text("%s", line.GetSubString(0, line.Length() - 2).AsCStr());
					}
				}
			}
		}
		ImGui::End();
	}
	for (auto pair : plugins) {
		if (pair.value->supports("Perpetual")) {
			CAST_PPTR(pair.value, PerpetualPlugin)->frame(delta.AsSeconds());
		}
	}
	ImGui::Render();
	Gfx::EndPass();
	Gfx::CommitFrame();
	return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

AppState::Code Zany80::OnCleanup() {
	for (KeyValuePair<String, PPTR> p : plugins) {
		CLEANUP_PPTR(p.value);
	}
	plugins.Clear();
	IMUI::Discard();
	Input::Discard();
	Gfx::Discard();
	IO::Discard();
	return App::OnCleanup();
}
