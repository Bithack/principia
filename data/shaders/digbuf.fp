UNIFORMS

varying lowp float FS_diffuse;
VARYINGS

void main(void)
{
    float ambient = .15;
    float shadow = SHADOW;

    float o = FS_diffuse * .7;
    vec4 color = vec4(o, FS_diffuse, o, 0.);
    vec4 am = vec4(ambient);

    gl_FragColor = shadow * ambient + color;
}

