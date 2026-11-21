#include <filesystem>

#include "stb_image.h"

#include "resourceManager.h"
#include "utils.h"
#include "model.h"
#include "shader.h"
#include "camera.h"
#include "texture.h"
#include "logger.h"
#include "utils.h"

std::unordered_map<std::string, std::unique_ptr<Model>> ResourceManager::_models;
std::unordered_map<std::string, std::unique_ptr<Shader>> ResourceManager::_shaders;
std::unordered_map<std::string, std::unique_ptr<Camera>> ResourceManager::_cameras;
std::unordered_map<std::string, std::unique_ptr<Texture>> ResourceManager::_textures;

bool ResourceManager::_measureLoadingTime;

Model* ResourceManager::LoadModel(const std::string& modelName, const std::string& objPath, const std::string& mtlPath){
	Utils::Timer timer;
	auto it = _models.find(modelName);
	if(it != _models.end()) return it->second.get();

	if(_measureLoadingTime) timer.Start();
	auto model = std::make_unique<Model>(objPath, mtlPath);
	Model* ptr = model.get();
	_models.emplace(modelName, std::move(model));
	if(_measureLoadingTime) timer.Stop();

	Logger::Success("ResourceManager::LoadModel", "Loaded model '" + modelName + "'" + (_measureLoadingTime ? " in " + std::to_string(timer.GetElapsed()) + "ms" : "") + ".");
	return ptr;
}

Model* ResourceManager::GetModel(const std::string& modelName){
	auto it = _models.find(modelName);
	if(it == _models.end()){
		Logger::Warning("ResourceManager::GetModel", "Model '" + modelName + "' not found.");
		return nullptr;
	}
	return it->second.get();
}

void ResourceManager::UnloadModel(const std::string& modelName){
	auto it = _models.find(modelName);
	if(it == _models.end()){
		Logger::Warning("ResourceManager::UnloadModel", "Model '" + modelName + "' not found.");
		return;
	}
	_models.erase(it);
}

Shader* ResourceManager::LoadShader(const std::string& shaderName, const std::string& computeShaderPath){
	Utils::Timer timer;
	auto it = _shaders.find(shaderName);
	if(it != _shaders.end()) return it->second.get();

	if(_measureLoadingTime) timer.Start();
	auto shader = std::make_unique<Shader>(computeShaderPath);
	Shader* ptr = shader.get();
	_shaders.emplace(shaderName, std::move(shader));
	if(_measureLoadingTime) timer.Stop();

	Logger::Success("ResourceManager::LoadShader", "Loaded compute shader '" + shaderName + "'" + (_measureLoadingTime ? " in " + std::to_string(timer.GetElapsed()) + "ms" : "") + ".");
	return ptr;
}

Shader* ResourceManager::LoadShader(const std::string& shaderName, const std::string& vertexShaderPath, const std::string& fragmentShaderPath){
	Utils::Timer timer;
	auto it = _shaders.find(shaderName);
	if(it != _shaders.end()) return it->second.get();

	if(_measureLoadingTime) timer.Start();
	auto shader = std::make_unique<Shader>(vertexShaderPath, fragmentShaderPath);
	Shader* ptr = shader.get();
	_shaders.emplace(shaderName, std::move(shader));
	if(_measureLoadingTime) timer.Stop();

	Logger::Success("ResourceManager::LoadShader", "Loaded shader '" + shaderName + "'" + (_measureLoadingTime ? " in " + std::to_string(timer.GetElapsed()) + "ms" : "") + ".");
	return ptr;
}

Shader* ResourceManager::GetShader(const std::string& shaderName){
	auto it = _shaders.find(shaderName);
	if(it == _shaders.end()){
		Logger::Warning("ResourceManager::GetShader", "Shader '" + shaderName + "' not found.");
		return nullptr;
	}
	return it->second.get();
}

void ResourceManager::UnloadShader(const std::string& shaderName){
	auto it = _shaders.find(shaderName);
	if(it == _shaders.end()){
		Logger::Warning("ResourceManager::UnloadShader", "Shader '" + shaderName + "' not found.");
		return;
	}
	_shaders.erase(it);
}

