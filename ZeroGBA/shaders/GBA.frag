#version 330

in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D ourTexture;

void main() {
    fragColor = texture(ourTexture,texCoord);
}