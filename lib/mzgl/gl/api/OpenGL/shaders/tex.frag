in lowp vec4 colorV;
in lowp vec2 texCoordV;
out vec4 fragColor;
uniform sampler2D myTextureSampler;

void main(void) {
    fragColor = texture(myTextureSampler, texCoordV) * colorV;
}