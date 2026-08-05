uniform lowp sampler2D tex_0;

void main() {
    lowp vec4 color;
    vec2 tx = gl_FragCoord.xy;
    color = .25 * texture2D(tex_0, (tx+vec2(0.0, -1.0)) * vec2(0.00390625, 0.00390625));
    color += .5 * texture2D(tex_0, tx * vec2(0.00390625, 0.00390625));
    color += .25 * texture2D(tex_0, (tx+vec2(0.0, 1.0)) * vec2(0.00390625, 0.00390625));
    gl_FragColor = color;
}
