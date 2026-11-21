#pragma once

#include "gl3w.h"
#include <glfw/glfw3.h>

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
	std::unordered_map<std::string, std::unique_ptr<Model>> _models;
	std::unordered_map<std::string, std::unique_ptr<Shader>> _shaders;
	std::unordered_map<std::string, std::unique_ptr<Camera>> _cameras;
	std::unordered_map<std::string, std::unique_ptr<Texture>> _textures;
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
	
	void loadModel(const std::string& modelName, const std::string& objPath, const std::string& mtlPath = "");
	void loadShader(const std::string& shaderName, const std::string& vertexPath, const std::string& fragmentPath);
	void loadShader(const std::string& shaderName, const std::string& computePath);
	void loadTexture(const std::string& textureName, const std::string& texturePath);
	void loadTextures(const std::string& textureName, const std::vector<std::string>& texturePaths);

	void processTextures(const std::string& computeShaderName, const std::vector<std::string>& inputTextures, const std::vector<std::string>& outputTextures, unsigned int textureWidth, unsigned int textureHeight, unsigned int textureDepth);

	template<typename... Args>
	void addCamera(const std::string& cameraName, Args&&... args){
		_cameras[cameraName] = std::make_unique<Camera>(std::forward<Args>(args)...);
	}
	void setActiveCamera(const std::string& cameraName);
	Camera* getActiveCamera();

	template<typename... Args>
	void addTexture(const std::string& textureName, Args&&... args){
		_textures[textureName] = std::make_unique<Texture>(std::forward<Args>(args)...);
	}

	void addLight(const Light& light);
	void removeLight(unsigned int index);
	void updateLight(unsigned int index, const Light& light);

	void removeModel(const std::string& modelName);
	void removeShader(const std::string& shaderName);
	void removeTexture(const std::string& textureName);
	void removeCamera(const std::string& cameraName);

	Model* getModel(const std::string& modelName);
	Shader* getShader(const std::string& shaderName);
	Texture* getTexture(const std::string& textureName);
	Camera* getCamera(const std::string& cameraName);

	float getAspectRatio();
	float getDeltaTime();
	float getTime();

	// Move to a submodule 
	void prefilterEnvMap(unsigned int baseResolution, unsigned int maxMipLevels);
};

void openGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
