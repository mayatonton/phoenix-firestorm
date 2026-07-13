/**
 * @file llglheaders.h
 * @brief LLGL definitions
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLGLHEADERS_H
#define LL_LLGLHEADERS_H

#if LL_DARWIN
// LL_DARWIN

#define GL_GLEXT_LEGACY
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#define GL_EXT_separate_specular_color 1
#define GL_GLEXT_PROTOTYPES
#include "GL/glext.h"

#include "GL/glh_extensions.h"

// These symbols don't exist on 10.3.9, so they have to be declared weak.  Redeclaring them here fixes the problem.
// Note that they also must not be called on 10.3.9.  This should be taken care of by a runtime check for the existence of the GL extension.
#include <AvailabilityMacros.h>

// <FS:ND> Workaround to get a OSX version to compile (until someone with a Mac makes including gl3.h work)
#ifndef GL_TEXTURE_RECTANGLE
  #define GL_TEXTURE_RECTANGLE GL_TEXTURE_RECTANGLE_ARB
#endif
// </FS:ND>

//GL_EXT_blend_func_separate
extern void glBlendFuncSeparateEXT(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) ;

// GL_EXT_framebuffer_object
extern GLboolean glIsRenderbufferEXT(GLuint renderbuffer) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glBindRenderbufferEXT(GLenum target, GLuint renderbuffer) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glDeleteRenderbuffersEXT(GLsizei n, const GLuint *renderbuffers) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glGenRenderbuffersEXT(GLsizei n, GLuint *renderbuffers) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glGetRenderbufferParameterivEXT(GLenum target, GLenum pname, GLint *params) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern GLboolean glIsFramebufferEXT(GLuint framebuffer) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glBindFramebufferEXT(GLenum target, GLuint framebuffer) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glDeleteFramebuffersEXT(GLsizei n, const GLuint *framebuffers) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glGenFramebuffersEXT(GLsizei n, GLuint *framebuffers) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern GLenum glCheckFramebufferStatusEXT(GLenum target) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glFramebufferTexture1DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glFramebufferTexture2DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glFramebufferTexture3DEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glFramebufferRenderbufferEXT(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glGetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment, GLenum pname, GLint *params) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
extern void glGenerateMipmapEXT(GLenum target) AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

#ifndef GL_ARB_framebuffer_object
#define glGenerateMipmap glGenerateMipmapEXT
#define GL_MAX_SAMPLES  0x8D57
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Define map buffer range headers on Mac
//
#ifndef GL_ARB_map_buffer_range
#define GL_MAP_READ_BIT                   0x0001
#define GL_MAP_WRITE_BIT                  0x0002
#define GL_MAP_INVALIDATE_RANGE_BIT       0x0004
#define GL_MAP_INVALIDATE_BUFFER_BIT      0x0008
#define GL_MAP_FLUSH_EXPLICIT_BIT         0x0010
#define GL_MAP_UNSYNCHRONIZED_BIT         0x0020
#endif

//
// Define multisample headers on Mac
//
#ifndef GL_ARB_texture_multisample
#define GL_SAMPLE_POSITION                0x8E50
#define GL_SAMPLE_MASK                    0x8E51
#define GL_SAMPLE_MASK_VALUE              0x8E52
#define GL_MAX_SAMPLE_MASK_WORDS          0x8E59
#define GL_TEXTURE_2D_MULTISAMPLE         0x9100
#define GL_PROXY_TEXTURE_2D_MULTISAMPLE   0x9101
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY   0x9102
#define GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9103
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE 0x9104
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY 0x9105
#define GL_TEXTURE_SAMPLES                0x9106
#define GL_TEXTURE_FIXED_SAMPLE_LOCATIONS 0x9107
#define GL_SAMPLER_2D_MULTISAMPLE         0x9108
#define GL_INT_SAMPLER_2D_MULTISAMPLE     0x9109
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE 0x910A
#define GL_SAMPLER_2D_MULTISAMPLE_ARRAY   0x910B
#define GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910C
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910D
#define GL_MAX_COLOR_TEXTURE_SAMPLES      0x910E
#define GL_MAX_DEPTH_TEXTURE_SAMPLES      0x910F
#define GL_MAX_INTEGER_SAMPLES            0x9110
#endif

//
// Define vertex buffer object headers on Mac
//
#ifndef GL_ARB_vertex_buffer_object
#define GL_BUFFER_SIZE_ARB                0x8764
#define GL_BUFFER_USAGE_ARB               0x8765
#define GL_ARRAY_BUFFER_ARB               0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB       0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB       0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB 0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB 0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB 0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB 0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB 0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB 0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB 0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB 0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB 0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB 0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB 0x889F
#define GL_READ_ONLY_ARB                  0x88B8
#define GL_WRITE_ONLY_ARB                 0x88B9
#define GL_READ_WRITE_ARB                 0x88BA
#define GL_BUFFER_ACCESS_ARB              0x88BB
#define GL_BUFFER_MAPPED_ARB              0x88BC
#define GL_BUFFER_MAP_POINTER_ARB         0x88BD
#define GL_STREAM_DRAW_ARB                0x88E0
#define GL_STREAM_READ_ARB                0x88E1
#define GL_STREAM_COPY_ARB                0x88E2
#define GL_STATIC_DRAW_ARB                0x88E4
#define GL_STATIC_READ_ARB                0x88E5
#define GL_STATIC_COPY_ARB                0x88E6
#define GL_DYNAMIC_DRAW_ARB               0x88E8
#define GL_DYNAMIC_READ_ARB               0x88E9
#define GL_DYNAMIC_COPY_ARB               0x88EA
#endif



#ifndef GL_ARB_vertex_buffer_object
/* GL types for handling large vertex buffer objects */
typedef intptr_t GLintptr;
typedef intptr_t GLsizeiptr;
#endif


