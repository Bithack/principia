uniform vec4 color;
UNIFORMS

varying lowp float FS_diffuse;
VARYINGS

GI_FUN

void main(void)
{
    gl_FragColor = SHADOW * color * FS_diffuse
        + color * (_AMBIENT) * AMBIENT_OCCL GI;
}

