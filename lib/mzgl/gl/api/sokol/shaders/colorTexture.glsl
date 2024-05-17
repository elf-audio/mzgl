@vs vs

precision highp float;
precision highp int;

uniform vertParams {
    mat4 mvp;
};

in vec4 Position;
in lowp vec2 TexCoord;
in lowp vec4 Color;

out lowp vec4 colorV;
out lowp vec2 texCoordV;
void main() {
    colorV		= Color;
    texCoordV	= TexCoord;
    gl_Position = mvp * Position;
}

@end

@fs fs

precision highp float;
precision highp int;

in lowp vec4 colorV;
in lowp vec2 texCoordV;
out vec4 fragColor;

uniform fragParams {
    lowp vec4 color;
};
uniform texture2D tex;
uniform sampler smp;

void main() {
    fragColor = texture(sampler2D(tex,smp), texCoordV) * colorV;
}

@end

@program colorTexture vs fs