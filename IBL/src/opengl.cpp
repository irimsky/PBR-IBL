#include <stdexcept>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <GLFW/glfw3.h>

#include "mesh.hpp"
#include "image.hpp"
#include "utils.hpp"
#include "opengl.hpp"
#include "application.hpp"


struct TransformUB
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

struct ShadingUB
{
	struct {
		glm::vec4 direction;
		glm::vec4 radiance;
	} lights[SceneSettings::NumLights];
	glm::vec4 eyePosition;
};

Renderer::Renderer(){}

GLFWwindow* Renderer::initialize(int width, int height, int maxSamples)
{
	glfwInit();
		
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// 4.5版本OpenGL
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

#if _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

	glfwWindowHint(GLFW_DEPTH_BITS, 0);
	glfwWindowHint(GLFW_STENCIL_BITS, 0);
	glfwWindowHint(GLFW_SAMPLES, 0);

	GLFWwindow* window = glfwCreateWindow(width, height, "PBR", nullptr, nullptr);
	if (!window) {
		throw std::runtime_error("GLFW窗口创建失败");
	}

	glfwMakeContextCurrent(window);
	// 设置监视器的最低刷新数
	glfwSwapInterval(-1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		throw std::runtime_error("GLAD初始化失败");
	}
	// 最大各向异性层数
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &m_capabilities.maxAnisotropy);

#if _DEBUG
	glDebugMessageCallback(Renderer::logMessage, nullptr);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif
	// 最大的MSAA子采样点数
	GLint maxSupportedSamples;
	glGetIntegerv(GL_MAX_SAMPLES, &maxSupportedSamples);
	const int samples = glm::min(maxSamples, maxSupportedSamples);

	m_framebuffer = createFrameBuffer(width, height, samples, GL_RGBA16F, GL_DEPTH24_STENCIL8);
	if (samples > 0) {
		m_resolveFramebuffer = createFrameBuffer(width, height, 0, GL_RGBA16F, GL_NONE);
	}
	else {
		m_resolveFramebuffer = m_framebuffer;
	}

	std::printf("OpenGL 4.5 Renderer [%s]\n", glGetString(GL_RENDERER));
	return window;
}

void Renderer::shutdown()
{
	if (m_framebuffer.id != m_resolveFramebuffer.id) {
		deleteFrameBuffer(m_resolveFramebuffer);
	}
	deleteFrameBuffer(m_framebuffer);

	glDeleteVertexArrays(1, &m_emptyVAO);

	m_skyboxShader.deleteProgram();
	m_pbrShader.deleteProgram();
	m_tonemapShader.deleteProgram();

	glDeleteBuffers(1, &m_transformUB);
	glDeleteBuffers(1, &m_shadingUB);

	deleteMeshBuffer(m_skybox);
	deleteMeshBuffer(m_pbrModel);

	deleteTexture(m_envTexture);
	deleteTexture(m_irmapTexture);
	deleteTexture(m_BRDF_LUT);

	deleteTexture(m_albedoTexture);
	deleteTexture(m_normalTexture);
	deleteTexture(m_metalnessTexture);
	deleteTexture(m_roughnessTexture);
}