Texture* ResourceManager::LoadTexture(const std::string& textureName, const std::string& texturePath, bool flipVertically){
	Utils::Timer timer;
	auto it = _textures.find(textureName);
	if(it != _textures.end()) return it->second.get();
	
	std::string fileFormat = Utils::toLower(std::filesystem::path(texturePath).extension());
	if(fileFormat.empty()){
		Logger::Error("ResourceManager::LoadTexture", "Texture name doesn't contain file format '" + texturePath + "'.");
		return nullptr;
	}
	bool isHDR = (fileFormat == ".hdr");

	int width, height, channels;
	GLuint internalFormat;

	stbi_set_flip_vertically_on_load(flipVertically);

	if(_measureLoadingTime) timer.Start();
	void* data = isHDR ? static_cast<void*>(stbi_loadf(texturePath.c_str(), &width, &height, &channels, 0)) :
						 static_cast<void*>(stbi_load(texturePath.c_str(), &width, &height, &channels, 0));

	if(!data){
		const char* failureReason = stbi_failure_reason();
    	std::string errorDetails = failureReason ? failureReason : "Unknown error";

		Logger::Error("ResourceManager::LoadTexture", "Failed to load " + std::string(isHDR ? "HDR" : "") + " texture image from path '" + texturePath + "' | Reason: " + errorDetails);
		return nullptr;
	}

	switch(channels){
		case 1: internalFormat = isHDR ? GL_R16F : GL_R8; break;
		case 2: internalFormat = isHDR ? GL_RG16F : GL_RG8; break;
		case 3: internalFormat = isHDR ? GL_RGB16F : GL_RGB8; break;
		case 4: internalFormat = isHDR ? GL_RGBA16F : GL_RGBA8; break;
		default: 
			Logger::Error("ResourceManager::LoadTexture", "Unsupported number of image channels.");
			stbi_image_free(data);
			return nullptr;
	}

	auto texture = std::make_unique<Texture>(GL_TEXTURE_2D, internalFormat, 1, width, height);
	texture->loadPixelsSingle(width, height, data);

	if(isHDR) texture->setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	else texture->setWrap(GL_REPEAT, GL_REPEAT);
	texture->setFilter(GL_LINEAR, GL_LINEAR);

	stbi_image_free(data);

	Texture* ptr = texture.get();
	_textures.emplace(textureName, std::move(texture));
	if(_measureLoadingTime) timer.Stop();

	Logger::Success("ResourceManager::LoadTexture", "Loaded " + std::string(isHDR ? "HDR" : "") + "texture '" + textureName + "'" + (_measureLoadingTime ? " in " + std::to_string(timer.GetElapsed()) + "ms" : "") + ".");
	return ptr;
}

