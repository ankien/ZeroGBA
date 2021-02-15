#version 330

out vec2 texCoord;

// (-x,-y), (+x,-y), (-x,+y), (+x,+y); all quadrants
const vec2 glVertices[4] = vec2[4](vec2(-1.0,-1.0), vec2(1.0,-1.0), vec2(-1.0,1.0), vec2(1.0,1.0));
// (-x,+y), (+x,+y), (-x,-y), (+x,-y); gl_Position doesn't use the same coordinate system as textures
const vec2 texVertices[4] = vec2[4](vec2(0.0,1.0), vec2(1.0,1.0), vec2(0.0,0.0), vec2(1.0,0.0));

void main() {
    // x,y,z,w
    gl_Position = vec4(glVertices[gl_VertexID],0.0,1.0);
    // x,y
    texCoord = texVertices[gl_VertexID];
}