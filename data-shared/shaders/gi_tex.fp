varying float z;
#ifdef ENABLE_GI
uniform lowp sampler2D tex_0;
varying vec2 FS_texcoord;
varying float nor;
#endif

void main(void)
{
#ifdef ENABLE_GI
    vec3 col = texture2D(tex_0, FS_texcoord).rgb;
    gl_FragColor = vec4(col.r*nor, z+.02, col.g*nor, col.b*nor);
#else
    gl_FragColor = vec4(z+.02);
#endif
}

