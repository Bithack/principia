UNIFORMS

varying lowp float FS_diffuse;
varying lowp vec4 FS_color;
VARYINGS

GI_FUN

void main(void)
{
    gl_FragColor = SHADOW * FS_color * FS_diffuse
        + FS_color * (_AMBIENT) * AMBIENT_OCCL GI;
}

