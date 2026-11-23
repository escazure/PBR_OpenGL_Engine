#pragma once

#include "gl3w.h"
#include <glfw/glfw3.h>

#include "resourceManager.h"
#include "model.h"
#include "shader.h"
#include "camera.h"
#include "texture.h"
#include "light.h"

#include <unordered_map>
#include <memory>
#include <functional>

#define PBRENGINE_INITIAL_LIGHT_CAPACITY 16

class Engine {
private: 
	GLFWwindow* _window = nullptr;
	std::vector<Light> _lights;
	Camera* _activeCamera = nullptr;

	std::function<void()> _renderCallback;

	GLuint _lightUBO = 0;
	unsigned int _lightBufferCapacity = 0;
	unsigned int _maximumLightBufferCapacity = 0;
	bool _lightsDirty = true;

	float _deltaTime = 0.0f;

	void _initializeLightBuffer();
	void _resizeLightBuffer(unsigned int capacity);
	void _uploadLightData();

	void _updateDeltaTime();

public:
	bool init();
	void run();
	void shutdown();

	void setRenderCallback(std::function<void()> renderCallback);
	
	void processTextures(Shader* shader, const std::vector<Texture*>& inputTextures, const std::vector<Texture*>& outputTextures, unsigned int textureWidth, unsigned int textureHeight, unsigned int textureDepth);

	void setActiveCamera(Camera* camera);
	Camera* getActiveCamera();

	void addLight(const Light& light);
	void removeLight(unsigned int index);
	void updateLight(unsigned int index, const Light& light);

	float getAspectRatio();
	float getDeltaTime();
	float getTime();
};

void openGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
