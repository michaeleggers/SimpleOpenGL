#version 410
layout (location = 0) in vec3 vp;
layout (location = 1) in vec3 normal;
out vec4 n;
out vec4 pos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main () {
    gl_Position = projection*view*model*vec4(vp, 1.0);
    n = view*model*vec4(normal, 0.f);
    pos = view*model*vec4(vp, 1.f);
}