Texture* ResourceManager::LoadTexture(const std::string& textureName, const std::vector<std::string>& texturePaths, bool flipVertically){
	Utils::Timer timer;
	auto it = _textures.find(textureName);
	if(it != _textures.end()) return it->second.get();

	if(texturePaths.size() != 6){
    	Logger::Error("ResourceManager::LoadTexture", "Cubemap requires exactly 6 texture faces.");
    	return nullptr;
	}

	bool isHDR = false;
	std::string prevFormat = Utils::toLower(std::filesystem::path(texturePaths[0]).extension());
	for(const std::string& path : texturePaths){
		std::string fileFormat = Utils::toLower(std::filesystem::path(path).extension());
		if(fileFormat.empty()){
			Logger::Error("ResourceManager::LoadTexture", "Texture name doesn't contain file format '" + path + "'.");
			return nullptr;
		}
		if(prevFormat != fileFormat){
			Logger::Error("ResourceManager::LoadTexture", "Not all textures have same file format.");
			return nullptr;
		}
		isHDR = (fileFormat == ".hdr");
	}

	int width, height, channels;
	std::vector<void*> rawFaces(texturePaths.size(), nullptr);

	stbi_set_flip_vertically_on_load(flipVertically);

	if(_measureLoadingTime) timer.Start();
	GLuint internalFormat;
	for(std::size_t i = 0; i < texturePaths.size(); i++){
		int w, h, c;

		rawFaces[i] = isHDR ? static_cast<void*>(stbi_loadf(texturePaths[i].c_str(), &w, &h, &c, 0)) :
						 static_cast<void*>(stbi_load(texturePaths[i].c_str(), &w, &h, &c, 0));

		if(!rawFaces[i]){
			const char* failureReason = stbi_failure_reason();
    		std::string errorDetails = failureReason ? failureReason : "Unknown error";

			Logger::Error("ResourceManager::LoadTexture", "Failed to load " + std::string(isHDR ? "HDR" : "") + " texture image from path '" + texturePaths[i] + "' | Reason: " + errorDetails);
			for(auto ptr : rawFaces) if(ptr) stbi_image_free(ptr);
			return nullptr;
		}
		
		if(i == 0){
			width = w;
			height = h;
			channels = c;
		}

		if(w != width || h != height || c != channels){
			Logger::Error("ResouceManager::LoadTexture", "Cubemap faces must have identical dimensions and channels counts.");
			
			for(auto ptr : rawFaces) if(ptr) stbi_image_free(ptr);
			return nullptr;
		}
	}
	
	switch(channels){
		case 1: internalFormat = isHDR ? GL_R16F : GL_R8; break;
		case 2: internalFormat = isHDR ? GL_RG16F : GL_RG8; break;
		case 3: internalFormat = isHDR ? GL_RGB16F : GL_RGB8; break;
		case 4: internalFormat = isHDR ? GL_RGBA16F : GL_RGBA8; break;
		default: 
			Logger::Error("ResourceManager::LoadTexture", "Unsupported number of image channels.");
			for(auto ptr : rawFaces) if(ptr) stbi_image_free(ptr);
			return nullptr;
	}

	auto texture = std::make_unique<Texture>(GL_TEXTURE_CUBE_MAP, internalFormat, 1, width, height);
	std::vector<const void*> faceDataSources(rawFaces.begin(), rawFaces.end());
	texture->loadPixelsMultiple(faceDataSources, width, height);
	texture->setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	texture->setFilter(GL_LINEAR, GL_LINEAR);

	for(auto ptr: rawFaces) if(ptr) stbi_image_free(ptr);
	
	Texture* ptr = texture.get();
	_textures.emplace(textureName, std::move(texture));
	if(_measureLoadingTime) timer.Stop();

	Logger::Success("ResourceManager::LoadTexture", "Loaded " + std::string(isHDR ? "HDR" : "") + "texture '" + textureName + "'" + (_measureLoadingTime ? " in " + std::to_string(timer.GetElapsed()) + "ms" : "") + ".");
	return ptr;
}

Texture* ResourceManager::GetTexture(const std::string& textureName){
	auto it = _textures.find(textureName);
	if(it == _textures.end()){
		Logger::Warning("ResourceManager::GetTexture", "Texture '" + textureName + "' not found.");
		return nullptr;
	}
	return it->second.get();
}

void ResourceManager::RemoveTexture(const std::string& textureName){
	auto it = _textures.find(textureName);
	if(it == _textures.end()){
		Logger::Warning("ResourceManager::RemoveTexture", "Texture '" + textureName + "' not found.");
		return;
	}
	_textures.erase(it);
}

Camera* ResourceManager::GetCamera(const std::string& cameraName){
	auto it = _cameras.find(cameraName);
	if(it == _cameras.end()){
		Logger::Warning("ResourceManager::GetCamera", "Camera '" + cameraName + "' not found.");
		return nullptr;
	}
	return it->second.get();
}

void ResourceManager::RemoveCamera(const std::string& cameraName){
	auto it = _cameras.find(cameraName);
	if(it == _cameras.end()){
		Logger::Warning("ResourceManager::RemoveCamera", "Camera '" + cameraName + "' not found.");
		return;
	}
	_cameras.erase(it);
}

void ResourceManager::Clear(){
	_models.clear();
	_shaders.clear();
	_cameras.clear();
	_textures.clear();

	Logger::Info("ResourceManager::Clear", "All resources cleared.");
}

void ResourceManager::SetMeasureLoadingTime(bool measureTime){
	_measureLoadingTime = measureTime;
}

bool ResourceManager::IsMeasureLoadingTime(){
	return _measureLoadingTime;
}