void Renderer::setup(const SceneSettings& scene)
{
	// 各种贴图大小
	static constexpr int kEnvMapSize = 1024;	// 必须是2的次幂
	static constexpr int kIrradianceMapSize = 32;
	static constexpr int kBRDF_LUT_Size = 512;

	// 设置OpenGL全局状态
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glFrontFace(GL_CCW);

	// 创建一个空的VAO
	glCreateVertexArrays(1, &m_emptyVAO);

	// 创建Uniform Buffer
	m_transformUB = createUniformBuffer<TransformUB>();
	m_shadingUB = createUniformBuffer<ShadingUB>();

	// 加载tonemap、天空盒、pbr着色器
	// TODO: recompile warning，检查一下
	m_tonemapShader = Shader("data/shaders/tonemap_vs.glsl", "data/shaders/tonemap_fs.glsl");
	
	m_skybox = createMeshBuffer(Mesh::fromFile("data/meshes/skybox.obj"));
	m_skyboxShader = Shader("data/shaders/skybox_vs.glsl", "data/shaders/skybox_fs.glsl");

	m_pbrModel = createMeshBuffer(Mesh::fromFile("data/meshes/helmet.obj"));
	m_pbrShader = Shader("data/shaders/pbr_vs.glsl", "data/shaders/pbr_fs.glsl");
	m_pbrShader.use();
	
	// 加载纹理贴图
	// TODO: 重构文件结构，支持自动扫描贴图
	m_albedoTexture = createTexture(Image::fromFile("data/textures/helmet_basecolor.tga", 3), GL_RGB, GL_SRGB8);
	m_normalTexture = createTexture(Image::fromFile("data/textures/helmet_normal.tga", 3), GL_RGB, GL_RGB8);
	m_pbrShader.setBool("haveMetalness", true);
	m_metalnessTexture = createTexture(Image::fromFile("data/textures/helmet_metalness.tga", 1), GL_RED, GL_R8);
	m_pbrShader.setBool("haveRoughness", true);
	m_roughnessTexture = createTexture(Image::fromFile("data/textures/helmet_roughness.tga", 1), GL_RED, GL_R8);
	m_pbrShader.setBool("haveOcclusion", true);
	m_occlusionTexture = createTexture(Image::fromFile("data/textures/helmet_occlusion.tga", 1), GL_RED, GL_R8);
	m_pbrShader.setBool("haveEmmisive", true);
	m_emmisionTexture = createTexture(Image::fromFile("data/textures/helmet_emission.tga", 3), GL_RGB, GL_SRGB8);

	// “尚未预滤波的环境贴图”变量（Cube Map类型)
	Texture envTextureUnfiltered = createTexture(GL_TEXTURE_CUBE_MAP, kEnvMapSize, kEnvMapSize, GL_RGBA16F);

	// 将环境贴图equirectangular投影成cubemap
	ComputeShader equirectToCubeShader = ComputeShader("data/shaders/cs_equirect2cube.glsl");

	// TODO: 支持直接读入cubemap
	// 加载环境贴图（equirectangular型）
	std::string envFilePath = "data/hdr/environment_";
	envFilePath += scene.envName + ".hdr";
	Texture envTextureEquirect = createTexture(Image::fromFile(envFilePath, 3), GL_RGB, GL_RGB16F, 1);
		
	// equirectangular 投影，将得到的结果写入“尚未预滤波的环境贴图”中
	equirectToCubeShader.use();
	glBindTextureUnit(0, envTextureEquirect.id);
	glBindImageTexture(1, envTextureUnfiltered.id, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	equirectToCubeShader.compute(
		envTextureUnfiltered.width / 32,
		envTextureUnfiltered.height / 32,
		6
	);
	equirectToCubeShader.deleteProgram();
	// 已经投影到了envTextureUnfiltered上，将Equirect贴图删除
	deleteTexture(envTextureEquirect);
		
	// 为“尚未预滤波的环境贴图”自动生成mipmap链
	glGenerateTextureMipmap(envTextureUnfiltered.id);

	// 环境贴图（cube map型）
	m_envTexture = createTexture(GL_TEXTURE_CUBE_MAP, kEnvMapSize, kEnvMapSize, GL_RGBA16F);
	// 复制“尚未预滤波的环境贴图”mipmap第0层（原图）到环境贴图的第0层上
	glCopyImageSubData(envTextureUnfiltered.id, GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
		m_envTexture.id, GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
		m_envTexture.width, m_envTexture.height, 6);

	// 滤波环境贴图变量
	ComputeShader prefilterShader = ComputeShader("data/shaders/cs_prefilter.glsl");
	prefilterShader.use();
	glBindTextureUnit(0, envTextureUnfiltered.id);
	// 根据粗糙度不同，对环境贴图进行预滤波（从第1级mipmap开始，第0级是原图）
	const float maxMipmapLevels = glm::max(float(m_envTexture.levels - 1), 1.0f);
	int size = kEnvMapSize / 2;
	for (int level = 1; level <= maxMipmapLevels; ++level) {
		const GLuint numGroups = glm::max(1, size / 32);
		// 将指定层级的纹理贴图绑定
		glBindImageTexture(1, m_envTexture.id, level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		prefilterShader.setFloat("roughness", (float)level / maxMipmapLevels);
		prefilterShader.compute(numGroups, numGroups, 6);
		size /= 2;
	}
	prefilterShader.deleteProgram();

	// 滤波过后的环境贴图已经存在m_envTexture里
	// 删除掉原有的“尚未预滤波的环境贴图”
	deleteTexture(envTextureUnfiltered);
	
	// 预计算漫反射用的 irradiance map.
	ComputeShader irradianceMapShader = ComputeShader("data/shaders/cs_irradiance_map.glsl");

	m_irmapTexture = createTexture(GL_TEXTURE_CUBE_MAP, kIrradianceMapSize, kIrradianceMapSize, GL_RGBA16F, 1);

	irradianceMapShader.use();
	glBindTextureUnit(0, m_envTexture.id);
	glBindImageTexture(1, m_irmapTexture.id, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	irradianceMapShader.compute(
		m_irmapTexture.width / 32, 
		m_irmapTexture.height / 32, 
		6
	);
	irradianceMapShader.deleteProgram();
	
	// 预计算高光部分需要的Look Up Texture (cosTheta, roughness)
	ComputeShader LUTShader = ComputeShader("data/shaders/cs_lut.glsl");

	m_BRDF_LUT = createTexture(GL_TEXTURE_2D, kBRDF_LUT_Size, kBRDF_LUT_Size, GL_RG16F, 1);
	glTextureParameteri(m_BRDF_LUT.id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_BRDF_LUT.id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	LUTShader.use();
	glBindImageTexture(0, m_BRDF_LUT.id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG16F);
	LUTShader.compute(
		m_BRDF_LUT.width / 32, 
		m_BRDF_LUT.height / 32,
		1
	);
	LUTShader.deleteProgram();
		
	glFinish();
}

void Renderer::render(GLFWwindow* window, const Camera& camera, const SceneSettings& scene)
{
	
	TransformUB transformUniforms;
	transformUniforms.model = 
		glm::eulerAngleXY(glm::radians(scene.objectPitch), glm::radians(scene.objectYaw))
		* glm::scale(glm::mat4(1.0f), glm::vec3(25, 25, 25));

	transformUniforms.view = camera.GetViewMatrix();
	transformUniforms.projection = glm::perspective(glm::radians(camera.Zoom), float(m_framebuffer.width)/float(m_framebuffer.height), 1.0f, 1000.0f);
	glNamedBufferSubData(m_transformUB, 0, sizeof(TransformUB), &transformUniforms);
	
	ShadingUB shadingUniforms;
	const glm::vec3 eyePosition = camera.Position;
	shadingUniforms.eyePosition = glm::vec4(eyePosition, 0.0f);
	for (int i = 0; i < SceneSettings::NumLights; ++i) {
		const SceneSettings::Light& light = scene.lights[i];
		shadingUniforms.lights[i].direction = glm::vec4{ light.direction, 0.0f };
		if (light.enabled) {
			shadingUniforms.lights[i].radiance = glm::vec4{ light.radiance, 0.0f };
		}
		else {
			shadingUniforms.lights[i].radiance = glm::vec4{};
		}
	}
	glNamedBufferSubData(m_shadingUB, 0, sizeof(ShadingUB), &shadingUniforms);
	
	// 渲染用的帧缓冲，接下来所有绘制的最后结果都会先保存在这个framebuffer上
	// 然后再将framebuffer绘制到屏幕上
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer.id);
	glClear(GL_DEPTH_BUFFER_BIT); 

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_transformUB);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_shadingUB);

	// 天空盒
	m_skyboxShader.use();
	glDisable(GL_DEPTH_TEST);
	glBindTextureUnit(0, m_envTexture.id);
	glBindVertexArray(m_skybox.vao);
	glDrawElements(GL_TRIANGLES, m_skybox.numElements, GL_UNSIGNED_INT, 0);

	// 模型
	m_pbrShader.use();
	glEnable(GL_DEPTH_TEST);
	glBindTextureUnit(0, m_albedoTexture.id);
	glBindTextureUnit(1, m_normalTexture.id);
	glBindTextureUnit(2, m_metalnessTexture.id);
	glBindTextureUnit(3, m_roughnessTexture.id);
	glBindTextureUnit(4, m_envTexture.id);
	glBindTextureUnit(5, m_irmapTexture.id);
	glBindTextureUnit(6, m_BRDF_LUT.id);
	glBindTextureUnit(7, m_occlusionTexture.id);
	glBindTextureUnit(8, m_emmisionTexture.id);
	glBindVertexArray(m_pbrModel.vao);
	glDrawElements(GL_TRIANGLES, m_pbrModel.numElements, GL_UNSIGNED_INT, 0);
	// Mesh::renderSphere();
	
	// 多重采样
	resolveFramebuffer(m_framebuffer, m_resolveFramebuffer);

	// 将整个framebuffer绘制在屏幕上（中间在着色器中进行一些后处理）
	// 解绑先前的framebuffer，绑定回默认屏幕的framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	m_tonemapShader.use();
	glBindTextureUnit(0, m_resolveFramebuffer.colorTarget);
	glBindVertexArray(m_emptyVAO);	// 空的VAO，仅做占位
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glfwSwapBuffers(window);
}

GLuint Renderer::compileShader(const std::string& filename, GLenum type)
{
	const std::string src = File::readText(filename);
	if (src.empty()) {
		throw std::runtime_error("Cannot read shader source file: " + filename);
	}
	const GLchar* srcBufferPtr = src.c_str();

	std::printf("Compiling GLSL shader: %s\n", filename.c_str());

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &srcBufferPtr, nullptr);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		GLsizei infoLogSize;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogSize);
		std::unique_ptr<GLchar[]> infoLog(new GLchar[infoLogSize]);
		glGetShaderInfoLog(shader, infoLogSize, nullptr, infoLog.get());
		throw std::runtime_error(std::string("Shader compilation failed: ") + filename + "\n" + infoLog.get());
	}
	return shader;
}

