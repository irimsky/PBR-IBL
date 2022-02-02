#pragma once


#include<vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

inline std::vector<float> toVec3f(float x, float y, float z)
{
	std::vector<float> res(3);
	res[0] = x;
	res[1] = y;
	res[2] = z;
	return res;
}

inline std::vector<float> toVec3f(const glm::vec3& vec)
{
	std::vector<float> res(3);
	res[0] = vec.x;
	res[1] = vec.y;
	res[2] = vec.z;
	return res;
}

inline glm::vec3 toGlmVec3(const std::vector<float>& vec)
{
	assert(vec.size() == 3);
	glm::vec3 res(vec[0], vec[1], vec[2]);
	return res;
}


