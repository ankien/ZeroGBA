#version 330

out vec2 texCoord;

const vec2 vertices[4] = vec2[4](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0));

void main() {
    gl_Position = vec4(vertices[gl_VertexID],0.0,1.0);
    texCoord = (vertices[gl_VertexID] * 2.0) / vec2(2.0, -2.0);
}