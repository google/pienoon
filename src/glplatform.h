// Copyright 2014 Wouter van Oortmerssen. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// OpenGL platform definitions

#ifndef GLPLATFORM_H
#define GLPLATFORM_H

#ifdef __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_OS_IPHONE
        #define PLATFORM_MOBILE
        //#include <SDL_opengles2.h>
        #include <OpenGLES/ES2/gl.h>
        #include <OpenGLES/ES2/glext.h>
    #else
        #include <OpenGL/gl.h>
    #endif
#else
    #ifdef __ANDROID__
        #define PLATFORM_MOBILE
        #include <GLES2/gl2.h>
        #include <GLES2/gl2ext.h>
    #else   // WIN32 & Linux
        #ifdef _WIN32
            #define VC_EXTRALEAN
            #ifndef WIN32_LEAN_AND_MEAN
                #define WIN32_LEAN_AND_MEAN
            #endif
            #define NOMINMAX
            #include <windows.h>
        #endif
        #include <GL/gl.h>
        #include <GL/glext.h>
        #ifdef _WIN32
            #define GLBASEEXTS \
                GLEXT(PFNGLACTIVETEXTUREARBPROC       , glActiveTexture           )
        #else
            #define GLBASEEXTS
        #endif
        #define GLEXTS \
            GLEXT(PFNGLGENBUFFERSARBPROC              , glGenBuffers              ) \
            GLEXT(PFNGLBINDBUFFERARBPROC              , glBindBuffer              ) \
            GLEXT(PFNGLMAPBUFFERARBPROC               , glMapBuffer               ) \
            GLEXT(PFNGLUNMAPBUFFERARBPROC             , glUnmapBuffer             ) \
            GLEXT(PFNGLBUFFERDATAARBPROC              , glBufferData              ) \
            GLEXT(PFNGLBUFFERSUBDATAARBPROC           , glBufferSubData           ) \
            GLEXT(PFNGLDELETEBUFFERSARBPROC           , glDeleteBuffers           ) \
            GLEXT(PFNGLGETBUFFERSUBDATAARBPROC        , glGetBufferSubData        ) \
            GLEXT(PFNGLVERTEXATTRIBPOINTERARBPROC     , glVertexAttribPointer     ) \
            GLEXT(PFNGLENABLEVERTEXATTRIBARRAYARBPROC , glEnableVertexAttribArray ) \
            GLEXT(PFNGLDISABLEVERTEXATTRIBARRAYARBPROC, glDisableVertexAttribArray) \
            GLEXT(PFNGLCREATEPROGRAMPROC              , glCreateProgram           ) \
            GLEXT(PFNGLDELETEPROGRAMPROC              , glDeleteProgram           ) \
            GLEXT(PFNGLDELETESHADERPROC               , glDeleteShader            ) \
            GLEXT(PFNGLUSEPROGRAMPROC                 , glUseProgram              ) \
            GLEXT(PFNGLCREATESHADERPROC               , glCreateShader            ) \
            GLEXT(PFNGLSHADERSOURCEPROC               , glShaderSource            ) \
            GLEXT(PFNGLCOMPILESHADERPROC              , glCompileShader           ) \
            GLEXT(PFNGLGETPROGRAMIVARBPROC            , glGetProgramiv            ) \
            GLEXT(PFNGLGETSHADERIVPROC                , glGetShaderiv             ) \
            GLEXT(PFNGLGETPROGRAMINFOLOGPROC          , glGetProgramInfoLog       ) \
            GLEXT(PFNGLGETSHADERINFOLOGPROC           , glGetShaderInfoLog        ) \
            GLEXT(PFNGLATTACHSHADERPROC               , glAttachShader            ) \
            GLEXT(PFNGLLINKPROGRAMARBPROC             , glLinkProgram             ) \
            GLEXT(PFNGLGETUNIFORMLOCATIONARBPROC      , glGetUniformLocation      ) \
            GLEXT(PFNGLUNIFORM1FARBPROC               , glUniform1f               ) \
            GLEXT(PFNGLUNIFORM2FARBPROC               , glUniform2f               ) \
            GLEXT(PFNGLUNIFORM3FARBPROC               , glUniform3f               ) \
            GLEXT(PFNGLUNIFORM4FARBPROC               , glUniform4f               ) \
            GLEXT(PFNGLUNIFORM1FVARBPROC              , glUniform1fv              ) \
            GLEXT(PFNGLUNIFORM2FVARBPROC              , glUniform2fv              ) \
            GLEXT(PFNGLUNIFORM3FVARBPROC              , glUniform3fv              ) \
            GLEXT(PFNGLUNIFORM4FVARBPROC              , glUniform4fv              ) \
            GLEXT(PFNGLUNIFORM1IARBPROC               , glUniform1i               ) \
            GLEXT(PFNGLUNIFORMMATRIX4FVARBPROC        , glUniformMatrix4fv        ) \
            GLEXT(PFNGLUNIFORMMATRIX4FVARBPROC/*type*/, glUniformMatrix3x4fv      ) \
            GLEXT(PFNGLBINDATTRIBLOCATIONARBPROC      , glBindAttribLocation      ) \
            GLEXT(PFNGLGETACTIVEUNIFORMARBPROC        , glGetActiveUniform        ) \
            GLEXT(PFNGLGENERATEMIPMAPEXTPROC          , glGenerateMipmap          )

        #define GLEXT(type, name) extern type name;
            GLBASEEXTS
            GLEXTS
        #undef GLEXT
    #endif
#endif

// Define a GL_CALL macro to wrap each (void-returning) OpenGL call.
// This logs GL error when LOG_GL_ERRORS below is defined.
#if defined(_DEBUG) || DEBUG==1
    #define LOG_GL_ERRORS
#endif
#ifdef LOG_GL_ERRORS
    #define GL_CALL(call) { call; LogGLError(__FILE__, __LINE__, #call); }
#else
    #define GL_CALL(call) call
#endif

// The error checking function used by the GL_CALL macro above,
// uses glGetError() to check for errors.
extern void LogGLError(const char *file, int line, const char *call);

#endif  // GLPLATFORM_H
