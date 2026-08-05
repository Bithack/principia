uniform sampler2D tex_0;
varying vec2 FS_texcoord;

void main() {
    vec4 color = texture2D(tex_0, FS_texcoord+vec2(0.,-2.*1./512.));
    color += 4.*texture2D(tex_0, FS_texcoord+vec2(0.,-1.*1./512.));
    color += 6.*texture2D(tex_0, FS_texcoord);
    color += 4.*texture2D(tex_0, FS_texcoord+vec2(0.,1.*1./512.));
    color += texture2D(tex_0, FS_texcoord+vec2(0.,2.*1./512.));
    gl_FragColor = color/16.;
}
