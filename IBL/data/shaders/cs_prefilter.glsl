#version 450 core

const float PI = 3.141592;
const float Epsilon = 0.00001;

const uint NumSamples = 1024;
const float InvNumSamples = 1.0 / float(NumSamples);

layout(local_size_x=32, local_size_y=32, local_size_z=1) in;

layout(binding=0) uniform samplerCube inputTexture;
// 一次只能绑定一层mipmap，所以只有一个纹理会输出
layout(binding=1, rgba16f) restrict writeonly uniform imageCube outputTexture;
// 粗糙度
uniform float roughness;

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

vec3 sampleSchlickGGX(float u, float v, float roughness)
{
	float alpha = roughness * roughness;

	float cosTheta = sqrt((1.0 - v) / (1.0 + (alpha*alpha - 1.0) * v));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	float phi = 2.0 * PI * u;

	return vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

// GGX/Towbridge-Reitz 法线分布函数
// Uses Disney's reparametrization of alpha = roughness^2.
float NDF_GGX(float NdotH, float roughness)
{
	float alpha   = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (NdotH * NdotH) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}


vec3 getSamplingVector()
{
    vec2 st = gl_GlobalInvocationID.xy/vec2(imageSize(outputTexture));
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
	// 防止BUG
	ivec2 outputSize = imageSize(outputTexture);
	if(gl_GlobalInvocationID.x >= outputSize.x || gl_GlobalInvocationID.y >= outputSize.y) {
		return;
	}
	
	vec2 inputSize = vec2(textureSize(inputTexture, 0));
	// cubemap上一个纹素对应的立体角（整个球面的立体角 / 6个cubemap面的纹素个数）
	float saTexel = 4.0 * PI / (6 * inputSize.x * inputSize.y);
	
	vec3 N = getSamplingVector();
	vec3 T, B;
	T = cross(N, vec3(0.0, 1.0, 0.0));
	T = mix(cross(N, vec3(1.0, 0.0, 0.0)), T, step(Epsilon, dot(T, T)));
	T = normalize(T);
	B = normalize(cross(N, T));
	

	// 假设 V = N
	vec3 V = N;

	vec3 color = vec3(0);
	float weight = 0;


	for(uint i=0; i < NumSamples; ++i) {
		vec2 u = sampleHammersley(i);
		vec3 h = sampleSchlickGGX(u.x, u.y, roughness);
		vec3 H = B * h.x + T * h.y + N * h.z;

		vec3 L = 2.0 * dot(V, H) * H - V;

		float NdotL = dot(N, L);
		if(NdotL > 0.0) {
			// 此处使用了 Krivanek 的 Pre-filtered importance sampling
			float NdotH = max(dot(N, H), 0.0);
			float HdotV = max(dot(V, H), 0.0);
			float D = NDF_GGX(NdotH, roughness);
			float pdf = D * NdotH / (4.0 * HdotV) + Epsilon;
			
			// 这次采样的立体角
			float saSample = 1.0 / (float(NumSamples) * pdf + Epsilon);
			// 对应的mipmap层级
			float mipLevel = roughness == 0.0? 0.0 : 0.5 * log2(saSample / saTexel);

			color  += textureLod(inputTexture, L, mipLevel).rgb * NdotL;
			weight += NdotL;
					
		}
	}
	color /= weight;

	imageStore(outputTexture, ivec3(gl_GlobalInvocationID), vec4(color, 1.0));
}
