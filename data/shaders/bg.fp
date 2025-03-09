uniform lowp sampler2D tex_0;
UNIFORMS

varying lowp float FS_diffuse; /* LOWP IMPORTANT */
varying lowp vec2 FS_texcoord; /* LOWP IMPORTANT */
VARYINGS

#ifdef ENABLE_AO
#define AO (1.0 - texture2D (tex_4, FS_ao).r*AMBIENT_OCCL_FACTOR)
#else
#define AO 1.0
#endif

void main(void)
{
    lowp vec4 tmpvar_1;
    tmpvar_1 = texture2D (tex_0, FS_texcoord);
    gl_FragColor = (((tmpvar_1 * FS_diffuse) * SHADOW)
            + (tmpvar_1 * (_AMBIENT * AO)));
}

