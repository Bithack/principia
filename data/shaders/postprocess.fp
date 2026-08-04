uniform mediump sampler2D tex_0;
varying lowp vec2 FS_texcoord;

void main() {
    //vec3 col = sqrt(texture2D(tex_0, FS_texcoord).rgb);
    vec3 col = pow(texture2D(tex_0, FS_texcoord).rgb, vec3(1./2.2));
    gl_FragColor = vec4(col, 1.);
}
