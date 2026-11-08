#pragma once

#include <glm/glm.hpp>

enum class LightType {
	Directional = 0,
	Point = 1
};

struct Light {
	glm::vec4 position;
	glm::vec4 color;
	float intensity;
	float range;
	int type;
	int padding;

	Light(glm::vec3 pos, glm::vec3 col, float intens, float rng, LightType t){
		position = glm::vec4(pos, 0.0f);
		color = glm::vec4(col, 0.0f);
		intensity = intens;
		range = rng;
		type = static_cast<int>(t);
		padding = 0;
	}
};
