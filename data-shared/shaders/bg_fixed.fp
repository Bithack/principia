uniform lowp sampler2D tex_0;
UNIFORMS

varying mediump vec2 FS_texcoord; /* LOWP IMPORTANT */
VARYINGS

void main(void)
{
    gl_FragColor = texture2D (tex_0, FS_texcoord);
}

