varying float z;

void main(void)
{
#ifdef ENABLE_GI
    gl_FragColor = vec4(0., z+SHADOW_BIAS, 0., 0.);
#else
    gl_FragColor = vec4(z+SHADOW_BIAS);
#endif
}