GLuint Renderer::linkProgram(std::initializer_list<GLuint> shaders)
{
	GLuint program = glCreateProgram();

	for (GLuint shader : shaders) {
		glAttachShader(program, shader);
	}
	glLinkProgram(program);
	for (GLuint shader : shaders) {
		glDetachShader(program, shader);
		glDeleteShader(shader);
	}

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_TRUE) {
		glValidateProgram(program);
		glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
	}
	if (status != GL_TRUE) {
		GLsizei infoLogSize;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogSize);
		std::unique_ptr<GLchar[]> infoLog(new GLchar[infoLogSize]);
		glGetProgramInfoLog(program, infoLogSize, nullptr, infoLog.get());
		throw std::runtime_error(std::string("Program link failed\n") + infoLog.get());
	}
	return program;
}

Texture Renderer::createTexture(GLenum target, int width, int height, GLenum internalformat, int levels) const
{
	Texture texture;
	texture.width = width;
	texture.height = height;
	texture.levels = (levels > 0) ? levels : Utility::numMipmapLevels(width, height);

	glCreateTextures(target, 1, &texture.id);
	glTextureStorage2D(texture.id, texture.levels, internalformat, width, height);
	glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, texture.levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameterf(texture.id, GL_TEXTURE_MAX_ANISOTROPY, m_capabilities.maxAnisotropy);
	return texture;
}

