#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

class Mesh
{
public:
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texcoord;
		glm::vec3 tangent;
		glm::vec3 bitangent;
	};
	static_assert(sizeof(Vertex) == 14 * sizeof(float), "Wrong Vertex Size");
	static const int NumAttributes = 5;

	struct Face
	{
		uint32_t v1, v2, v3;
	};
	static_assert(sizeof(Face) == 3 * sizeof(uint32_t), "Wrong Face Size");

	static std::shared_ptr<Mesh> fromFile(const std::string& filename);
	static std::shared_ptr<Mesh> fromString(const std::string& data);

	const std::vector<Vertex>& vertices() const { return m_vertices; }
	const std::vector<Face>& faces() const { return m_faces; }
	static void renderSphere();
	static unsigned int sphereVAO;
	static unsigned int indexCount;

private:
	Mesh(const struct aiMesh* mesh);

	std::vector<Vertex> m_vertices;
	std::vector<Face> m_faces;
};
