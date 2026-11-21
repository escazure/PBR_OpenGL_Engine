#include "engine.h"
#include "input.h"
#include "utils.h"

#include <string>

bool Engine::init(){
	Logger::Info("Engine::init", "Started Initialization");

	// Window and Context initialization //
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	_window = glfwCreateWindow(mode->width, mode->height, "PBR", NULL, NULL);

	glfwMakeContextCurrent(_window);
	glfwSetKeyCallback(_window, Input::keyCallback);
	glfwSetCursorPosCallback(_window, Input::mouseCallback);

	// Should probably extract into separate function when introduce GUI //
	glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	if(gl3wInit() != 0){
		Logger::Error("GL3W::gl3wInit", "Failed to init");
		return false;
	}

	#ifdef ENABLE_OPENGL_DEBUG
		Logger::Info("Engine::init", "GL_DEBUG_OUTPUT is enabled.");
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(openGLDebugCallback, nullptr);
	#endif

	GLint maxUBOSize = 0;
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize);
	_maximumLightBufferCapacity = maxUBOSize / sizeof(Light);

	_initializeLightBuffer();

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	Logger::Success("Engine::init", "Done");
	return true;
}

void Engine::run(){
	Logger::Info("Engine::run", "Started render loop");
	// Do something with those //
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// Let user set those //
	const float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	const float clearDepth = 1.0f;

	while(!glfwWindowShouldClose(_window)){	
		_updateDeltaTime();

		// Update input state and poll events //
		Input::updateState();
		glfwPollEvents();

		// Upload light data only if something changed //
		if(_lightsDirty) _uploadLightData();

		// For now only active camera is updated //
		if(_activeCamera) _activeCamera->update(_deltaTime);

		// Clear color and depth buffers //
		glClearNamedFramebufferfv(0, GL_COLOR, 0, clearColor);
		glClearNamedFramebufferfv(0, GL_DEPTH, 0, &clearDepth);

		// Invoke user callback if exists //
		if(_renderCallback) _renderCallback();

		// Swap buffers //
		glfwSwapBuffers(_window);
	}	
}

void Engine::shutdown(){
	Logger::Info("Engine::shutdown", "Shutting down");
	ResourceManager::Clear();
	glfwDestroyWindow(_window);
	glfwTerminate();
}

void Engine::setRenderCallback(std::function<void()> renderCallback){
	_renderCallback = std::move(renderCallback);
}

void Engine::addLight(const Light& light){
	_lights.push_back(light);
	_lightsDirty = true;
}

void Engine::updateLight(unsigned int index, const Light& light){
	_lights[index] = light;
	_lightsDirty = true;
}

void Engine::removeLight(unsigned int index){
	_lights.erase(_lights.begin() + index);
	_lightsDirty = true;
}

float Engine::getAspectRatio(){
	int width, height;
	glfwGetFramebufferSize(_window, &width, &height);
	if(height == 0) return 1.0f;
	return static_cast<float>(width)/static_cast<float>(height);
}

