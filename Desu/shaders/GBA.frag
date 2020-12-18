#version 330

in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D ourTexture;

void main() {
    // https://byuu.net/video/color-emulation/
    
    vec4 texColor = texture(ourTexture,texCoord);

    texColor.rgb = pow(texColor.rgb / 31, vec3(4.0));
    fragColor.rgb = pow(vec3(  0 * texColor.b +  50 * texColor.g + 255 * texColor.r,
                              30 * texColor.b + 230 * texColor.g +  10 * texColor.r,
                             220 * texColor.b +  10 * texColor.g +  50 * texColor.r
                                ) / 255, vec3(0.45455));

    fragColor.a = 1.0;
}