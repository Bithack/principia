uniform vec4 color;
UNIFORMS

VARYINGS
varying lowp vec3 FS_n;

void main(void)
{
    gl_FragColor.xyz = color.xyz*(.9 + dot(FS_n, vec3(0.,0.,1.)*.1));
    gl_FragColor.a = color.a;
}

