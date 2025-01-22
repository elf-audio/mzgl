
uniform mat4 mvp;

in vec4 Position;
in lowp vec2 TexCoord;
uniform lowp vec4 color;

out lowp vec4 colorV;
out lowp vec2 texCoordV;
void main(void) {
	colorV		= color;
	texCoordV	= TexCoord;
	gl_Position = mvp * Position;
}