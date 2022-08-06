uniform sampler2D tex_0;
uniform lowp vec4      color1;
uniform lowp vec4      color2;
UNIFORMS

varying lowp vec2 FS_texcoord;
varying lowp vec3 FS_n;
varying lowp vec3 FS_t;
varying lowp vec3 FS_b;
VARYINGS

void main(void)
{
    lowp vec4 sample = texture2D(tex_0, FS_texcoord);
    vec3 normal = normalize(2.*sample.xyz-1.);
    lowp vec4 albedo = mix(color1, color2, sample.w*sample.w);

    normal = normalize(FS_t * normal.x + FS_n * normal.z + FS_b * normal.y);

    float diffuse = max(dot(LIGHT, normal), 0.);

    gl_FragColor = SHADOW*albedo*diffuse +
        albedo*(_AMBIENT)*AMBIENT_OCCL;
}

