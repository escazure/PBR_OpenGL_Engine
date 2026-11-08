#include "engine.h"
#include "input.h"
#include "utils.h"
#include "stb_image.h"

bool Engine::init(){
	Logger::Log("Engine::init", "Started Initialization");

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

	glCreateBuffers(1, &_lightUBO);
	glNamedBufferStorage(_lightUBO, sizeof(Light) * 16, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, 1, _lightUBO, 0, sizeof(Light) * 16);

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	Logger::Success("Engine::init", "Done");
	return true;
}

void Engine::run(){
	Logger::Log("Engine::run", "Started render loop");
	// Do something with those //
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// Let user set those //
	const float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	const float clearDepth = 1.0f;

	float lastFrame = 0.0f;
	while(!glfwWindowShouldClose(_window)){	
		// Extract into separate function //
		float currentFrame = static_cast<float>(glfwGetTime());
		float deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Update input state and poll events //
		Input::updateState();
		glfwPollEvents();

		// Extract into separate function //
		if(!_lights.empty()) glNamedBufferSubData(_lightUBO, 0, sizeof(Light) * _lights.size(), _lights.data());
		
		// For now camera is updated on engine side explicitly //
		_cameras["MainCamera"]->update(deltaTime);

		// User should be able to set clear color and clear depth values //
		glClearNamedFramebufferfv(0, GL_COLOR, 0, clearColor);
		glClearNamedFramebufferfv(0, GL_DEPTH, 0, &clearDepth);

		// Invoke user callback if exists //
		if(_renderCallback){
			_renderCallback();
		}

		// Swap buffers //
		glfwSwapBuffers(_window);
	}	
}

void Engine::shutdown(){
	Logger::Log("Engine::shutdown", "Shutting down");
	glfwDestroyWindow(_window);
	glfwTerminate();
}

void Engine::setRenderCallback(std::function<void()> renderCallback){
	_renderCallback = std::move(renderCallback);
}

Model* Engine::getModel(const std::string& modelName){
	if(_models.find(modelName) != _models.end())
		return _models[modelName].get();

	Logger::Warning("Engine::getModel", "Model not found: " + modelName);
	return nullptr;
}

Shader* Engine::getShader(const std::string& shaderName){
	if(_shaders.find(shaderName) != _shaders.end())
		return _shaders[shaderName].get();

	Logger::Warning("Engine::getShader", "Shader not found: " + shaderName);
	return nullptr;
}

Texture* Engine::getTexture(const std::string& textureName){
	if(_textures.find(textureName) != _textures.end())
		return _textures[textureName].get();

	Logger::Warning("Engine::getTexture", "Texture not found: " + textureName);
	return nullptr;
}

Camera* Engine::getCamera(const std::string& cameraName){
	if(_cameras.find(cameraName) != _cameras.end())
		return _cameras[cameraName].get();

	Logger::Warning("Engine::getCamera", "Camera not found: " + cameraName);
	return nullptr;
}

void Engine::loadModel(const std::string& modelName, const std::string& objPath, const std::string& mtlPath){
	_models[modelName] = std::make_unique<Model>(objPath, mtlPath);
	Logger::Success("Engine::loadModel", "Loaded model from path: " + objPath);
}

void Engine::loadShader(const std::string& shaderName, const std::string& vertexPath, const std::string& fragmentPath){
	_shaders[shaderName] = std::make_unique<Shader>(shaderName, vertexPath, fragmentPath);	
	Logger::Success("Engine::loadShader", "Loaded vertex/fragment shaders from paths: " + vertexPath + " | " + fragmentPath);
}

void Engine::loadShader(const std::string& shaderName, const std::string& computePath){
	_shaders[shaderName] = std::make_unique<Shader>(shaderName, computePath);	
	Logger::Success("Engine::loadShader", "Loaded compute shader from path: " + computePath);
}

// Rename and remake loading functions for different types of textures //
void Engine::loadTexture(const std::string& textureName, const std::string& texturePath){
	std::size_t pointIdx = texturePath.find_last_of(".");
	if(pointIdx == std::string::npos){
		Logger::Error("Engine::loadTexture", "Texture name doesn't contain file format: " + texturePath);
		return;
	}
	std::string fileFormat = Utils::toLower(texturePath.substr(pointIdx + 1));
	bool isHDR = (fileFormat == "hdr" || fileFormat == "exr");

	int width, height, channels;
	GLuint internalFormat;

	stbi_set_flip_vertically_on_load(true);

	void* data = isHDR ? static_cast<void*>(stbi_loadf(texturePath.c_str(), &width, &height, &channels, 0)) :
						 static_cast<void*>(stbi_load(texturePath.c_str(), &width, &height, &channels, 0));

	if(!data){
		const char* failureReason = stbi_failure_reason();
    	std::string errorDetails = failureReason ? failureReason : "Unknown error";

		Logger::Error("Engine::loadTexture", "Failed to load " + std::string(isHDR ? " HDR " : "") + " texture image from path: " + texturePath + " | Reason: " + errorDetails);
		return;
	}

	switch(channels){
		case 1: internalFormat = isHDR ? GL_R16F : GL_R8; break;
		case 2: internalFormat = isHDR ? GL_RG16F : GL_RG8; break;
		case 3: internalFormat = isHDR ? GL_RGB16F : GL_RGB8; break;
		case 4: internalFormat = isHDR ? GL_RGBA16F : GL_RGBA8; break;
		default: internalFormat = isHDR ? GL_RGB16F : GL_RGB8;
	}

	auto texture = std::make_unique<Texture>(GL_TEXTURE_2D, internalFormat, 1, width, height);
	texture->loadPixelsSingle(width, height, data);
	if(isHDR)
		texture->setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	else
		texture->setWrap(GL_REPEAT, GL_REPEAT);

	texture->setFilter(GL_LINEAR, GL_LINEAR);

	stbi_image_free(data);
	_textures[textureName] = std::move(texture);
	Logger::Success("Engine::loadTexture", "Loaded " + std::string(isHDR ? " HDR " : " ") + "texture image from path: " + texturePath);
}

