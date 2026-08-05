uniform sampler2D tex_0;
varying vec2 FS_texcoord;

void main() {
    vec4 color = texture2D(tex_0, FS_texcoord+vec2(-2.*1./512.,0.));
    color += 4.*texture2D(tex_0, FS_texcoord+vec2(-1.*1./512.,0.));
    color += 6.*texture2D(tex_0, FS_texcoord);
    color += 4.*texture2D(tex_0, FS_texcoord+vec2(1.*1./512.,0.));
    color += texture2D(tex_0, FS_texcoord+vec2(2.*1./512.,0.));
    gl_FragColor = color/16.;
}