#ifndef GL_ARB_vertex_buffer_object
#define GL_ARB_vertex_buffer_object 1
#ifdef GL_GLEXT_FUNCTION_POINTERS
typedef void (* glBindBufferARBProcPtr) (GLenum target, GLuint buffer);
typedef void (* glDeleteBufferARBProcPtr) (GLsizei n, const GLuint *buffers);
typedef void (* glGenBuffersARBProcPtr) (GLsizei n, GLuint *buffers);
typedef GLboolean (* glIsBufferARBProcPtr) (GLuint buffer);
typedef void (* glBufferDataARBProcPtr) (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
typedef void (* glBufferSubDataARBProcPtr) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
typedef void (* glGetBufferSubDataARBProcPtr) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid *data);
typedef GLvoid* (* glMapBufferARBProcPtr) (GLenum target, GLenum access);   /* Flawfinder: ignore */
typedef GLboolean (* glUnmapBufferARBProcPtr) (GLenum target);
typedef void (* glGetBufferParameterivARBProcPtr) (GLenum target, GLenum pname, GLint *params);
typedef void (* glGetBufferPointervARBProcPtr) (GLenum target, GLenum pname, GLvoid* *params);
#else
extern void glBindBufferARB (GLenum, GLuint);
extern void glDeleteBuffersARB (GLsizei, const GLuint *);
extern void glGenBuffersARB (GLsizei, GLuint *);
extern GLboolean glIsBufferARB (GLuint);
extern void glBufferDataARB (GLenum, GLsizeiptrARB, const GLvoid *, GLenum);
extern void glBufferSubDataARB (GLenum, GLintptrARB, GLsizeiptrARB, const GLvoid *);
extern void glGetBufferSubDataARB (GLenum, GLintptrARB, GLsizeiptrARB, GLvoid *);
extern GLvoid* glMapBufferARB (GLenum, GLenum);
extern GLboolean glUnmapBufferARB (GLenum);
extern void glGetBufferParameterivARB (GLenum, GLenum, GLint *);
extern void glGetBufferPointervARB (GLenum, GLenum, GLvoid* *);
#endif /* GL_GLEXT_FUNCTION_POINTERS */
#endif

#ifndef GL_ARB_texture_rg
#define GL_RG                             0x8227
#define GL_RG_INTEGER                     0x8228
#define GL_R8                             0x8229
#define GL_R16                            0x822A
#define GL_RG8                            0x822B
#define GL_RG16                           0x822C
#define GL_R16F                           0x822D
#define GL_R32F                           0x822E
#define GL_RG16F                          0x822F
#define GL_RG32F                          0x8230
#define GL_R8I                            0x8231
#define GL_R8UI                           0x8232
#define GL_R16I                           0x8233
#define GL_R16UI                          0x8234
#define GL_R32I                           0x8235
#define GL_R32UI                          0x8236
#define GL_RG8I                           0x8237
#define GL_RG8UI                          0x8238
#define GL_RG16I                          0x8239
#define GL_RG16UI                         0x823A
#define GL_RG32I                          0x823B
#define GL_RG32UI                         0x823C
#endif

