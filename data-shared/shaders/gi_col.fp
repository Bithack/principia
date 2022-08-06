varying float z;
#ifdef ENABLE_GI
varying float nor;
#endif

uniform vec4 color;

void main(void)
{
#ifdef ENABLE_GI
    vec3 col = color.rgb;
    gl_FragColor = vec4(col.r*nor, z+.02, col.g*nor, col.b*nor);
#else
    gl_FragColor = vec4(z+.02);
#endif
}

