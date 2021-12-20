#pragma once
#include <GLFW/glfw3.h>
#include <stdexcept>

#include "application.hpp"

const int ScreenWidth = 1200;
const int ScreenHeight = 800;
const int DisplaySamples = 16;
const float MoveSpeed = 50;
const float OrbitSpeed = 0.8;

float Application::lastX = ScreenWidth / 2.0f;
float Application::lastY = ScreenHeight / 2.0f;
bool Application::firstMouse = true;
float Application::deltaTime = 0.0f;
float Application::lastFrame = 0.0f;

SceneSettings Application::sceneSetting;
Camera Application::m_camera(glm::vec3(0.0f, 0.0f, 150.0f));

Application::Application()
	: m_window(nullptr)
{}

Application::~Application()
{
	if (m_window) {
		glfwDestroyWindow(m_window);
	}
	glfwTerminate();
}

void Application::run(const std::unique_ptr<RendererInterface>& renderer)
{
	glfwWindowHint(GLFW_RESIZABLE, 0);
	m_window = renderer->initialize(ScreenWidth, ScreenHeight, DisplaySamples);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetCursorPosCallback(m_window, Application::mousePositionCallback);
	glfwSetScrollCallback(m_window, Application::mouseScrollCallback);
	glfwSetKeyCallback(m_window, Application::keyCallback);
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	renderer->setup(sceneSetting);
	while (!glfwWindowShouldClose(m_window)) {

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput();

		renderer->render(m_window, m_camera, sceneSetting);
		glfwPollEvents();
	}

	renderer->shutdown();
}

void Application::mousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; 

	lastX = xpos;
	lastY = ypos;

	m_camera.ProcessMouseMovement(xoffset, yoffset);
}

void Application::mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	m_camera.ProcessMouseScroll(yoffset);
}

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Application* self = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

	if (action == GLFW_PRESS) {

		SceneSettings::Light* light = nullptr;

		switch (key) {
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, true);
			break;
		case GLFW_KEY_F1:
			light = &sceneSetting.lights[0];
			break;
		case GLFW_KEY_F2:
			light = &sceneSetting.lights[1];
			break;
		case GLFW_KEY_F3:
			light = &sceneSetting.lights[2];
			break;
		}

		if (light) {
			light->enabled = !light->enabled;
		}
	}
}

void Application::processInput()
{
	if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
		m_camera.ProcessKeyboard(FORWARD, deltaTime * MoveSpeed);
	if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
		m_camera.ProcessKeyboard(BACKWARD, deltaTime * MoveSpeed);
	if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
		m_camera.ProcessKeyboard(LEFT, deltaTime * MoveSpeed);
	if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
		m_camera.ProcessKeyboard(RIGHT, deltaTime * MoveSpeed);
	if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS)
		sceneSetting.objectPitch -= OrbitSpeed;
	if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
		sceneSetting.objectPitch += OrbitSpeed;
	if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS)
		sceneSetting.objectYaw -= OrbitSpeed;
	if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		sceneSetting.objectYaw += OrbitSpeed;
}
