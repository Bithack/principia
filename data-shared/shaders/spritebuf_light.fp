uniform sampler2D tex_0;

UNIFORMS

varying lowp vec2 FS_texcoord;
varying lowp vec4 FS_color;
VARYINGS

void main(void)
{
    gl_FragColor = texture2D(tex_0, FS_texcoord) * FS_color;
}

