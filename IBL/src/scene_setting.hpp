#pragma once

#include <glm/mat4x4.hpp>
#include <string>
#include "mesh.hpp"

class SceneSettings
{
public:
	static const int NumLights = 3;
	struct Light {
		std::vector<float> radiance;
		std::vector<float> direction;
		bool enabled = false;
	} lights[NumLights];

	char* envName;
	char* preEnv;
	std::vector<char*> envNames;
	
	char* objName;
	char* preObj;
	std::vector<char*> objNames;
	Mesh::ObjectType objType;

	std::string objExt;
	std::string texExt;

	float objectScale;
	float objectYaw;
	float objectPitch;
};
