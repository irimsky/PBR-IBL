#pragma once

#include <glad/glad.h>
#include <string>
#include <glm/mat4x4.hpp>

#include "shader.hpp"
#include "camera.hpp"
#include "scene_setting.hpp"

struct GLFWwindow;

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

class Renderer
{
public:
	Renderer();
	GLFWwindow* initialize(int width, int height, int maxSamples) ;
	void setup(const SceneSettings& scene) ;
	void render(GLFWwindow* window, const Camera& camera, const SceneSettings& scene);
	void renderImgui(SceneSettings& scene);
	void shutdown();

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

	void loadModels(const std::string& modelName, SceneSettings& scene);
	void loadSceneHdr(const std::string& filename);
	void calcLUT();
	

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
	ComputeShader m_equirectToCubeShader;
	ComputeShader m_prefilterShader;
	ComputeShader m_irradianceMapShader;

	int m_EnvMapSize;
	int m_IrradianceMapSize;
	int m_BRDF_LUT_Size;

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