// Update this //
void Engine::loadTextures(const std::string& textureName, const std::vector<std::string>& texturePaths){
	Logger::Log("Engine::loadTextures", "Loading texture images from paths: " + texturePaths[0] + ", ... ");
	int width, height, channels;
	std::vector<unsigned char*> rawFaces(texturePaths.size(), nullptr);

	stbi_set_flip_vertically_on_load(true);

	GLuint internalFormat;

	for(int i = 0; i < texturePaths.size(); i++){
		int w, h, c;
		rawFaces[i] = stbi_load(texturePaths[i].c_str(), &w, &h, &c, 0);
		if(!rawFaces[i]){
			Logger::Error("Engine::loadTextures", "Failed to load texture image from path: " + texturePaths[i]);
			for(int j = 0; j <= i; j++) if(rawFaces[j]) stbi_image_free(rawFaces[j]);
			return;
		}

		if(i == 0){
			width = w;
			height = h;
			channels = c;
		}
	}
	
	switch(channels){
		case 1: internalFormat = GL_R8; break;
		case 2: internalFormat = GL_RG8; break;
		case 3: internalFormat = GL_RGB8; break;
		case 4: internalFormat = GL_RGBA8; break;
	}

	auto texture = std::make_unique<Texture>(GL_TEXTURE_CUBE_MAP, internalFormat, 1, width, height);
	std::vector<const void*> faceDataSources(rawFaces.begin(), rawFaces.end());
	texture->loadPixelsMultiple(faceDataSources, width, height);
	texture->setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	texture->setFilter(GL_LINEAR, GL_LINEAR);

	for(auto ptr: rawFaces) if(ptr) stbi_image_free(ptr);

	_textures[textureName] = std::move(texture);
	Logger::Success("Engine::loadTextures", "Done");
}

void Engine::addLight(const Light& light){
	_lights.push_back(light);
}

float Engine::getAspectRatio(){
	int width, height;
	glfwGetFramebufferSize(_window, &width, &height);
	if(height == 0) return 1.0f;
	return static_cast<float>(width)/static_cast<float>(height);
}

void Engine::processTextures(const std::string& computeShaderName, const std::vector<std::string>& inputTextures, const std::vector<std::string>& outputTextures, unsigned int textureWidth, unsigned int textureHeight, unsigned int textureDepth){
	if(_shaders.find(computeShaderName) == _shaders.end()){
		Logger::Error("Engine::processTextures", "Compute shader not found: " + computeShaderName);
		return;
	}

	for(const auto& inTexture : inputTextures){
		for(const auto& outTexture : outputTextures){
			if(inTexture == outTexture){
				Logger::Error("Engine::processTextures", "Texture '" + inTexture + "' can't be bound as both input and output");
				return;
			}
		}
	}

	_shaders[computeShaderName]->use();

	for(std::size_t i = 0; i < inputTextures.size(); i++)
		_textures[inputTextures[i]]->bind(i);

	for(std::size_t i = 0; i < outputTextures.size(); i++)
		_textures[outputTextures[i]]->bindAsImage(i);

	unsigned int numGroupX = (textureWidth + 15) / 16;
	unsigned int numGroupY = (textureHeight + 15) / 16;
	unsigned int numGroupZ = textureDepth;

	glDispatchCompute(numGroupX, numGroupY, numGroupZ);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

// Figure out how to remove this later 
void Engine::prefilterEnvMap(unsigned int baseResolution, unsigned int maxMipLevels){
	_shaders["prefilterCubemap"]->use();
	_textures["EnvironmentCubemap"]->bind(0);
	
	for(unsigned int mip = 0; mip < maxMipLevels; mip++){
		unsigned int mipResolution = baseResolution >> mip;
		float roughness = static_cast<float>(mip) / static_cast<float>(maxMipLevels - 1);
		_shaders["prefilterCubemap"]->setFloat("uRoughness", roughness);
		_textures["PrefilterEnvCubemap"]->bindAsImage(0, mip);
		
		unsigned int numGroupX = (mipResolution + 15) / 16;
		unsigned int numGroupY = (mipResolution + 15) / 16;
		glDispatchCompute(numGroupX, numGroupY, 6);
	}

	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
