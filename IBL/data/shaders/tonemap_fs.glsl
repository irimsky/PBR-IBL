#version 450 core

const float gamma     = 2.2;
const float exposure  = 1.0;
const float pureWhite = 1.0;

layout(binding=0) uniform sampler2D sceneColor;

in  vec2 screenPosition;
out vec4 outColor;

vec3 ACESToneMapping(vec3 color)
{
	const float A = 2.51f;
	const float B = 0.03f;
	const float C = 2.43f;
	const float D = 0.59f;
	const float E = 0.14f;

	return clamp((color * (A * color + B)) / (color * (C * color + D) + E), 0.0, 1.0);
}

void main()
{
	vec3 color = texture(sceneColor, screenPosition).rgb * exposure;

	// HDR到LDR的色调映射, tone mapping，参考：https://zhuanlan.zhihu.com/p/21983679
	vec3 mappedColor = ACESToneMapping(color);

	// 伽马矫正
	outColor = vec4(pow(mappedColor, vec3(1.0/gamma)), 1.0);
}
