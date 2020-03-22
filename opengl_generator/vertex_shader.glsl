#version 330 core

in vec3 pos;
in vec4 col;

out vec4 fragCol;

uniform mat4 view;
uniform mat4 projection;

void main() {
	gl_Position = projection * view * vec4(pos, 1.0);
	fragCol = col;
}