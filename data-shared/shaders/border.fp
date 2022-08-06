uniform lowp sampler2D tex_0;
UNIFORMS

varying lowp vec3 FS_texcoord_diffuse;
VARYINGS

void main(void)
{
    lowp vec4 albedo = texture2D(tex_0, FS_texcoord_diffuse.xy);
    gl_FragColor = albedo * FS_texcoord_diffuse.z * SHADOW + albedo * (_AMBIENT);
}
