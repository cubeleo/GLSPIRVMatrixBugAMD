#include <Windows.h>

#include "GLFW/glfw3.h"

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>

#define GL_ACTIVE_UNIFORMS                0x8B86
#define GL_VERTEX_SHADER                  0x8B31
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_SHADER_BINARY_FORMAT_SPIR_V    0x9551

#define GL_FLOAT_VEC2 0x8B50
#define GL_FLOAT_VEC3 0x8B51
#define GL_FLOAT_VEC4 0x8B52
#define GL_FLOAT_MAT2 0x8B5A
#define GL_FLOAT_MAT3 0x8B5B
#define GL_FLOAT_MAT4 0x8B5C
#define GL_SAMPLER_2D 0x8B5E
#define GL_SAMPLER_3D 0x8B5F
#define GL_SAMPLER_CUBE 0x8B60
#define GL_SAMPLER_2D_SHADOW 0x8B62

typedef char GLchar;
typedef uint64_t GLuint64;
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;

char const * StringFromTypeEnum(int typeEnum)
{
    switch (typeEnum)
    {
        case GL_FLOAT_VEC2:
            return "GL_FLOAT_VEC2";
        case GL_FLOAT_VEC3:
            return "GL_FLOAT_VEC3";
        case GL_FLOAT_VEC4:
            return "GL_FLOAT_VEC4";
        case GL_FLOAT_MAT2:
            return "GL_FLOAT_MAT2";
        case GL_FLOAT_MAT3:
            return "GL_FLOAT_MAT3";
        case GL_FLOAT_MAT4:
            return "GL_FLOAT_MAT4";
        case GL_SAMPLER_2D:
            return "GL_SAMPLER_2D";
        case GL_SAMPLER_3D:
            return "GL_SAMPLER_3D";
        case GL_SAMPLER_CUBE:
            return "GL_SAMPLER_CUBE";
        case GL_SAMPLER_2D_SHADOW:
            return "GL_SAMPLER_2D_SHADOW";
        default:
            return "(unknown)";
    }
}

bool ReadSpirv(std::vector<uint8_t> * spirv, char const * path)
{
    FILE * f = fopen(path, "rb");
    if (!f)
    {
        printf("Could not open %s.\n", path);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    spirv->resize(size);
    fread(spirv->data(), 1, size, f);
    fclose(f);

    return true;
}

int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow * window = glfwCreateWindow(640, 480, "SPIRVMatrixBug", nullptr, nullptr);

    glfwMakeContextCurrent(window);

    HINSTANCE dll = LoadLibraryA("opengl32.dll");
    typedef PROC WINAPI wglGetProcAddressProc(LPCSTR lpszProc);
    if (!dll)
    {
        OutputDebugStringA("opengl32.dll not found.\n");
        return false;
    }

    wglGetProcAddressProc* wglGetProcAddress =
        (wglGetProcAddressProc*)GetProcAddress(dll, "wglGetProcAddress");

    typedef void glBindBufferProc(GLenum target, GLuint buffer);
    glBindBufferProc * glBindBuffer = (glBindBufferProc *)wglGetProcAddress("glBindBuffer");
    typedef GLuint glCreateShaderProc(GLenum type);
    glCreateShaderProc * glCreateShader = (glCreateShaderProc *)wglGetProcAddress("glCreateShader");
    typedef void glShaderBinaryProc(GLsizei count, const GLuint * shaders, GLenum binaryFormat, const void * binary, GLsizei length);
    glShaderBinaryProc * glShaderBinary = (glShaderBinaryProc *)wglGetProcAddress("glShaderBinary");
    typedef void glSpecializeShaderProc(GLuint shader, const GLchar * pEntryPoint, GLuint numSpecializationConstants, const GLuint * pConstantIndex, const GLuint * pConstantValue);
    glSpecializeShaderProc * glSpecializeShader = (glSpecializeShaderProc *)wglGetProcAddress("glSpecializeShader");
    typedef GLuint glCreateProgramProc();
    glCreateProgramProc * glCreateProgram = (glCreateProgramProc *)wglGetProcAddress("glCreateProgram");
    typedef GLuint glAttachShaderProc(GLuint program, GLuint shader);
    glAttachShaderProc * glAttachShader = (glAttachShaderProc *)wglGetProcAddress("glAttachShader");
    typedef GLuint glLinkProgramProc(GLuint program);
    glLinkProgramProc * glLinkProgram = (glLinkProgramProc *)wglGetProcAddress("glLinkProgram");
    typedef void glGetProgramivProc(GLuint program, GLenum pname, GLint *params);
    glGetProgramivProc * glGetProgramiv = (glGetProgramivProc *)wglGetProcAddress("glGetProgramiv");
    typedef void glGetActiveUniformProc(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
    glGetActiveUniformProc * glGetActiveUniform = (glGetActiveUniformProc *)wglGetProcAddress("glGetActiveUniform");

    std::vector<uint8_t> vertexSpirv;
    if (!ReadSpirv(&vertexSpirv, "Shader.vert.spv"))
    {
        return -1;
    }

    std::vector<uint8_t> fragmentSpirv;
    if (!ReadSpirv(&fragmentSpirv, "Shader.frag.spv"))
    {
        return -1;
    }

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderBinary(1, &vertexShader, GL_SHADER_BINARY_FORMAT_SPIR_V, vertexSpirv.data(), vertexSpirv.size());
    glSpecializeShader(vertexShader, "main", 0, 0, 0);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderBinary(1, &fragmentShader, GL_SHADER_BINARY_FORMAT_SPIR_V, fragmentSpirv.data(), fragmentSpirv.size());
    glSpecializeShader(fragmentShader, "main", 0, 0, 0);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint uniformCount;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniformCount);
    printf("Uniform count is %d\n", uniformCount);

    for (GLint ii = 0; ii < uniformCount; ++ii)
    {
        GLint size;
        GLenum type;
        glGetActiveUniform(program, ii, 0, nullptr, &size, &type, nullptr);

        // I'm expecting GL_FLOAT_MAT4 here.
        // NVidia gives me a GL_FLOAT_MAT4, but AMD gives me a GL_FLOAT_VEC4 array!
        printf("Type is %s\n", StringFromTypeEnum(type));
    }

    glfwTerminate();

    return 0;
}