// May be needed for DARWIN...
// #ifndef GL_ARB_compressed_tex_image
// #define GL_ARB_compressed_tex_image 1
// #ifdef GL_GLEXT_FUNCTION_POINTERS
// typedef void (* glCompressedTexImage1D) (GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid*);
// typedef void (* glCompressedTexImage2D) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
// typedef void (* glCompressedTexImage3D) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
// typedef void (* glCompressedTexSubImage1D) (GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid*);
// typedef void (* glCompressedTexSubImage2D) (GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
// typedef void (* glCompressedTexSubImage3D) (GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
// typedef void (* glGetCompressedTexImage) (GLenum, GLint, GLvoid*);
// #else
// extern void glCompressedTexImage1D (GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid*);
// extern void glCompressedTexImage2D (GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
// extern void glCompressedTexImage3D (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
// extern void glCompressedTexSubImage1D (GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid*);
// extern void glCompressedTexSubImage2D (GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
// extern void glCompressedTexSubImage3D (GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
// extern void glGetCompressedTexImage (GLenum, GLint, GLvoid*);
// #endif /* GL_GLEXT_FUNCTION_POINTERS */
// #endif

#ifdef __cplusplus
}
#endif

#include <OpenGL/gl.h>

#else

typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef signed char    GLbyte;
typedef short          GLshort;
typedef int            GLint;
typedef int            GLsizei;
typedef unsigned char  GLubyte;
typedef unsigned short GLushort;
typedef unsigned int   GLuint;
typedef float          GLfloat;
typedef float          GLclampf;
typedef double         GLdouble;
typedef double         GLclampd;
typedef char           GLchar;
typedef void           GLvoid;

