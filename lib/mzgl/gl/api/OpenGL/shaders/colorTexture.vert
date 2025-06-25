uniform mat4 mvp;

in vec4 Position;
in lowp vec2 TexCoord;
in lowp vec4 Color;

out lowp vec4 colorV;
out lowp vec2 texCoordV;
void main(void) {
	colorV		= Color;
	texCoordV	= TexCoord;
	gl_Position = mvp * Position;
}