Texture Renderer::createTexture(const std::shared_ptr<class Image>& image, GLenum format, GLenum internalformat, int levels) const
{
	Texture texture = createTexture(GL_TEXTURE_2D, image->width(), image->height(), internalformat, levels);
	if (image->isHDR()) {
		glTextureSubImage2D(texture.id, 0, 0, 0, texture.width, texture.height, format, GL_FLOAT, image->pixels<float>());
	}
	else {
		glTextureSubImage2D(texture.id, 0, 0, 0, texture.width, texture.height, format, GL_UNSIGNED_BYTE, image->pixels<unsigned char>());
	}

	if (texture.levels > 1) {
		glGenerateTextureMipmap(texture.id);
	}
	return texture;
}

void Renderer::deleteTexture(Texture& texture)
{
	glDeleteTextures(1, &texture.id);
	std::memset(&texture, 0, sizeof(Texture));
}

FrameBuffer Renderer::createFrameBuffer(int width, int height, int samples, GLenum colorFormat, GLenum depthstencilFormat)
{
	FrameBuffer fb;
	fb.width = width;
	fb.height = height;
	fb.samples = samples;

	glCreateFramebuffers(1, &fb.id);

	// 为Framebuffer创建Renderbuffer
	// 可用于分配和存储颜色，深度或模板值，并可用作帧缓冲区对象中的颜色，深度或模板附件
	if (colorFormat != GL_NONE) {
		if (samples > 0) {
			glCreateRenderbuffers(1, &fb.colorTarget);
			glNamedRenderbufferStorageMultisample(fb.colorTarget, samples, colorFormat, width, height);
			glNamedFramebufferRenderbuffer(fb.id, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, fb.colorTarget);
		}
		else {
			glCreateTextures(GL_TEXTURE_2D, 1, &fb.colorTarget);
			glTextureStorage2D(fb.colorTarget, 1, colorFormat, width, height);
			glNamedFramebufferTexture(fb.id, GL_COLOR_ATTACHMENT0, fb.colorTarget, 0);
		}
	}
	// 为Renderbuffer指定深度/模板格式
	if (depthstencilFormat != GL_NONE) {
		glCreateRenderbuffers(1, &fb.depthStencilTarget);
		if (samples > 0) {
			glNamedRenderbufferStorageMultisample(fb.depthStencilTarget, samples, depthstencilFormat, width, height);
		}
		else {
			glNamedRenderbufferStorage(fb.depthStencilTarget, depthstencilFormat, width, height);
		}
		glNamedFramebufferRenderbuffer(fb.id, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb.depthStencilTarget);
	}

	// 检查Framebuffer完成状态
	GLenum status = glCheckNamedFramebufferStatus(fb.id, GL_DRAW_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		throw std::runtime_error("Framebuffer创建失败: " + std::to_string(status));
	}
	return fb;
}

