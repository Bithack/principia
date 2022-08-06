uniform lowp sampler2D tex_0;
uniform lowp vec4 ao_mask2;
UNIFORMS

varying lowp float FS_diffuse;
varying lowp vec2 FS_texcoord;
VARYINGS

void main(void)
{
    lowp vec4 albedo;
    albedo = texture2D(tex_0, FS_texcoord);
    gl_FragColor = SHADOW*FS_diffuse*albedo + (_AMBIENT)*albedo*AMBIENT_OCCL2;
}

