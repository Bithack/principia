
#undef glGenVertexArrays
extern void APIENTRY (*glGenVertexArrays) (GLsizei n, GLuint *arrays);
#undef glBindVertexArray
extern void APIENTRY (*glBindVertexArray) (GLuint array);
#undef glGenBuffers
extern void APIENTRY (*glGenBuffers) (GLsizei n, GLuint *buffers);
#undef glBindBuffer
extern void APIENTRY (*glBindBuffer) (GLenum target, GLuint buffer);
#undef glBufferData
extern void APIENTRY (*glBufferData) (GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
#undef glVertexAttribPointer
extern void APIENTRY (*glVertexAttribPointer) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
#undef glGetUniformLocation
extern GLint APIENTRY (*glGetUniformLocation) (GLuint program, const GLchar *name);
#undef glUseProgram
extern void APIENTRY (*glUseProgram) (GLuint program);
#undef glGetAttribLocation
extern GLint APIENTRY (*glGetAttribLocation) (GLuint program, const GLchar *name);
#undef glCreateShader
extern GLuint APIENTRY (*glCreateShader) (GLenum type);
#undef glShaderSource
extern void APIENTRY (*glShaderSource) (GLuint shader, GLsizei count, const GLchar* *string, const GLint *length);
#undef glCreateProgram
extern GLuint APIENTRY (*glCreateProgram) (void);
#undef glAttachShader
extern void APIENTRY (*glAttachShader) (GLuint program, GLuint shader);
#undef glLinkProgram
extern void APIENTRY (*glLinkProgram) (GLuint program);
#undef glCompileShader
extern void APIENTRY (*glCompileShader) (GLuint shader);
#undef glGetProgramInfoLog
extern void APIENTRY (*glGetProgramInfoLog) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
#undef glGetShaderInfoLog
extern void APIENTRY (*glGetShaderInfoLog) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
#undef glUniformMatrix4fv
extern void APIENTRY (*glUniformMatrix4fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
#undef glEnableVertexAttribArray
extern void APIENTRY (*glEnableVertexAttribArray) (GLuint index);
#undef glDisableVertexAttribArray
extern void APIENTRY (*glDisableVertexAttribArray) (GLuint index);
#undef glBindAttribLocation
extern void APIENTRY (*glBindAttribLocation) (GLuint program, GLuint index, const GLchar *name);
