@vs vs

uniform vertParams {
    mat4 mvp;
};

in vec4 Position;
in lowp vec2 TexCoord;

out lowp vec2 texCoordV;

void main() {
    texCoordV	= TexCoord;
    gl_Position = mvp * Position;
}

@end

@fs fs

uniform fragParams {
    lowp vec4 color;
};
uniform texture2D tex;
uniform sampler smp;



in lowp vec2 texCoordV;
out vec4 fragColor;

void main() {
    fragColor = texture(sampler2D(tex,smp), texCoordV) * color;
}

@end

@program tex vs fs