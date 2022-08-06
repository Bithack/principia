uniform sampler2D tex_0;

UNIFORMS

varying lowp vec2 FS_texcoord;
varying lowp vec4 FS_color;
VARYINGS

void main(void)
{
    vec4 s = texture2D(tex_0, FS_texcoord);
    vec4 col = s * FS_color * s.a;
    gl_FragColor = col;
}

