uniform sampler2D tex_0;

UNIFORMS

varying lowp vec2 FS_texcoord;
varying lowp vec4 FS_color;
VARYINGS

void main(void)
{
    vec3 col = FS_color.rgb;

    gl_FragColor = vec4(col, texture2D(tex_0, FS_texcoord).r * FS_color.a);
}

