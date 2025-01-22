
uniform lowp vec4 color;
in vec2 texCoordV;
out vec4 fragColor;
uniform sampler2D myTextureSampler;

void main() {
    fragColor = color;
    fragColor.a *= texture(myTextureSampler, texCoordV).a;
}