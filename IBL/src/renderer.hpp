#pragma once
#include <glm/mat4x4.hpp>

#include "camera.hpp"

struct GLFWwindow;

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
	float objectYaw;
	float objectPitch;
};


class RendererInterface
{
public:
	virtual ~RendererInterface() = default;

	virtual GLFWwindow* initialize(int width, int height, int maxSamples) = 0;
	virtual void shutdown() = 0;
	virtual void setup(const SceneSettings& scene) = 0;
	virtual void render(GLFWwindow* window, const Camera& camera, const SceneSettings& scene) = 0;
};
