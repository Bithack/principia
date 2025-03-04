uniform lowp sampler2D tex_0;
UNIFORMS

varying lowp float FS_diffuse;
varying lowp vec2 FS_texcoord;
VARYINGS

GI_FUN

void main(void)
{
    lowp vec4 albedo = texture2D(tex_0, FS_texcoord);
    gl_FragColor = SHADOW*albedo*FS_diffuse +
        albedo*(_AMBIENT)*AMBIENT_OCCL GI;
}

