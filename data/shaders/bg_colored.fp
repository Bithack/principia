uniform lowp sampler2D tex_0;
uniform vec4 color;
UNIFORMS

varying lowp float FS_diffuse; /* LOWP IMPORTANT */
varying lowp vec2 FS_texcoord; /* LOWP IMPORTANT */
VARYINGS

GI_FUN

#ifdef ENABLE_AO
#define AO (1.0 - texture2D (tex_4, FS_ao).r*AMBIENT_OCCL_FACTOR)
#else
#define AO 1.0
#endif

void main(void)
{
    gl_FragColor = (((color * FS_diffuse) * SHADOW)
            + (color * (_AMBIENT * AO))) GI;
}

