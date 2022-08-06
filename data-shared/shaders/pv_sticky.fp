uniform sampler2D tex_0;
UNIFORMS
uniform lowp vec4 color;

varying mediump vec4 FS_diffuse_texcoord;
varying lowp vec3 FS_normal;
varying lowp vec3 FS_eye;
VARYINGS

void main(void)
{
    float ambient = _AMBIENT;

    float shadow = SHADOW;

    vec4 col = color;/* - texture2D(tex_0, FS_texcoord).rrrr;*/

    float s = texture2D(tex_0, FS_diffuse_texcoord.zw).r;

    col = mix(color, vec4(.0, .0, .0, 1.), s);

    vec3 n = FS_normal;
    vec3 e = FS_eye;
    vec3 R = normalize(reflect(LIGHT, n));
    float specular = pow(clamp(dot(R, e), .0, 1.), 6.);

    gl_FragColor = FS_diffuse_texcoord.x*specular*.5 + shadow * col * FS_diffuse_texcoord.x + col * ambient;
}

