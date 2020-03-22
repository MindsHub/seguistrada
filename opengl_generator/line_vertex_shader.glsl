#version 330 core

in vec2 pos;
in vec4 col;

out vec4 fragCol;

void main() {
	gl_Position = vec4(pos, 0.0f, 1.0f);
	fragCol = col;
}