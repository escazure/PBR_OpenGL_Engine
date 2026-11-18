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

class Engine {
private: 
	GLFWwindow* _window = nullptr;
	std::unordered_map<std::string, std::unique_ptr<Model>> _models;
	std::unordered_map<std::string, std::unique_ptr<Shader>> _shaders;
	std::unordered_map<std::string, std::unique_ptr<Camera>> _cameras;
	std::unordered_map<std::string, std::unique_ptr<Texture>> _textures;
	std::vector<Light> _lights;

	std::function<void()> _renderCallback;

	GLuint _lightUBO = 0;

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

	template<typename... Args>
	void addTexture(const std::string& textureName, Args&&... args){
		_textures[textureName] = std::make_unique<Texture>(std::forward<Args>(args)...);
	}

	void addLight(const Light& light);

	Model* getModel(const std::string& modelName);
	Shader* getShader(const std::string& shaderName);
	Texture* getTexture(const std::string& textureName);
	Camera* getCamera(const std::string& cameraName);

	float getAspectRatio();

	// To be removed 
	void prefilterEnvMap(unsigned int baseResolution, unsigned int maxMipLevels);
};

void openGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
