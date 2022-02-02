#include <cstdio>
#include <string>
#include <memory>

#include "application.hpp"
#include "opengl.hpp"
#include "math.hpp"

void init();

int main(int argc, char* argv[])
{
	init();
	Renderer* renderer = new Renderer();
	try {
		Application().run(std::unique_ptr<Renderer>{ renderer });
	}
	catch (const std::exception& e) {
		std::fprintf(stderr, "Error: %s\n", e.what());
		return 1;
	}
}

void init()
{
	Application::sceneSetting.envNames = File::readAllFilesInDir(".\\data\\hdr");
	Application::sceneSetting.envName = new char[128];
	strcpy(Application::sceneSetting.envName, Application::sceneSetting.envNames[0]);
	Application::sceneSetting.preEnv = new char[128];
	strcpy(Application::sceneSetting.preEnv, Application::sceneSetting.envNames[0]);
	
	Application::sceneSetting.objNames = File::readAllDirsInDir(".\\data\\models");
	Application::sceneSetting.objName = new char[128];
	strcpy(Application::sceneSetting.objName, Application::sceneSetting.objNames[0]);
	Application::sceneSetting.preObj = new char[128];
	strcpy(Application::sceneSetting.preObj, Application::sceneSetting.objNames[0]);
	
	std::string objName = Application::sceneSetting.objNames[0];
	if(objName.substr(objName.find_last_of('_') + 1) == "ball")
		Application::sceneSetting.objType = Mesh::Ball;	
	else
		Application::sceneSetting.objType = Mesh::ImportModel;

	Application::sceneSetting.objectScale = 25.0;
	Application::sceneSetting.objectPitch = 0;
	Application::sceneSetting.objectYaw = -90;

	// π‚’’…Ë÷√
	Application::sceneSetting.lights[0].direction = toVec3f(glm::normalize(glm::vec3{ -1.0f,  0.0f, 0.0f }));
	Application::sceneSetting.lights[1].direction = toVec3f(glm::normalize(glm::vec3{ 1.0f,  0.0f, 0.0f }));
	Application::sceneSetting.lights[2].direction = toVec3f(glm::normalize(glm::vec3{ 0.0f, -1.0f, 0.0f }));

	Application::sceneSetting.lights[0].radiance = std::vector<float>(3, 1.0f);
	Application::sceneSetting.lights[1].radiance = std::vector<float>(3, 1.0f);
	Application::sceneSetting.lights[2].radiance = std::vector<float>(3, 1.0f);
}
