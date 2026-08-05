uniform lowp sampler2D tex_0;

varying lowp vec2 tx0;
varying lowp vec2 tx1;
varying lowp vec2 tx2;
varying lowp vec2 tx3;
varying lowp vec2 tx4;

void main() {
	lowp vec4 color;
	color = .25 * texture2D(tex_0, tx0);
	color += .125 * texture2D(tex_0, tx1);
	color += .125 * texture2D(tex_0, tx2);
	color += .125 * texture2D(tex_0, tx3);
	color += .125 * texture2D(tex_0, tx4);
	gl_FragColor = color;
}
