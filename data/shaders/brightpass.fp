uniform mediump sampler2D tex_0;
varying lowp vec2 FS_texcoord;

void main() {
	vec3 color = texture2D(tex_0, FS_texcoord).rgb;
	float lum = dot(color, vec3(0.33, 0.33, 0.33));
	lum = lum*lum;
	gl_FragColor = vec4((lum - .5)*2.);
}
