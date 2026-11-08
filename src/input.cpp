#include "input.h"
#include "logger.h"

std::unordered_map<int, bool> currentKeys;
std::unordered_map<int, bool> previousKeys;

glm::vec2 mousePosition;
glm::vec2 mouseDelta;

bool firstMouse = true;

void Input::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
	if(action == GLFW_PRESS) currentKeys[key] = true;
	if(action == GLFW_RELEASE) currentKeys[key] = false;
}

void Input::mouseCallback(GLFWwindow* window, double xpos, double ypos){
	glm::vec2 newPosition = {xpos, ypos};

	if(firstMouse){
		mousePosition = newPosition;
		firstMouse = false;
		return;
	}

	mouseDelta += newPosition - mousePosition;
	mousePosition = newPosition;	
}

bool Input::isKeyDown(int key){
	return currentKeys[key];
}

bool Input::isKeyPressed(int key){
	return currentKeys[key] && !previousKeys[key];
}

bool Input::isKeyReleased(int key){
	return !currentKeys[key] && previousKeys[key];
}

glm::vec2 Input::getMousePosition(){
	return mousePosition;
}

glm::vec2 Input::getMouseDelta(){
	return mouseDelta;
}

void Input::updateState(){
	previousKeys = currentKeys;
	mouseDelta = {0.0f, 0.0f};
}
