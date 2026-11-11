#version 430 core

vec2 vertices[6] = vec2[6](
    vec2(-1, 1),
    vec2(-1, -1),
    vec2(1, 1),
    vec2(-1, -1),
    vec2(1, -1),
    vec2(1, 1));
vec2 uvs[6] = vec2[6](
    vec2(0, 0),
    vec2(0, 1),
    vec2(1, 0),
    vec2(0, 1),
    vec2(1, 1),
    vec2(1, 0));

out vec2 f_uv;

void main() {
  gl_Position = vec4(vertices[gl_VertexID], 0, 1);
  f_uv = uvs[gl_VertexID];
}