#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#include "logger.h"

class Model;
class Shader;
class Texture;
class Camera;

class ResourceManager {
private:
	static std::unordered_map<std::string, std::unique_ptr<Model>> _models;
	static std::unordered_map<std::string, std::unique_ptr<Shader>> _shaders;
	static std::unordered_map<std::string, std::unique_ptr<Camera>> _cameras;
	static std::unordered_map<std::string, std::unique_ptr<Texture>> _textures;

	static bool _measureLoadingTime;

public:
	static Model* LoadModel(const std::string& modelName, const std::string& objPath, const std::string& mtlPath = "");
	static Model* GetModel(const std::string& modelName);
	static void UnloadModel(const std::string& modelName);

	static Shader* LoadShader(const std::string& shaderName, const std::string& computeShaderPath);
	static Shader* LoadShader(const std::string& shaderName, const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
	static Shader* GetShader(const std::string& shaderName);
	static void UnloadShader(const std::string& shaderName);

	template<typename... Args>
	static Texture* AddTexture(const std::string& textureName, Args&&... args){
		auto it = _textures.find(textureName);
		if(it != _textures.end()){
			Logger::Warning("ResourceManager::AddTexture", "Trying to add already existing texture '" + textureName + "'.");
			return nullptr;
		}

		auto texture = std::make_unique<Texture>(std::forward<Args>(args)...);
		Texture* ptr = texture.get();
		_textures.emplace(textureName, std::move(texture));
		return ptr;
	}
	static Texture* LoadTexture(const std::string& textureName, const std::string& texturePath, bool flipVertically = true);
	static Texture* LoadTexture(const std::string& textureName, const std::vector<std::string>& texturePaths, bool flipVertically = true);
	static Texture* GetTexture(const std::string& textureName);
	static void RemoveTexture(const std::string& textureName);

	template<typename... Args>
	static Camera* AddCamera(const std::string& cameraName, Args&&... args){
		auto it = _cameras.find(cameraName);
		if(it != _cameras.end()){
			Logger::Warning("ResourceManager::AddCamera", "Trying to add alread existing camera'" + cameraName + "'.");
			return nullptr;
		}

		auto camera = std::make_unique<Camera>(std::forward<Args>(args)...);
		Camera* ptr = camera.get();	
		_cameras.emplace(cameraName, std::move(camera));
		return ptr;
	}

	static Camera* GetCamera(const std::string& cameraName);
	static void RemoveCamera(const std::string& cameraName);

	static void Clear();

	static void SetMeasureLoadingTime(bool measureTime);
	static bool IsMeasureLoadingTime();
};