void Engine::processTextures(Shader* shader, const std::vector<Texture*>& inputTextures, const std::vector<Texture*>& outputTextures, unsigned int textureWidth, unsigned int textureHeight, unsigned int textureDepth){
	for(const auto& inTexture : inputTextures){
		for(const auto& outTexture : outputTextures){
			if(inTexture == outTexture){
				Logger::Error("Engine::processTextures", "Textures can't be bound as both input and output");
				return;
			}
		}
	}

	shader->use();

	for(std::size_t i = 0; i < inputTextures.size(); i++)
		inputTextures[i]->bind(i);

	for(std::size_t i = 0; i < outputTextures.size(); i++)
		outputTextures[i]->bindAsImage(i);

	unsigned int numGroupX = (textureWidth + 15) / 16;
	unsigned int numGroupY = (textureHeight + 15) / 16;
	unsigned int numGroupZ = textureDepth;

	glDispatchCompute(numGroupX, numGroupY, numGroupZ);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

// Figure out how to remove this later 
void Engine::prefilterEnvMap(unsigned int baseResolution, unsigned int maxMipLevels){
	Shader* shader = ResourceManager::GetShader("PrefilterCube");
	shader->use();
	ResourceManager::GetTexture("EnvironmentCubemap")->bind(0);
	
	for(unsigned int mip = 0; mip < maxMipLevels; mip++){
		unsigned int mipResolution = baseResolution >> mip;
		float roughness = static_cast<float>(mip) / static_cast<float>(maxMipLevels - 1);
		shader->setFloat("uRoughness", roughness);
		ResourceManager::GetTexture("PrefilterEnvCubemap")->bindAsImage(0, mip);
		
		unsigned int numGroupX = (mipResolution + 15) / 16;
		unsigned int numGroupY = (mipResolution + 15) / 16;
		glDispatchCompute(numGroupX, numGroupY, 6);
	}

	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void Engine::setActiveCamera(Camera* camera){
	_activeCamera = camera;
}

Camera* Engine::getActiveCamera(){
	if(_activeCamera)
		return _activeCamera;

	Logger::Warning("Engine::getActiveCamera", "Active camera not set.");
	return nullptr;
}

void Engine::_initializeLightBuffer(){
	glCreateBuffers(1, &_lightUBO);
	_lightBufferCapacity = PBRENGINE_INITIAL_LIGHT_CAPACITY;
	glNamedBufferData(_lightUBO, sizeof(Light) * _lightBufferCapacity, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, _lightUBO);
}

void Engine::_resizeLightBuffer(unsigned int capacity){
	if(capacity <= _lightBufferCapacity) return;
	if(capacity >= _maximumLightBufferCapacity) Logger::Warning("Engine::_resizeLightBuffer", "Can't increase light buffer size, hit maximum amount of lights."); return;
	_lightBufferCapacity = capacity;
	glNamedBufferData(_lightUBO, sizeof(Light) * _lightBufferCapacity, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, _lightUBO);
}

void Engine::_uploadLightData(){
	if(_lights.empty()) return;
	if(_lights.size() >= _lightBufferCapacity) _resizeLightBuffer(_lights.size());
	glNamedBufferSubData(_lightUBO, 0, sizeof(Light) * _lights.size(), _lights.data());
}

void Engine::_updateDeltaTime(){
	static float lastFrame = 0;
	static float currentFrame = 0;

	currentFrame = getTime();
	_deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
}

float Engine::getDeltaTime(){
	return _deltaTime;
}

float Engine::getTime(){
	return static_cast<float>(glfwGetTime());
}

std::string getDebugSource(GLenum source){
	switch(source){
		case GL_DEBUG_SOURCE_API: return "API";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW_SYSTEM";
		case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER_COMPILER";
		case GL_DEBUG_SOURCE_THIRD_PARTY: return "THIRD_PARTY";
		case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
		case GL_DEBUG_SOURCE_OTHER: return "OTHER";
	}
	return "UNKNOWN";
}

std::string getDebugType(GLenum type){
	switch(type){
		case GL_DEBUG_TYPE_ERROR: return "ERROR";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED_BEHAVIOR";
		case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
		case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
		case GL_DEBUG_TYPE_MARKER: return "MARKER";
		case GL_DEBUG_TYPE_PUSH_GROUP: return "PUSH_GROUP";
		case GL_DEBUG_TYPE_POP_GROUP: return "POP_GROUP";
		case GL_DEBUG_TYPE_OTHER: return "OTHER";
	}
	return "UNKNOWN";
}

void openGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam){
	std::string logSource = "OPENGL_DEBUG_CALLBACK:" + getDebugSource(source);
	std::string logMessage = "[" + getDebugType(type) + "], ID[" + std::to_string(id) + "]: " + std::string(message);
	switch(severity){
		case GL_DEBUG_SEVERITY_HIGH:
			Logger::Error(logSource, logMessage);
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
		case GL_DEBUG_SEVERITY_LOW:
			Logger::Warning(logSource, logMessage);
			break;
		default:
			Logger::Info(logSource, logMessage);
			break;
	}
}
