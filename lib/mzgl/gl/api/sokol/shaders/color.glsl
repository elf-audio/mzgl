@vs vs
in vec4 Position;
in vec4 Color;

uniform vertParams {
	mat4 mvp;
};
out vec4 ioColor;
void main() {
    gl_Position = mvp * Position;
    ioColor = Color;
}
@end

@fs fs
in vec4 ioColor;
out vec4 frag_color;
uniform fragParams {
    vec4 color;
};
void main() {
    frag_color = color * ioColor;
}
@end

@program color vs fs