#define GL_ALPHA 0x1906
#define GL_ALPHA8 0x803C
#define GL_ALPHA_TEST 0x0BC0
#define GL_ALWAYS 0x0207
#define GL_ARB_texture_multisample 1
#define GL_ARRAY_BUFFER 0x8892
#define GL_BACK 0x0405
#define GL_BGRA 0x80E1
#define GL_BGRA_EXT 0x80E1
#define GL_BLEND 0x0BE2
#define GL_BYTE 0x1400
#define GL_CLIP_PLANE0 0x3000
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_COLOR_ATTACHMENT0_EXT 0x8CE0
#define GL_COLOR_ATTACHMENT1 0x8CE1
#define GL_COLOR_ATTACHMENT1_EXT 0x8CE1
#define GL_COLOR_ATTACHMENT2 0x8CE2
#define GL_COLOR_ATTACHMENT2_EXT 0x8CE2
#define GL_COLOR_ATTACHMENT3 0x8CE3
#define GL_COLOR_ATTACHMENT3_EXT 0x8CE3
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_COLOR_INDEX 0x1900
#define GL_COMPRESSED_ALPHA 0x84E9
#define GL_COMPRESSED_LUMINANCE 0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA 0x84EB
#define GL_COMPRESSED_RED 0x8225
#define GL_COMPRESSED_RG 0x8226
#define GL_COMPRESSED_RGB 0x84ED
#define GL_COMPRESSED_RGBA 0x84EE
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#define GL_COMPRESSED_SRGB 0x8C48
#define GL_COMPRESSED_SRGB_ALPHA 0x8C49
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#define GL_CONSTANT 0x8576
#define GL_CULL_FACE 0x0B44
#define GL_DEPTH24_STENCIL8 0x88F0
#define GL_DEPTH24_STENCIL8_EXT 0x88F0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_DEPTH_ATTACHMENT_EXT 0x8D00
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_DEPTH_CLAMP 0x864F
#define GL_DEPTH_COMPONENT 0x1902
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_DEPTH_TEST 0x0B71
#define GL_DITHER 0x0BD0
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_DRAW_FRAMEBUFFER_EXT 0x8CA9
#define GL_DST_ALPHA 0x0304
#define GL_DST_COLOR 0x0306
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_EQUAL 0x0202
#define GL_FALSE 0
#define GL_FILL 0x1B02
#define GL_FLOAT 0x1406
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_FRAGMENT_SHADER_ARB 0x8B30
#define GL_FRAMEBUFFER 0x8D40
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_FRAMEBUFFER_COMPLETE_EXT 0x8CD5
#define GL_FRAMEBUFFER_EXT 0x8D40
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT 0x8CD7
#define GL_FRAMEBUFFER_UNSUPPORTED 0x8CDD
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT 0x8CDD
#define GL_FRONT 0x0404
#define GL_FRONT_AND_BACK 0x0408
#define GL_GEOMETRY_SHADER 0x8DD9
#define GL_GEQUAL 0x0206
#define GL_GREATER 0x0204
#define GL_HALF_FLOAT 0x140B
#define GL_INT 0x1404
#define GL_KEEP 0x1E00
#define GL_LEQUAL 0x0203
#define GL_LESS 0x0201
#define GL_LINE 0x1B01
#define GL_LINES 0x0001
#define GL_LINE_LOOP 0x0002
#define GL_LINE_STRIP 0x0003
#define GL_LUMINANCE 0x1909
#define GL_LUMINANCE8 0x8040
#define GL_LUMINANCE8_ALPHA8 0x8045
#define GL_LUMINANCE_ALPHA 0x190A
#define GL_MODELVIEW_MATRIX 0x0BA6
#define GL_MULTISAMPLE 0x809D
#define GL_MULTISAMPLE_ARB 0x809D
#define GL_ONE 1
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_ONE_MINUS_DST_COLOR 0x0307
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_POINTS 0x0000
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_POLYGON_OFFSET_LINE 0x2A02
#define GL_PREVIOUS 0x8578
#define GL_PRIMARY_COLOR 0x8577
#define GL_R11F_G11F_B10F 0x8C3A
#define GL_R16F 0x822D
#define GL_R32F 0x822E
#define GL_R8 0x8229
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_READ_FRAMEBUFFER_EXT 0x8CA8
#define GL_RED 0x1903
#define GL_RENDERBUFFER 0x8D41
#define GL_RENDERBUFFER_EXT 0x8D41
#define GL_RENDERER 0x1F01
#define GL_REPLACE 0x1E01
#define GL_RG 0x8227
#define GL_RG16F 0x822F
#define GL_RG32F 0x8230
#define GL_RG8 0x822B
#define GL_RGB 0x1907
#define GL_RGB10_A2 0x8059
#define GL_RGB16F 0x881B
#define GL_RGB32F 0x8815
#define GL_RGB8 0x8051
#define GL_RGBA 0x1908
#define GL_RGBA16 0x805B
#define GL_RGBA16F 0x881A
#define GL_RGBA32F 0x8814
#define GL_RGBA8 0x8058
#define GL_SCISSOR_TEST 0x0C11
#define GL_SHORT 0x1402
#define GL_SRC_ALPHA 0x0302
#define GL_SRC_COLOR 0x0300
#define GL_SRGB 0x8C40
#define GL_SRGB8_ALPHA8 0x8C43
#define GL_SRGB_ALPHA 0x8C42
#define GL_STENCIL_ATTACHMENT 0x8D20
#define GL_STENCIL_ATTACHMENT_EXT 0x8D20
#define GL_STENCIL_TEST 0x0B90
#define GL_TEXTURE 0x1702
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_2D_MULTISAMPLE 0x9100
#define GL_TEXTURE_3D 0x806F
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_CUBE_MAP_ARRAY 0x9009
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#define GL_TEXTURE_RECTANGLE 0x84F5
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_FAN 0x0006
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRUE 1
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_INT 0x1405
#define GL_UNSIGNED_INT_8_8_8_8 0x8035
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define GL_UNSIGNED_SHORT 0x1403
#define GL_VENDOR 0x1F00
#define GL_VERSION 0x1F02
#define GL_VERTEX_SHADER 0x8B31
#define GL_VERTEX_SHADER_ARB 0x8B31
#define GL_ZERO 0

#endif // LL_DARWIN

#include "volk.h"

#endif // LL_LLGLHEADERS_H
