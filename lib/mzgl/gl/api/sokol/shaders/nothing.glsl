@vs vs
in vec4 Position;
uniform vertParams {
	mat4 mvp;
};
void main() {
    gl_Position = mvp * Position;
}
@end

@fs fs
out vec4 frag_color;
uniform fragParams {
    vec4 color;
};
void main() {
    frag_color = color;
}
@end

@program nothing vs fs