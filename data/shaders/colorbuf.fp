UNIFORMS

varying lowp float FS_diffuse;
varying lowp vec4 FS_color;
VARYINGS

void main(void)
{
    gl_FragColor = SHADOW * FS_color * FS_diffuse
        + FS_color * (_AMBIENT) * AMBIENT_OCCL;
}

