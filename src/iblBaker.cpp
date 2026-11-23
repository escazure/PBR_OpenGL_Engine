#include "iblBaker.h"
#include "texture.h"
#include "shader.h"
#include "logger.h"

#include <string>

void IBLBaker::initialize(){
	if(isInitialized()) return;
	std::string shaderPath = std::string(ASSET_DIR) + "iblShaders/";
	_convertEquirectToCubemapShader = std::make_unique<Shader>(shaderPath + "equirectToCubemap.comp");
	_convoluteEnvironmentShader = std::make_unique<Shader>(shaderPath + "convCubemap.comp");
	_prefilterEnvironmentShader = std::make_unique<Shader>(shaderPath + "prefilterCubemap.comp");
	_integrateBRDFShader = std::make_unique<Shader>(shaderPath + "integrateBRDF.comp");
}

bool IBLBaker::isInitialized(){
	return _convertEquirectToCubemapShader &&
           _convoluteEnvironmentShader &&
           _prefilterEnvironmentShader &&
           _integrateBRDFShader;
}

void IBLBaker::shutdown(){
	_convertEquirectToCubemapShader.reset();
	_convoluteEnvironmentShader.reset();
	_prefilterEnvironmentShader.reset();
	_integrateBRDFShader.reset();
}

std::unique_ptr<Texture> IBLBaker::convertEquirectToCubemap(Texture* equirectTexture, unsigned int envResolution){
	if(equirectTexture == nullptr){
		Logger::Error("IBLBaker::convertEquirectToCubemap", "Equirect texture is nullptr.");
		return nullptr;
	}

	_convertEquirectToCubemapShader->use();
	equirectTexture->bind(0);
	
	std::unique_ptr<Texture> environmentCubemap = std::make_unique<Texture>(GL_TEXTURE_CUBE_MAP, GL_RGBA16F, 1, envResolution, envResolution);
	environmentCubemap->setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	environmentCubemap->setFilter(GL_LINEAR, GL_LINEAR);

	environmentCubemap->bindAsImage(0);

	unsigned int numGroupX = (envResolution + 15) / 16;
	unsigned int numGroupY = (envResolution + 15) / 16;
	glDispatchCompute(numGroupX, numGroupY, 6);

	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	return environmentCubemap;
}

std::unique_ptr<Texture> IBLBaker::convoluteEnvironmentCubemap(Texture* environmentCubemap, unsigned int irradianceResolution){
	if(environmentCubemap == nullptr){
		Logger::Error("IBLBaker::convoluteEnvironmentCubemap", "Environment cubemap is nullptr.");
		return nullptr;
	}

	_convoluteEnvironmentShader->use();
	environmentCubemap->bind(0);
	
	std::unique_ptr<Texture> convolutedEnvironmentCubemap = std::make_unique<Texture>(GL_TEXTURE_CUBE_MAP, GL_RGBA16F, 1, irradianceResolution, irradianceResolution);
	convolutedEnvironmentCubemap->setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	convolutedEnvironmentCubemap->setFilter(GL_LINEAR, GL_LINEAR);

	convolutedEnvironmentCubemap->bindAsImage(0);

	unsigned int numGroupX = (irradianceResolution + 15) / 16;
	unsigned int numGroupY = (irradianceResolution + 15) / 16;
	glDispatchCompute(numGroupX, numGroupY, 6);

	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	return convolutedEnvironmentCubemap;
}

std::unique_ptr<Texture> IBLBaker::prefilterEnvironmentCubemap(Texture* environmentCubemap, unsigned int prefilterResolution, unsigned int prefilterMipLevels){
	if(1 << (prefilterMipLevels - 1) > prefilterResolution){
		Logger::Warning("IBLBaker::prefilterEnvironmentCubemap", "Initial prefilter cubemap resolution does not fit all mip map levels, reduce mip levels or increase resolution.");
		return nullptr;
	}

	if(environmentCubemap == nullptr){
		Logger::Error("IBLBaker::prefilterEnvironmentCubemap", "Environment cubemap is nullptr.");
		return nullptr;
	}

	_prefilterEnvironmentShader->use();
	environmentCubemap->bind(0);

	std::unique_ptr<Texture> prefilteredEnvironmentCubemap = std::make_unique<Texture>(GL_TEXTURE_CUBE_MAP, GL_RGBA16F, prefilterMipLevels, prefilterResolution, prefilterResolution);
	prefilteredEnvironmentCubemap->setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	prefilteredEnvironmentCubemap->setFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

	for(unsigned int mip = 0; mip < prefilterMipLevels; mip++){
		unsigned int mipResolution = prefilterResolution >> mip;
		float roughness = prefilterMipLevels > 1 ? static_cast<float>(mip) / static_cast<float>(prefilterMipLevels - 1) : 0.0f;
		_prefilterEnvironmentShader->setFloat("uRoughness", roughness);
		
		prefilteredEnvironmentCubemap->bindAsImage(0, mip);
		
		unsigned int numGroupX = (mipResolution + 15) / 16;
		unsigned int numGroupY = (mipResolution + 15) / 16;
		glDispatchCompute(numGroupX, numGroupY, 6);
	}

	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	return prefilteredEnvironmentCubemap;
}

std::unique_ptr<Texture> IBLBaker::integrateBRDF(Texture* environmentCubemap, unsigned int brdfLutResolution){
	if(environmentCubemap == nullptr){
		Logger::Error("IBLBaker::integrateBRDF", "Environment cubemap is nullptr.");
		return nullptr;
	}

	_integrateBRDFShader->use();
	environmentCubemap->bind(0);
	
	std::unique_ptr<Texture> integratedBRDF = std::make_unique<Texture>(GL_TEXTURE_2D, GL_RG16F, 1, brdfLutResolution, brdfLutResolution);
	integratedBRDF->setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	integratedBRDF->setFilter(GL_LINEAR, GL_LINEAR);

	integratedBRDF->bindAsImage(0);

	unsigned int numGroupX = (brdfLutResolution + 15) / 16;
	unsigned int numGroupY = (brdfLutResolution + 15) / 16;
	glDispatchCompute(numGroupX, numGroupY, 1);

	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	return integratedBRDF;
}

IBLMapSet IBLBaker::bakeFromEquirect(Texture* equirectTexture, const IBLBakeConfig& config){
	if(equirectTexture == nullptr) return {};

	IBLMapSet mapSet;
	mapSet.environmentCubemap = convertEquirectToCubemap(equirectTexture, config.envResolution);

	if(!mapSet.environmentCubemap) return {};

	mapSet.irradianceCubemap = convoluteEnvironmentCubemap(mapSet.environmentCubemap.get(), config.irradianceResolution);
	mapSet.prefilteredCubemap = prefilterEnvironmentCubemap(mapSet.environmentCubemap.get(), config.prefilterResolution, config.prefilterMipLevels);
	mapSet.brdfLUT= integrateBRDF(mapSet.environmentCubemap.get(), config.brdfLutResolution);

	return mapSet;
}
