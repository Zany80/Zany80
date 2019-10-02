#pragma once

#include <Core/Main.h>
#include <Core/String/StringBuilder.h>
#include <Core/Time/Clock.h>
#include <glm/glm.hpp>

#define ORYOL_GLFW (!(ORYOL_D3D11 || ORYOL_METAL) && (ORYOL_WINDOWS || ORYOL_MACOS || ORYOL_LINUX))

using namespace Oryol;

class SIMPLE : public App {
public:
	SIMPLE();
	AppState::Code OnInit();
	AppState::Code OnRunning();
	AppState::Code OnCleanup();
	void report_error(const char *errorMessage);
	void ToggleFullscreen();
private:
	bool is_fullscreen;
	bool show_debug_window;
	TimePoint tp;
	String error;
};
extern SIMPLE *zany;

void SetupIcon();
