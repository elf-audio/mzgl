uniform mat4 mvp;

in vec4 Position;
uniform lowp vec4 color;
out lowp vec4 colorV;

void main(void) {
	colorV		= color;
	gl_Position = mvp * Position;
}