void Renderer::resolveFramebuffer(const FrameBuffer& srcfb, const FrameBuffer& dstfb)
{
	if (srcfb.id == dstfb.id) {
		return;
	}

	std::vector<GLenum> attachments;
	if (srcfb.colorTarget) {
		attachments.push_back(GL_COLOR_ATTACHMENT0);
	}
	if (srcfb.depthStencilTarget) {
		attachments.push_back(GL_DEPTH_STENCIL_ATTACHMENT);
	}
	assert(attachments.size() > 0);

	glBlitNamedFramebuffer(srcfb.id, dstfb.id, 0, 0, srcfb.width, srcfb.height, 0, 0, dstfb.width, dstfb.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glInvalidateNamedFramebufferData(srcfb.id, (GLsizei)attachments.size(), &attachments[0]);
}

void Renderer::deleteFrameBuffer(FrameBuffer& fb)
{
	if (fb.id) {
		glDeleteFramebuffers(1, &fb.id);
	}
	if (fb.colorTarget) {
		if (fb.samples == 0) {
			glDeleteTextures(1, &fb.colorTarget);
		}
		else {
			glDeleteRenderbuffers(1, &fb.colorTarget);
		}
	}
	if (fb.depthStencilTarget) {
		glDeleteRenderbuffers(1, &fb.depthStencilTarget);
	}
	std::memset(&fb, 0, sizeof(FrameBuffer));
}

MeshBuffer Renderer::createMeshBuffer(const std::shared_ptr<class Mesh>& mesh)
{
	MeshBuffer buffer;
	buffer.numElements = static_cast<GLuint>(mesh->faces().size()) * 3;

	const size_t vertexDataSize = mesh->vertices().size() * sizeof(Mesh::Vertex);
	const size_t indexDataSize = mesh->faces().size() * sizeof(Mesh::Face);

	glCreateBuffers(1, &buffer.vbo);
	glNamedBufferStorage(buffer.vbo, vertexDataSize, reinterpret_cast<const void*>(&mesh->vertices()[0]), 0);
	glCreateBuffers(1, &buffer.ibo);
	glNamedBufferStorage(buffer.ibo, indexDataSize, reinterpret_cast<const void*>(&mesh->faces()[0]), 0);

	glCreateVertexArrays(1, &buffer.vao);
	glVertexArrayElementBuffer(buffer.vao, buffer.ibo);
	for (int i = 0; i < Mesh::NumAttributes; ++i) {
		glVertexArrayVertexBuffer(buffer.vao, i, buffer.vbo, i * sizeof(glm::vec3), sizeof(Mesh::Vertex));
		glEnableVertexArrayAttrib(buffer.vao, i);
		glVertexArrayAttribFormat(buffer.vao, i, i == 2 ? 2 : 3, GL_FLOAT, GL_FALSE, 0);
		glVertexArrayAttribBinding(buffer.vao, i, i);
	}
	return buffer;
}

void Renderer::deleteMeshBuffer(MeshBuffer& buffer)
{
	if (buffer.vao) {
		glDeleteVertexArrays(1, &buffer.vao);
	}
	if (buffer.vbo) {
		glDeleteBuffers(1, &buffer.vbo);
	}
	if (buffer.ibo) {
		glDeleteBuffers(1, &buffer.ibo);
	}
	std::memset(&buffer, 0, sizeof(MeshBuffer));
}

GLuint Renderer::createUniformBuffer(const void* data, size_t size)
{
	GLuint ubo;
	glCreateBuffers(1, &ubo);
	glNamedBufferStorage(ubo, size, data, GL_DYNAMIC_STORAGE_BIT);
	return ubo;
}

#if _DEBUG
void Renderer::logMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
		std::fprintf(stderr, "GL: %s\n", message);
	}
}
#endif


