uniform vec4 color;
varying mediump float FS_diffuse;

void main(void)
{
    float ambient = AMBIENT_M;
    /*gl_FragColor = pow(color * FS_diffuse + color * ambient, vec4(vec3(1./2.2), 1.));*/
    gl_FragColor = vec4(color.rgb * FS_diffuse + color.rgb * ambient, color.a);
}

