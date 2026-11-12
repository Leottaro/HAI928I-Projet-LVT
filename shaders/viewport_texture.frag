#version 430 core

// uniform sampler2D texture_in;

in vec2 f_uv;
out vec4 out_color;

void main() {
  out_color = vec4(f_uv, 0., 1.);
  // out_color = texture(texture_in, f_uv);
}