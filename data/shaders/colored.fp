uniform vec4 color;
UNIFORMS

varying mediump vec3 FS_eye;
varying mediump vec3 FS_normal;
VARYINGS

void main(void)
{
    vec3 n = normalize(FS_normal);
    float diffuse = clamp(dot(LIGHT, n)*_DIFFUSE, 0., 1.);
    float ambient = _AMBIENT + .25*n.z;

    vec3 e = normalize(FS_eye);
    vec3 R = normalize(reflect(LIGHT, n));
    float specular = pow(clamp(max(dot(R, e), .0), .0, 1.), 12.);

    float shadow = SHADOW;

    gl_FragColor = specular*vec4(.6, .6, .6, 1.)*shadow + shadow * color * diffuse + color * ambient;
}

