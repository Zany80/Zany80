#pragma once

#include <Core/Main.h>
#include <Core/String/StringBuilder.h>
#include <Core/Time/Clock.h>
#include <glm/glm.hpp>

#define ORYOL_GLFW (!(ORYOL_D3D11 || ORYOL_METAL) && (ORYOL_WINDOWS || ORYOL_MACOS || ORYOL_LINUX))

using namespace Oryol;

class Zany80 : public App {
public:
	AppState::Code OnInit();
	AppState::Code OnRunning();
	AppState::Code OnCleanup();
	void reportError(const char *errorMessage);
private:
	#if ORYOL_GLFW
	void Fullscreen();
	bool fullscreen;
	#endif
	bool debug_window, hub;
	glm::vec4 windowed_position;
	TimePoint tp;
	String error;
};
extern Zany80 *zany;
