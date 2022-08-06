uniform sampler2D tex_0;
uniform vec4 color;
UNIFORMS

varying lowp float FS_diffuse;
varying lowp vec2 FS_texcoord;
VARYINGS

void main(void)
{
    vec4 c = texture2D(tex_0, FS_texcoord);
    gl_FragColor = SHADOW * c * color * FS_diffuse + c.a * c * color * (_AMBIENT)*AMBIENT_OCCL;
}

