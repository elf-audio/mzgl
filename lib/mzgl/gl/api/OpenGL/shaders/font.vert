uniform mat4 mvp;

in vec4 Position;
in vec2 TexCoord;

uniform lowp vec4 color;

out vec2 texCoordV;

void main() {
	texCoordV	= TexCoord;
	gl_Position = mvp * Position;
}