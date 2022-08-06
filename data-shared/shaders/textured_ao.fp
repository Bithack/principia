uniform sampler2D tex_0;
UNIFORMS

varying lowp float FS_diffuse;
varying lowp vec2 FS_texcoord;
varying mediump vec3 FS_normal;
varying mediump vec3 FS_eye;
VARYINGS

void main(void)
{
    vec4 color = texture2D(tex_0, FS_texcoord);
    vec3 n = normalize(FS_normal);
    vec3 e = normalize(FS_eye);
    vec3 R = normalize(reflect(LIGHT, n));
    float shadow = SHADOW;
    float specular = pow(clamp(dot(R, e), .0, 2.), 3.);
    gl_FragColor = specular*shadow*vec4(0.5) + shadow * color * FS_diffuse + color.a * color * (_AMBIENT)*AMBIENT_OCCL;
}

