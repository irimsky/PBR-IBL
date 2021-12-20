#version 450 core

const float PI = 3.141592;
const float Epsilon = 0.00001;

const int NumLights = 3;

// �ǽ�����F0����Ϊ0.4
const vec3 NonMetalF0 = vec3(0.04);

struct AnalyticalLight {
	vec3 direction;
	vec3 radiance;
};

layout(location=0) in Vertex
{
	vec3 position;
	vec2 texcoord;
	mat3 tangentBasis;
} vin;

layout(location=0) out vec4 color;


layout(std140, binding=1) uniform ShadingUniforms
{
	AnalyticalLight lights[NumLights];
	vec3 eyePosition;
};

layout(binding=0) uniform sampler2D albedoTexture;
layout(binding=1) uniform sampler2D normalTexture;
layout(binding=2) uniform sampler2D metalnessTexture;
layout(binding=3) uniform sampler2D roughnessTexture;
layout(binding=4) uniform samplerCube specularTexture;
layout(binding=5) uniform samplerCube irradianceTexture;
layout(binding=6) uniform sampler2D specularBRDF_LUT;
layout(binding=7) uniform sampler2D occlusionTexture;
layout(binding=8) uniform sampler2D emmisiveTexture;

uniform bool haveMetalness;
uniform bool haveRoughness;
uniform bool haveOcclusion;
uniform bool haveEmission;


float NDF_GGX(float cosLh, float roughness)
{
	float alpha   = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}


float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

float gaSchlickGGX(float NdotL, float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic�������н�������ֱ�ӹⲿ��
	return gaSchlickG1(NdotL, k) * gaSchlickG1(NdotV, k);
}

vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (vec3(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// IBL��Ϊ�˽�kd�ӻ�����ȡ���������Ľ���
vec3 fresnelRoughness(vec3 F0, float cosTheta, float roughness)
{
	return F0 + (max(vec3(1-roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
	vec3 albedo = texture(albedoTexture, vin.texcoord).rgb;
	float metalness = 0.0;
	float roughness = 0.5;
	if(haveMetalness)
		metalness = texture(metalnessTexture, vin.texcoord).r;
	if(haveRoughness)
		roughness = texture(roughnessTexture, vin.texcoord).r;

	vec3 V = normalize(eyePosition - vin.position);
	vec3 N = normalize(2.0 * texture(normalTexture, vin.texcoord).rgb - 1.0);
	N = normalize(vin.tangentBasis * N);
	
	float NdotV = max(0.0, dot(N, V));
		
	vec3 R = 2.0 * NdotV * N - V;

	// ���ڽ������壬��F0�ý����Ȳ�ֵģ��
	vec3 F0 = mix(NonMetalF0, albedo, metalness);

	// ֱ�ӹ���
	vec3 directLighting = vec3(0);
	for(int i=0; i<NumLights; ++i)
	{
		vec3 L = -lights[i].direction;
		vec3 Lradiance = lights[i].radiance;
		vec3 H = normalize(V + L);

		float NdotL = max(0.0, dot(N, L));
		float NdotH = max(0.0, dot(N, H));
		float HdotV = max(0.0, dot(H, V));

		vec3  F = fresnelSchlick(F0, HdotV);
		float D = NDF_GGX(NdotH, roughness);
		float G = gaSchlickGGX(NdotL, NdotV, roughness);

		vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metalness);

		// �����䲿��
		vec3 diffuseBRDF = kd * albedo;

		// �߹ⲿ��
		// ��ֹ����0
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * NdotL * NdotV);

		directLighting += (diffuseBRDF + specularBRDF) * Lradiance * NdotL;
	}

	// ��������
	vec3 ambientLighting;
	vec3 irradiance = texture(irradianceTexture, N).rgb;

	vec3 Froughness = fresnelRoughness(F0, NdotV, roughness);
	vec3 kd = mix(vec3(1.0) - Froughness, vec3(0.0), metalness);
	// 1/PI �Ѿ���Ԥ����Ĺ����м������
	vec3 diffuseIBL = kd * albedo * irradiance;

	// ����Mipmap�ȼ�
	int specularTextureMaxLevels = textureQueryLevels(specularTexture);

	vec3 specularIrradiance = textureLod(specularTexture, R, roughness * specularTextureMaxLevels).rgb;
	vec2 specularBRDF = texture(specularBRDF_LUT, vec2(NdotV, roughness)).rg;

	vec3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;

	ambientLighting = diffuseIBL + specularIBL;
	
	// �������ڱ�
	float AO = 1.0f;
	if(haveOcclusion)
		AO = texture(occlusionTexture, vin.texcoord).r;
	
	// �Է�����
	vec3 emmision = vec3(0);
	if(haveEmission)
		emmision = texture(emmisiveTexture, vin.texcoord).rgb;

	// ���ս��
	color = vec4(directLighting + AO * ambientLighting + emmision, 1.0);
}
