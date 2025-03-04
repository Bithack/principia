uniform sampler2D tex_0;

UNIFORMS

varying lowp vec3 FS_uv;
VARYINGS

void main(void)
{
    float ambient = _AMBIENT;
    float shadow = SHADOW;

    vec4 col = texture2D(tex_0, FS_uv.xy);

    gl_FragColor = vec4(vec3(FS_uv.z), .75) * col * (shadow*.5+ambient*2.);
}

