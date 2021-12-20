#pragma once

#include <glad/glad.h>
#include <string>


#include "renderer.hpp"
#include "shader.hpp"
#include "application.hpp"


struct MeshBuffer
{
	MeshBuffer() : vbo(0), ibo(0), vao(0) {}
	GLuint vbo, ibo, vao;
	GLuint numElements;
};

struct FrameBuffer
{
	FrameBuffer() : id(0), colorTarget(0), depthStencilTarget(0) {}
	GLuint id;
	GLuint colorTarget;
	GLuint depthStencilTarget;
	int width, height;
	int samples;
};

struct Texture
{
	Texture() : id(0) {}
	GLuint id;
	int width, height;
	int levels;
};

class Renderer final : public RendererInterface
{
public:
	Renderer();
	GLFWwindow* initialize(int width, int height, int maxSamples) override;
	void setup(const SceneSettings& scene) override;
	void render(GLFWwindow* window, const Camera& camera, const SceneSettings& scene);
	void shutdown() override;

private:
	Texture createTexture(GLenum target, int width, int height, GLenum internalformat, int levels = 0) const;
	Texture createTexture(const std::shared_ptr<class Image>& image, GLenum format, GLenum internalformat, int levels = 0) const;
	static void deleteTexture(Texture& texture);

	static FrameBuffer createFrameBuffer(int width, int height, int samples, GLenum colorFormat, GLenum depthstencilFormat);
	static void resolveFramebuffer(const FrameBuffer& srcfb, const FrameBuffer& dstfb);
	static void deleteFrameBuffer(FrameBuffer& fb);

	static MeshBuffer createMeshBuffer(const std::shared_ptr<class Mesh>& mesh);
	static void deleteMeshBuffer(MeshBuffer& buffer);

	static GLuint createUniformBuffer(const void* data, size_t size);
	template<typename T> GLuint createUniformBuffer(const T* data = nullptr)
	{
		return createUniformBuffer(data, sizeof(T));
	}

#if _DEBUG
	static void logMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
#endif

	struct {
		float maxAnisotropy = 1.0f;
	} m_capabilities;

	FrameBuffer m_framebuffer;
	FrameBuffer m_resolveFramebuffer;

	MeshBuffer m_skybox;
	MeshBuffer m_pbrModel;

	GLuint m_emptyVAO;

	Shader m_tonemapShader;
	Shader m_skyboxShader;
	Shader m_pbrShader;

	Texture m_envTexture;
	Texture m_irmapTexture;
	Texture m_BRDF_LUT;

	Texture m_albedoTexture;
	Texture m_normalTexture;
	Texture m_metalnessTexture;
	Texture m_roughnessTexture;
	Texture m_occlusionTexture;
	Texture m_emissionTexture;

	GLuint m_transformUB;
	GLuint m_shadingUB;
};



