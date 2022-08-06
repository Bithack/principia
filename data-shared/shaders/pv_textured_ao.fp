uniform sampler2D tex_0;
UNIFORMS

varying lowp float FS_diffuse;
varying lowp vec2 FS_texcoord;
VARYINGS

void main(void)
{
    vec4 color = texture2D(tex_0, FS_texcoord);
    gl_FragColor = SHADOW * color * FS_diffuse + color.a * color * (_AMBIENT)*AMBIENT_OCCL;
}

