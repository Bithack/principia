uniform vec4 color;
UNIFORMS

varying mediump vec3 FS_eye;
varying mediump vec3 FS_normal;
VARYINGS

GI_FUN

void main(void)
{
    vec3 n = normalize(FS_normal);
    float diffuse = clamp(dot(LIGHT, n)*_DIFFUSE, 0., 1.);
    float ambient = _AMBIENT + .25*n.z;

    vec3 e = normalize(FS_eye);
    vec3 R = normalize(reflect(LIGHT, n));
    float specular = pow(clamp(max(dot(R, e), .0), 0., 1.), 18.);
    specular += pow(clamp(max(dot(vec3(0.,0.,1), n), .0), 0., 1.), 18.);

    float shadow = clamp(SHADOW, .5, 1.);

    gl_FragColor = specular*vec4(.7) + shadow * color * diffuse + color * ambient GI;
}

