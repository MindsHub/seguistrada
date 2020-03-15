// fragment shader: per ogni pixel decide il suo colore
#version 330 core

in vec3 fragCol;

out vec4 outColor;

uniform sampler2D texture1;
uniform float time;

void main() {
   outColor = vec4(fragCol, 1.0);
}