@vs vs

uniform vertParams {
    mat4 mvp;
};

in vec4 Position;
in vec2 TexCoord;
in vec4 Color;

out vec2 texCoordV;
out vec4 colorV;

void main() {
    texCoordV	= TexCoord;
    colorV		= Color;
    gl_Position = mvp * Position;
}

@end

@fs fs

uniform texture2D tex;
uniform sampler smp;

in vec2 texCoordV;
in vec4 colorV;
out vec4 fragColor;

void main() {
    fragColor = colorV;
    fragColor.a *= texture(sampler2D(tex, smp), texCoordV).r;
}

@end

@program colorFont vs fs