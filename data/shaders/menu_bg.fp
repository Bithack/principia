uniform sampler2D tex_0;
uniform vec4      color;
varying lowp vec2 FS_texcoord;

void main() {
    gl_FragColor = texture2D(tex_0, FS_texcoord)*color;
}
