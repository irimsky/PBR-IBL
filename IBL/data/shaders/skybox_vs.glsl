#version 450 core


layout(std140, binding=0) uniform TransformUniforms
{
	mat4 model;
	mat4 view;
	mat4 projection;
};


layout(location=0) in vec3 position;
layout(location=0) out vec3 localPosition;

void main()
{
	localPosition = position.xyz;
	vec4 clip_pos   = projection * mat4(mat3(view)) * vec4(position, 1.0);
	gl_Position = clip_pos.xyww;
}
