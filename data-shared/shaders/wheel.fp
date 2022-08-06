uniform sampler2D tex_0;
UNIFORMS

varying vec4 FS_diffuse_texcoord;
varying mediump vec3 FS_normal;
varying mediump vec3 FS_eye;
VARYINGS

void main(void)
{
    float ambient = _AMBIENT;

    float shadow = SHADOW;

    vec3 n = normalize(FS_normal);
    vec3 e = normalize(FS_eye);
    vec3 R = normalize(reflect(LIGHT, n));
    float specular = pow(clamp(dot(R, e), .0, 1.), 5.);
    /*float specular = pow(max(dot(R, e), .0), 5);*/

    vec4 color = texture2D(tex_0, FS_diffuse_texcoord.zw);

    gl_FragColor = color*specular*shadow + shadow * color * FS_diffuse_texcoord.x + (color.a+.3) * color * _AMBIENT*AMBIENT_OCCL;
}

