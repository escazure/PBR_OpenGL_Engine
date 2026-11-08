#pragma once

#include <glfw/glfw3.h>
#include <glm/glm.hpp>

#include <unordered_map>

namespace Input {
	void mouseCallback(GLFWwindow* window, double xpos, double ypos);	
	void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	bool isKeyDown(int key);
	bool isKeyPressed(int key);
	bool isKeyReleased(int key);

	glm::vec2 getMousePosition();
	glm::vec2 getMouseDelta();

	void updateState();
};
