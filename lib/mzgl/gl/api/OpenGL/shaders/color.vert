uniform mat4 mvp;
uniform lowp vec4 color;

in vec4 Position;
in lowp vec4 Color;

out lowp vec4 colorV;

void main(void) {
	colorV		= Color * color;
	gl_Position = mvp * Position;
}