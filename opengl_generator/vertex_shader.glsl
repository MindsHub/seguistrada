// vertex shader: dati i vertici 3D ne determina la posizione sullo schermo 2D
#version 330 core

in vec3 pos;
in vec3 col;

out vec3 fragCol;

uniform mat4 view;
uniform mat4 projection;

void main() {
	gl_Position = projection * view * vec4(pos, 1.0);
	fragCol = col;
}