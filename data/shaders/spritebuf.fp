uniform sampler2D tex_0;

UNIFORMS

varying lowp vec2 FS_texcoord;
varying lowp vec4 FS_color;
VARYINGS

void main(void)
{
    float ambient = _AMBIENT;
    float shadow = SHADOW;

    vec4 col = texture2D(tex_0, FS_texcoord) * FS_color;

    gl_FragColor = (shadow*.5+ambient*2.) * col;
    //gl_FragColor = col;
}

