uniform lowp sampler2D tex_0;
UNIFORMS

varying lowp vec2 FS_texcoord;
varying lowp vec2 FS_diffuse;
VARYINGS

void main(void)
{
    lowp vec4 albedo = texture2D(tex_0, FS_texcoord);
    gl_FragColor = vec4(SHADOW*albedo.xyz*.6*FS_diffuse.y + albedo.xyz*FS_diffuse.x, albedo.w);
}

