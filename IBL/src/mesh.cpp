#include <cstdio>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>

#include <glad/glad.h>
#include <stdexcept>

#include "mesh.hpp"




const unsigned int ImportFlags =
	aiProcess_CalcTangentSpace |
	aiProcess_Triangulate |
	aiProcess_SortByPType |
	aiProcess_PreTransformVertices |
	aiProcess_GenNormals |
	aiProcess_GenUVCoords |
	aiProcess_OptimizeMeshes |
	aiProcess_Debone |
	aiProcess_ValidateDataStructure;


struct LogStream : public Assimp::LogStream
{
	static void initialize()
	{
		if (Assimp::DefaultLogger::isNullLogger()) {
			Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
			Assimp::DefaultLogger::get()->attachStream(new LogStream, Assimp::Logger::Err | Assimp::Logger::Warn);
		}
	}

	void write(const char* message) override
	{
		std::fprintf(stderr, "Assimp: %s", message);
	}
};

Mesh::Mesh(const aiMesh* mesh)
{
	assert(mesh->HasPositions());
	assert(mesh->HasNormals());

	m_vertices.reserve(mesh->mNumVertices);
	for (size_t i = 0; i < m_vertices.capacity(); ++i) {
		Vertex vertex;
		vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
		vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        
		if (mesh->HasTextureCoords(0)) {
			vertex.texcoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		}
        // 不是所有模型都有切线，assimp会自动计算（aiProcess_CalcTangentSpace）
        if (mesh->HasTangentsAndBitangents()) {
			vertex.tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
			vertex.bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
		}
		m_vertices.push_back(vertex);
	}

	m_faces.reserve(mesh->mNumFaces);
	for (size_t i = 0; i < m_faces.capacity(); ++i) {
		assert(mesh->mFaces[i].mNumIndices == 3);
		m_faces.push_back({ mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1], mesh->mFaces[i].mIndices[2] });
	}
}

unsigned int Mesh::sphereVAO = 0;
unsigned int Mesh::indexCount = 0;
void Mesh::renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals, tangents, bitangents;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                glm::vec3 normal = glm::vec3(xPos, yPos, zPos);
                glm::vec3 tangent = glm::vec3(-1, -1, (xPos + yPos)/2);
                glm::vec3 bitangent = glm::cross(normal, tangent);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(normal);
                tangents.push_back(tangent);
                bitangents.push_back(bitangent);
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = indices.size();

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
            if (tangents.size() > 0)
            {
                data.push_back(tangents[i].x);
                data.push_back(tangents[i].y);
                data.push_back(tangents[i].z);
            }
            if (bitangents.size() > 0)
            {
                data.push_back(bitangents[i].x);
                data.push_back(bitangents[i].y);
                data.push_back(bitangents[i].z);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3 + 3 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)(11 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

std::shared_ptr<Mesh> Mesh::fromFile(const std::string& filename)
{
	LogStream::initialize();

	std::printf("Loading mesh: %s\n", filename.c_str());

	std::shared_ptr<Mesh> mesh;
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(filename, ImportFlags);
	if (scene && scene->HasMeshes()) {
		mesh = std::shared_ptr<Mesh>(new Mesh{ scene->mMeshes[0] });
	}
	else {
		throw std::runtime_error("Failed to load mesh file: " + filename);
	}
	return mesh;
}

std::shared_ptr<Mesh> Mesh::fromString(const std::string& data)
{
	LogStream::initialize();

	std::shared_ptr<Mesh> mesh;
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFileFromMemory(data.c_str(), data.length(), ImportFlags, "nff");
	if (scene && scene->HasMeshes()) {
		mesh = std::shared_ptr<Mesh>(new Mesh{ scene->mMeshes[0] });
	}
	else {
		throw std::runtime_error("Failed to create mesh from string: " + data);
	}
	return mesh;
}
