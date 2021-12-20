#pragma once
#include <glm/mat4x4.hpp>
#include "camera.hpp"
#include "scene_setting.hpp"

struct GLFWwindow;


class RendererInterface
{
public:
	virtual ~RendererInterface() = default;

	virtual GLFWwindow* initialize(int width, int height, int maxSamples) = 0;
	virtual void shutdown() = 0;
	virtual void setup(const SceneSettings& scene) = 0;
	virtual void render(GLFWwindow* window, const Camera& camera, const SceneSettings& scene) = 0;
};
