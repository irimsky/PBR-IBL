#include <cstdio>
#include <string>
#include <memory>
#include <vector>

#include "application.hpp"

#include "opengl.hpp"

void init(const char* objName, const char* envName, const char* objExt, const char* texExt);

int main(int argc, char* argv[])
{
	init("helmet", "street", ".obj", ".tga");
	RendererInterface* renderer = new Renderer();
	try {
		Application().run(std::unique_ptr<RendererInterface>{ renderer });
	}
	catch (const std::exception& e) {
		std::fprintf(stderr, "Error: %s\n", e.what());
		return 1;
	}
}

void init(const char* objName, const char* envName, const char* objExt, const char* texExt)
{
	Application::sceneSetting.envName = envName;
	Application::sceneSetting.objName = objName;
	std::string name = objName;	
	if(name.substr(name.find_last_of('_')+1) == "ball")
		Application::sceneSetting.objType = Mesh::Ball;	
	else
		Application::sceneSetting.objType = Mesh::ImportModel;

	Application::sceneSetting.objExt = objExt;	// 模型文件后缀名
	Application::sceneSetting.texExt = texExt;	// 纹理文件后缀名

	Application::sceneSetting.objectPitch = 0;
	Application::sceneSetting.objectYaw = -90;

	// 光照设置
	Application::sceneSetting.lights[0].direction = glm::normalize(glm::vec3{ -1.0f,  0.0f, 0.0f });
	Application::sceneSetting.lights[1].direction = glm::normalize(glm::vec3{ 1.0f,  0.0f, 0.0f });
	Application::sceneSetting.lights[2].direction = glm::normalize(glm::vec3{ 0.0f, -1.0f, 0.0f });

	Application::sceneSetting.lights[0].radiance = glm::vec3{ 1.0f };
	Application::sceneSetting.lights[1].radiance = glm::vec3{ 1.0f };
	Application::sceneSetting.lights[2].radiance = glm::vec3{ 1.0f };
}
