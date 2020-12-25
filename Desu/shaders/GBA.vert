#version 330

out vec2 texCoord;

// (-x,-y), (+x,-y), (-x,+y), (+x,+y)
const vec2 vertices[4] = vec2[4](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0));

void main() {
    // x,y,z,w
    gl_Position = vec4(vertices[gl_VertexID],0.0,1.0);
    // x,y
    texCoord = (vertices[gl_VertexID] + 1.0) / vec2(2.0, -2.0);
}