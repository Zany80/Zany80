#include <Zany80/Zany80.h>
#include <GLFW/glfw3.h>
#include <Zany80/internal/glfw_icon.h>
#include <Gfx/private/displayMgr.h>

void SetupIcon() {
	glfwSetWindowIcon(Oryol::_priv::glfwDisplayMgr::glfwWindow, 1, &icon);
}

void Zany80::ToggleFullscreen() {
	static glm::vec4 windowed_position;
	int monitor_count;
	GLFWmonitor **monitors = glfwGetMonitors(&monitor_count);
	o_assert (monitor_count != 0);
	const GLFWvidmode* mode = glfwGetVideoMode(monitors[0]);
	if (this->is_fullscreen) {
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
