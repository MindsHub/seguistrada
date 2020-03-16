// fragment shader: per ogni pixel decide il suo colore
#version 330 core

in vec4 fragCol;

out vec4 outColor;

void main() {
   outColor = fragCol;
}