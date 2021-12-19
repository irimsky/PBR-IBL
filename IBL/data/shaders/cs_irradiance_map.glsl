#version 450 core

const float PI = 3.141592;
const float Epsilon = 0.0001;
const uint NumSamples = 64 * 1024;
const float InvNumSamples = 1.0 / float(NumSamples);

layout(local_size_x=32, local_size_y=32, local_size_z=1) in;

layout(binding=0) uniform samplerCube inputTexture;
layout(binding=1, rgba16f) restrict writeonly uniform imageCube outputTexture;


// 为了在球面上均匀采样，使用Hammersley采样
// 参考：https://zhuanlan.zhihu.com/p/103715075
float radicalInverse(uint bits)
{
	//高低16位换位置
	bits = (bits << 16u) | (bits >> 16u);
	//A是5的按位取反
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	//C是3的按位取反
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Hammersley采样点集合的第i个点
vec2 sampleHammersley(uint i)
{
	return vec2(i * InvNumSamples, radicalInverse(i));
}

// 随机数转为采样的向量
vec3 sampleHemisphere(float u, float v)
{
    float phi = v * 2.0 * PI;
	float cosTheta = sqrt(1.0 - u);
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

// 根据所在的CS的work group ID计算投影所向的方向 (32,32,6)
vec3 getSamplingVector()
{
    vec2 st = gl_GlobalInvocationID.xy / vec2(imageSize(outputTexture));
    vec2 uv = 2.0 * vec2(st.x, 1.0-st.y) - vec2(1.0);

    vec3 ret;

    if(gl_GlobalInvocationID.z == 0)      ret = vec3(1.0,  uv.y, -uv.x);
    else if(gl_GlobalInvocationID.z == 1) ret = vec3(-1.0, uv.y,  uv.x);
    else if(gl_GlobalInvocationID.z == 2) ret = vec3(uv.x, 1.0, -uv.y);
    else if(gl_GlobalInvocationID.z == 3) ret = vec3(uv.x, -1.0, uv.y);
    else if(gl_GlobalInvocationID.z == 4) ret = vec3(uv.x, uv.y, 1.0);
    else if(gl_GlobalInvocationID.z == 5) ret = vec3(-uv.x, uv.y, -1.0);
    return normalize(ret);
}


void main(void)
{
	// TBN向量
	vec3 N = getSamplingVector();
	vec3 T, B;
	T = cross(N, vec3(0.0, 1.0, 0.0));
	T = mix(cross(N, vec3(1.0, 0.0, 0.0)), T, step(Epsilon, dot(T, T)));
	T = normalize(T);
	B = normalize(cross(N, T));

	// 半球蒙特卡洛积分
	vec3 irradiance = vec3(0);
	for(uint i=0; i<NumSamples; ++i) {
		vec2 u  = sampleHammersley(i);
		vec3 hemisphereVec = sampleHemisphere(u.x, u.y);
		// 切线空间到世界空间
		vec3 Li = B * hemisphereVec.x + T * hemisphereVec.y + N * hemisphereVec.z;
		float cosTheta = max(0.0, dot(Li, N));
		// 使用的是余弦为权重的采样，已经将PI约去（pdf = dot(Li*N) / PI）
		irradiance += textureLod(inputTexture, Li, 0).rgb * cosTheta;
	}

	irradiance *= vec3(InvNumSamples);
	imageStore(outputTexture, ivec3(gl_GlobalInvocationID), vec4(irradiance, 1.0));
}
