uniform sampler2D tex_0;
uniform sampler2D tex_1;
UNIFORMS

varying mediump float FS_diffuse;
varying lowp vec2 FS_texcoord;
varying lowp vec2 FS_texcoord2;
varying mediump vec3 FS_normal;
varying mediump vec3 FS_eye;
VARYINGS

void main(void)
{
    vec4 color = texture2D(tex_0, FS_texcoord);
    vec3 reflection = texture2D(tex_1, FS_texcoord2).rgb;

    vec3 n = normalize(FS_normal);
    vec3 e = normalize(FS_eye);
    vec3 R = normalize(reflect(LIGHT, n));
    float specular = pow(clamp(dot(R, e), .0, 1.), 5.);

    //if (color.b > .2) {
    //color.rgb += (color.b > .7 ? 1. : .1) * ((n.z) * .6 * (texture2D(tex_1, FS_texcoord2).rgb -vec3(.7)));
    color.rgb += (.1+max(.0, color.b-.7) * 20.0) * ((n.z) * .7 * (reflection - vec3(.7)));
    //}

    gl_FragColor = SHADOW * (color+color*specular) * FS_diffuse +
        color * (_AMBIENT)*AMBIENT_OCCL;
}

