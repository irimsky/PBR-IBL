#version 450 core

const float PI = 3.141592;
const float Epsilon = 0.001;

const uint NumSamples = 1024;
const float InvNumSamples = 1.0 / float(NumSamples);

layout(local_size_x=32, local_size_y=32, local_size_z=1) in;
layout(binding=0, rg16f) restrict writeonly uniform image2D LUT;


float radicalInverse(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 sampleHammersley(uint i)
{
	return vec2(i * InvNumSamples, radicalInverse(i));
}

// SchlickGGX重要性采样的NDF，考虑粗糙度控制范围
vec3 sampleSchlickGGX(float u, float v, float roughness)
{
	float alpha = roughness * roughness;

	float cosTheta = sqrt((1.0 - v) / (1.0 + (alpha*alpha - 1.0) * v));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	float phi = 2.0 * PI * u;

	return vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

// 下面函数用到的项
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX近似的几何衰减项 G，用了 Smith shadowing-masking 近似
float gaSchlickGGX(float NdotL, float NdotV, float roughness)
{
	float r = roughness;
	float k = (r * r) / 2.0; // Epic论文中提出了这一改动
	return gaSchlickG1(NdotL, k) * gaSchlickG1(NdotV, k);
}


void main(void)
{
	// 贴图的两个参数
	float NdotV = gl_GlobalInvocationID.x / float(imageSize(LUT).x);
	float roughness = gl_GlobalInvocationID.y / float(imageSize(LUT).y);

	// 防止除以0的现象
	NdotV = max(NdotV, Epsilon);

	// 因为GGX具有各向同性，所以随便取一个V；法向量是(0, 0, 1)
	vec3 V = vec3(sqrt(1.0 - NdotV*NdotV), 0.0, NdotV);

	// precomputed item = F0 * scale + bias
	float scale = 0;
	float bias = 0;

	for(uint i=0; i<NumSamples; ++i) {
		vec2 u  = sampleHammersley(i);
		// 随机采样半程向量 H
		vec3 H = sampleSchlickGGX(u.x, u.y, roughness);

		// 入射方向 L
		vec3 L = 2.0 * dot(V, H) * H - V;

		float NdotL = max(0.0, L.z);
		float NdotH = max(0.0, H.z);
		float VdotH = max(0.0, dot(V, H));

		if(NdotL > 0.0) {
			float G  = gaSchlickGGX(NdotV, NdotL, roughness);
			float Gv = G * VdotH / (NdotH * NdotV);
			float Fc = pow(1.0 - VdotH, 5);

			scale += (1.0 - Fc) * Gv;
			bias += Fc * Gv;
		}
	}

	imageStore(LUT, ivec2(gl_GlobalInvocationID), vec4(scale, bias, 0, 0) * InvNumSamples);
}
