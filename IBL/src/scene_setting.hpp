#pragma once

#include <glm/mat4x4.hpp>
#include <string>
#include "mesh.hpp"

class SceneSettings
{
public:
	static const int NumLights = 3;
	struct Light {
		glm::vec3 direction;
		glm::vec3 radiance;
		bool enabled = false;
	} lights[NumLights];

	std::string envName;
	std::string objName;
	std::string objExt;
	std::string texExt;

	Mesh::ObjectType objType;

	float objectYaw;
	float objectPitch;
};
