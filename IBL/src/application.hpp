#pragma once
#include "renderer.hpp"


class Application
{
public:
	Application();
	~Application();
	void run(const std::unique_ptr<RendererInterface>& renderer);

	static SceneSettings sceneSetting;
	

private:
	static void mousePositionCallback(GLFWwindow* window, double xpos, double ypos);
	static void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	void processInput();

	GLFWwindow* m_window;
	static float lastX;
	static float lastY;
	static Camera m_camera;
	static bool firstMouse;
	static float deltaTime;
	static float lastFrame;
	
};
