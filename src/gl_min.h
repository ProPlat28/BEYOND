#pragma once
// Minimal OpenGL 3.3 core loader (replaces GLEW): no third-party dependency, same code on Windows/Linux/macOS.
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <cstddef>
#ifdef _WIN32
#define GLAPI_CALL __stdcall
#else
#define GLAPI_CALL
#endif
typedef unsigned int GLenum, GLuint, GLbitfield; typedef int GLint, GLsizei; typedef float GLfloat; typedef unsigned char GLboolean; typedef ptrdiff_t GLsizeiptr, GLintptr; typedef char GLchar;
#define GL_ARRAY_BUFFER 0x8892
#define GL_BLEND 0x0BE2
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_COLOR_BUFFER_BIT 0x4000
#define GL_COMPILE_STATUS 0x8B81
#define GL_DEPTH_BUFFER_BIT 0x100
#define GL_DEPTH_TEST 0x0B71
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_FLOAT 0x1406
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#define GL_LINEAR 0x2601
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_MULTISAMPLE 0x809D
#define GL_ONE 1
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_SRC_ALPHA 0x0302
#define GL_POINTS 0
#define GL_TRIANGLES 4
#define GL_PROGRAM_POINT_SIZE 0x8642
#define GL_REPEAT 0x2901
#define GL_RGBA 0x1908
#define GL_SRGB8_ALPHA8 0x8C43
#define GL_STATIC_DRAW 0x88E4
#define GL_STREAM_DRAW 0x88E0
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_INT 0x1405
#define GL_LIST(F) \
  F(void, glActiveTexture, (GLenum)) F(void, glAttachShader, (GLuint, GLuint)) F(void, glBindBuffer, (GLenum, GLuint)) F(void, glBindTexture, (GLenum, GLuint)) F(void, glBindVertexArray, (GLuint)) \
  F(void, glBlendFunc, (GLenum, GLenum)) F(void, glBufferData, (GLenum, GLsizeiptr, const void*, GLenum)) F(void, glBufferSubData, (GLenum, GLintptr, GLsizeiptr, const void*)) F(void, glClear, (GLbitfield)) \
  F(void, glClearColor, (GLfloat, GLfloat, GLfloat, GLfloat)) F(void, glCompileShader, (GLuint)) F(GLuint, glCreateProgram, ()) F(GLuint, glCreateShader, (GLenum)) F(void, glDeleteBuffers, (GLsizei, const GLuint*)) \
  F(void, glDeleteTextures, (GLsizei, const GLuint*)) F(void, glDeleteVertexArrays, (GLsizei, const GLuint*)) F(void, glDepthMask, (GLboolean)) F(void, glDisable, (GLenum)) F(void, glDrawArrays, (GLenum, GLint, GLsizei)) \
  F(void, glDrawElements, (GLenum, GLsizei, GLenum, const void*)) F(void, glEnable, (GLenum)) F(void, glEnableVertexAttribArray, (GLuint)) F(void, glGenBuffers, (GLsizei, GLuint*)) F(void, glGenTextures, (GLsizei, GLuint*)) \
  F(void, glGenVertexArrays, (GLsizei, GLuint*)) F(void, glGenerateMipmap, (GLenum)) F(void, glGetShaderInfoLog, (GLuint, GLsizei, GLsizei*, char*)) F(void, glGetShaderiv, (GLuint, GLenum, GLint*)) \
  F(GLint, glGetUniformLocation, (GLuint, const char*)) F(void, glLinkProgram, (GLuint)) F(void, glShaderSource, (GLuint, GLsizei, const char* const*, const GLint*)) \
  F(void, glTexImage2D, (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*)) F(void, glTexParameterf, (GLenum, GLenum, GLfloat)) F(void, glTexParameteri, (GLenum, GLenum, GLint)) \
  F(void, glUniform1f, (GLint, GLfloat)) F(void, glUniform1i, (GLint, GLint)) F(void, glUniform2f, (GLint, GLfloat, GLfloat)) F(void, glUniform3f, (GLint, GLfloat, GLfloat, GLfloat)) \
  F(void, glUniform4f, (GLint, GLfloat, GLfloat, GLfloat, GLfloat)) F(void, glUniformMatrix4fv, (GLint, GLsizei, GLboolean, const GLfloat*)) F(void, glUseProgram, (GLuint)) \
  F(void, glVertexAttribPointer, (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)) F(void, glViewport, (GLint, GLint, GLsizei, GLsizei))
#define GL_DECL(R, N, A) typedef R(GLAPI_CALL* PFN_##N) A; inline PFN_##N N = nullptr;
GL_LIST(GL_DECL)
#undef GL_DECL
// call after glfwMakeContextCurrent(); returns false if any function is missing
inline bool glLoad() {
  bool ok = true;
#define GL_LOAD(R, N, A) N = (PFN_##N)glfwGetProcAddress(#N); if (!N) ok = false;
  GL_LIST(GL_LOAD)
#undef GL_LOAD
  return ok;
}
