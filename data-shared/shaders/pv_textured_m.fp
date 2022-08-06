uniform sampler2D tex_0;

varying mediump float FS_diffuse;
varying mediump vec2 FS_texcoord;

void main(void)
{
    float ambient =  AMBIENT_M;
    vec4 color = texture2D(tex_0, FS_texcoord);
    gl_FragColor = vec4((color.rgb * FS_diffuse + color.rgb * ambient), color.a);
}

