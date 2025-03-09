varying float z;

void main(void)
{
    gl_FragColor = vec4(z+SHADOW_BIAS);
}

