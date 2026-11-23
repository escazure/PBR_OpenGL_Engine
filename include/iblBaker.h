#pragma once

#include <memory>

class Shader;
class Texture;

struct IBLBakeConfig {
	unsigned int envResolution = 2048;
	unsigned int irradianceResolution = 32;
	unsigned int prefilterResolution = 256;
	unsigned int prefilterMipLevels = 5;
	unsigned int brdfLutResolution = 512;
};

struct IBLMapSet {
	std::unique_ptr<Texture> environmentCubemap = nullptr;
	std::unique_ptr<Texture> irradianceCubemap = nullptr;
	std::unique_ptr<Texture> prefilteredCubemap = nullptr;
	std::unique_ptr<Texture> brdfLUT = nullptr;
};

class IBLBaker {
private:
	inline static std::unique_ptr<Shader> _convertEquirectToCubemapShader = nullptr;
    inline static std::unique_ptr<Shader> _convoluteEnvironmentShader = nullptr;
    inline static std::unique_ptr<Shader> _prefilterEnvironmentShader = nullptr;
    inline static std::unique_ptr<Shader> _integrateBRDFShader = nullptr;

public:
	static void initialize();
	static bool isInitialized();
	static void shutdown();

	static std::unique_ptr<Texture> convertEquirectToCubemap(Texture* equirectTexture, unsigned int envResolution);
	static std::unique_ptr<Texture> convoluteEnvironmentCubemap(Texture* environmentCubemap, unsigned int irradianceResolution);
	static std::unique_ptr<Texture> prefilterEnvironmentCubemap(Texture* environmentCubemap, unsigned int prefilterResolution, unsigned int prefilterMipLevels);
	static std::unique_ptr<Texture> integrateBRDF(Texture* environmentCubemap, unsigned int brdfLutResolution);

	static IBLMapSet bakeFromEquirect(Texture* equirectCubemap, const IBLBakeConfig& config = {});
};
