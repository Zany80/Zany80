#pragma once

#include <Core/Main.h>
#include <Core/String/StringBuilder.h>
#include <Core/Time/Clock.h>
#include <glm/glm.hpp>

#define ORYOL_GLFW (!(ORYOL_D3D11 || ORYOL_METAL) && (ORYOL_WINDOWS || ORYOL_MACOS || ORYOL_LINUX))

using namespace Oryol;

class Zany80 : public App {
public:
	Zany80();
	AppState::Code OnInit();
	AppState::Code OnRunning();
	AppState::Code OnCleanup();
	void report_error(const char *errorMessage);
private:
	void ToggleFullscreen();
	bool is_fullscreen;
	bool show_debug_window, hub;
	TimePoint tp;
	String error;
};
extern Zany80 *zany;

void SetupIcon();
