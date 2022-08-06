UNIFORMS

varying lowp float FS_diffuse;
VARYINGS

void main(void)
{
    float ambient = _AMBIENT;
    float shadow = SHADOW;

    float o = FS_diffuse * .7;
    vec4 color = vec4(FS_diffuse, FS_diffuse, o, 0.);
    vec4 am = vec4(ambient);

    gl_FragColor = shadow * ambient + color;
}

