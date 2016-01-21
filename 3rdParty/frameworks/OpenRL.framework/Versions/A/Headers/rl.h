/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright(c) 2006-2013 Caustic Graphics, Inc.    All rights reserved.        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef __rl_h__
#define __rl_h__

#include <OpenRL/rlplatform.h>


/*-------------------------------------------------------------------------
 * Data type definitions
 *-----------------------------------------------------------------------*/

typedef void                   RLvoid;
typedef RLint                  RLboolean;
typedef RLint                  RLenum;
typedef RLuint                 RLbitfield;
typedef float                  RLfloat;
typedef double                 RLdouble;
typedef size_t                 RLsize;

typedef RLboolean              RLbvec2 [2];
typedef RLboolean              RLbvec3 [3];
typedef RLboolean              RLbvec4 [4];

typedef RLint                  RLivec2 [2];
typedef RLint                  RLivec3 [3];
typedef RLint                  RLivec4 [4];

typedef RLfloat                RLvec2  [2];
typedef RLfloat                RLvec3  [3];
typedef RLfloat                RLvec4  [4];

typedef RLfloat                RLmat2  [4];
typedef RLfloat                RLmat3  [9];
typedef RLfloat                RLmat4  [16];

enum RLprimitive {
	RL_NULL_PRIMITIVE          = 0,
	RL_MAX_PRIMITIVE_HANDLE    = ~(RLint)0
};

typedef struct _RLbuffer*      RLbuffer;
typedef enum RLprimitive       RLprimitive;
typedef struct _RLtexture*     RLtexture;
typedef struct _RLframebuffer* RLframebuffer;
typedef struct _RLshader*      RLshader;
typedef struct _RLprogram*     RLprogram;


/*-------------------------------------------------------------------------
 * Constant definitions
 *-----------------------------------------------------------------------*/

/* Version */
#define RL_VERSION_1_0                                1
#define RL_VERSION_1_1                                1
#define RL_VERSION_1_2                                1
#define RL_VERSION_1_3                                1
#define RL_VERSION_1_4                                1

/* Null handles */
#define RL_NULL_BUFFER                                ((RLbuffer)0)
#define RL_NULL_TEXTURE                               ((RLtexture)0)
#define RL_NULL_FRAMEBUFFER                           ((RLframebuffer)0)
#define RL_NULL_SHADER                                ((RLshader)0)
#define RL_NULL_PROGRAM                               ((RLprogram)0)

/* Clear Buffer Mask */
#define RL_COLOR_BUFFER_BIT                           0x00004000
#define RL_DRAW_BUFFER0_BIT                           0x8000
#define RL_DRAW_BUFFER1_BIT                           (RL_DRAW_BUFFER0_BIT<<1)
#define RL_DRAW_BUFFER2_BIT                           (RL_DRAW_BUFFER0_BIT<<2)
#define RL_DRAW_BUFFER3_BIT                           (RL_DRAW_BUFFER0_BIT<<3)
#define RL_DRAW_BUFFER4_BIT                           (RL_DRAW_BUFFER0_BIT<<4)
#define RL_DRAW_BUFFER5_BIT                           (RL_DRAW_BUFFER0_BIT<<5)
#define RL_DRAW_BUFFER6_BIT                           (RL_DRAW_BUFFER0_BIT<<6)
#define RL_DRAW_BUFFER7_BIT                           (RL_DRAW_BUFFER0_BIT<<7)
#define RL_DRAW_BUFFER8_BIT                           (RL_DRAW_BUFFER0_BIT<<8)
#define RL_DRAW_BUFFER9_BIT                           (RL_DRAW_BUFFER0_BIT<<9)
#define RL_DRAW_BUFFER10_BIT                          (RL_DRAW_BUFFER0_BIT<<10)
#define RL_DRAW_BUFFER11_BIT                          (RL_DRAW_BUFFER0_BIT<<11)
#define RL_DRAW_BUFFER12_BIT                          (RL_DRAW_BUFFER0_BIT<<12)
#define RL_DRAW_BUFFER13_BIT                          (RL_DRAW_BUFFER0_BIT<<13)
#define RL_DRAW_BUFFER14_BIT                          (RL_DRAW_BUFFER0_BIT<<14)
#define RL_DRAW_BUFFER15_BIT                          (RL_DRAW_BUFFER0_BIT<<15)

/* Boolean */
#define RL_FALSE                                      0
#define RL_TRUE                                       1

/* Begin Mode */
#define RL_TRIANGLES                                  0x0004
#define RL_TRIANGLE_STRIP                             0x0005

/* Buffer Objects */
#define RL_ARRAY_BUFFER                               0x8892
#define RL_ELEMENT_ARRAY_BUFFER                       0x8893
#define RL_ARRAY_BUFFER_BINDING                       0x8894
#define RL_ELEMENT_ARRAY_BUFFER_BINDING               0x8895
#define RL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING         0x889F
#define RL_VERTEX_ATTRIB_ELEMENT_ARRAY_BUFFER_BINDING 0x88A0
#define RL_PRIMITIVE_ELEMENT_ARRAY_BUFFER_BINDING     0x88A1

#define RL_STREAM_DRAW                                0x88E0
#define RL_STREAM_READ                                0x88E1
#define RL_STREAM_COPY                                0x88E2
#define RL_STATIC_DRAW                                0x88E4
#define RL_STATIC_READ                                0x88E5
#define RL_STATIC_COPY                                0x88E6
#define RL_DYNAMIC_DRAW                               0x88E8
#define RL_DYNAMIC_READ                               0x88E9
#define RL_DYNAMIC_COPY                               0x88EA

#define RL_BUFFER_SIZE                                0x8764
#define RL_BUFFER_USAGE                               0x8765
#define RL_BUFFER_ACCESS                              0x88BB
#define RL_BUFFER_MAPPED                              0x88BC

#define RL_CURRENT_VERTEX_ATTRIB                      0x8626

/* Enable Cap */
#define RL_TEXTURE_2D                                 0x0DE1

/* Error Code */
#define RL_NO_ERROR                                   0
#define RL_INVALID_ENUM                               0x0500
#define RL_INVALID_VALUE                              0x0501
#define RL_INVALID_OPERATION                          0x0502
#define RL_OUT_OF_MEMORY                              0x0505
#define RL_INVALID_FRAMEBUFFER_OPERATION              0x0506
#define RL_UNIMPLEMENTED                              0x0507
#define RL_INTERNAL_ERROR                             0x0508

/* Front Face Direction */
#define RL_CW                                         0x0900
#define RL_CCW                                        0x0901

/* Get PName */
#define RL_VIEWPORT                                   0x0BA2
#define RL_TEXTURE_BINDING_2D                         0x8069

/* Hint Mode */
#define RL_DONT_CARE                                  0x1100
#define RL_FASTEST                                    0x1101
#define RL_NICEST                                     0x1102

/* Data Type */
#define RL_BYTE                                       0x1400
#define RL_UNSIGNED_BYTE                              0x1401
#define RL_SHORT                                      0x1402
#define RL_UNSIGNED_SHORT                             0x1403
#define RL_INT                                        0x1404
#define RL_UNSIGNED_INT                               0x1405
#define RL_FLOAT                                      0x1406

/* Pixel Format */
#define RL_RGB                                        0x1907
#define RL_RGBA                                       0x1908
#define RL_LUMINANCE                                  0x1909

/* Shaders */
#define RL_VERTEX_SHADER                              0x8B31
#define RL_RAY_SHADER                                 0x10200
#define RL_FRAME_SHADER                               0x10201
#define RL_PREFIX_RAY_SHADER						  0x10203
#define RL_MAX_VERTEX_ATTRIBS                         0x8869
#define RL_MAX_VERTEX_UNIFORM_VECTORS                 0x8DFB
#define RL_MAX_VARYING_VECTORS                        0x8DFC
#define RL_MAX_GLOBAL_VECTORS                         0x8DFD
#define RL_DELETE_STATUS                              0x8B80
#define RL_SHADER_TYPE                                0x8B4F
#define RL_LINK_STATUS                                0x8B82
#define RL_ATTACHED_SHADERS                           0x8B85
#define RL_ACTIVE_UNIFORMS                            0x8B86
#define RL_ACTIVE_UNIFORM_MAX_LENGTH                  0x8B87
#define RL_ACTIVE_ATTRIBUTES                          0x8B89
#define RL_ACTIVE_ATTRIBUTE_MAX_LENGTH                0x8B8A
#define RL_SHADING_LANGUAGE_VERSION                   0x8B8C
#define RL_CURRENT_PROGRAM                            0x8B8D

/* String Name */
#define RL_VENDOR                                     0x1F00
#define RL_RENDERER                                   0x1F01
#define RL_VERSION                                    0x1F02
#define RL_EXTENSIONS                                 0x1F03

/* Texture Mag Filter */
#define RL_NEAREST                                    0x2600
#define RL_LINEAR                                     0x2601

/* Texture Min Filter */
#define RL_NEAREST_MIPMAP_NEAREST                     0x2700
#define RL_LINEAR_MIPMAP_LINEAR                       0x2703

/* Texture Parameter Name */
#define RL_TEXTURE_MAG_FILTER                         0x2800
#define RL_TEXTURE_MIN_FILTER                         0x2801
#define RL_TEXTURE_WRAP_S                             0x2802
#define RL_TEXTURE_WRAP_T                             0x2803

#define RL_TEXTURE_WIDTH                              0x1000
#define RL_TEXTURE_HEIGHT                             0x1001
#define RL_TEXTURE_DEPTH                              0x1002
#define RL_TEXTURE_INTERNAL_FORMAT                    0x1003

#define RL_MAX_TEXTURE_SIZE                           0x0D33

/* Texture Wrap Mode */
#define RL_REPEAT                                     0x2901
#define RL_CLAMP_TO_EDGE                              0x812F

/* Uniform Types */
#define RL_FLOAT_VEC2                                 0x8B50
#define RL_FLOAT_VEC3                                 0x8B51
#define RL_FLOAT_VEC4                                 0x8B52
#define RL_INT_VEC2                                   0x8B53
#define RL_INT_VEC3                                   0x8B54
#define RL_INT_VEC4                                   0x8B55
#define RL_BOOL                                       0x8B56
#define RL_BOOL_VEC2                                  0x8B57
#define RL_BOOL_VEC3                                  0x8B58
#define RL_BOOL_VEC4                                  0x8B59
#define RL_FLOAT_MAT2                                 0x8B5A
#define RL_FLOAT_MAT3                                 0x8B5B
#define RL_FLOAT_MAT4                                 0x8B5C
#define RL_SAMPLER_2D                                 0x8B5E

/* Vertex Arrays */
#define RL_VERTEX_ATTRIB_ARRAY_SIZE                   0x8623
#define RL_VERTEX_ATTRIB_ARRAY_STRIDE                 0x8624
#define RL_VERTEX_ATTRIB_ARRAY_TYPE                   0x8625
#define RL_VERTEX_ATTRIB_ARRAY_NORMALIZED             0x886A
#define RL_VERTEX_ATTRIB_ARRAY_POINTER                0x8645

/* Shader Source */
#define RL_COMPILE_STATUS                             0x8B81
#define RL_SHADER_SOURCE_LENGTH                       0x8B88

/* Shader Binary */
#define RL_PLATFORM_BINARY                            0x8D63

/* Framebuffer Object. */
#define RL_FRAMEBUFFER                                0x8D40

#define RL_COLOR_ATTACHMENT0                          0x8CE0
#define RL_COLOR_ATTACHMENT1                          0x8CE1
#define RL_COLOR_ATTACHMENT2                          0x8CE2
#define RL_COLOR_ATTACHMENT3                          0x8CE3
#define RL_COLOR_ATTACHMENT4                          0x8CE4
#define RL_COLOR_ATTACHMENT5                          0x8CE5
#define RL_COLOR_ATTACHMENT6                          0x8CE6
#define RL_COLOR_ATTACHMENT7                          0x8CE7
#define RL_COLOR_ATTACHMENT8                          0x8CE8
#define RL_COLOR_ATTACHMENT9                          0x8CE9
#define RL_COLOR_ATTACHMENT10                         0x8CEA
#define RL_COLOR_ATTACHMENT11                         0x8CEB
#define RL_COLOR_ATTACHMENT12                         0x8CEC
#define RL_COLOR_ATTACHMENT13                         0x8CED
#define RL_COLOR_ATTACHMENT14                         0x8CEE
#define RL_COLOR_ATTACHMENT15                         0x8CEF

#define RL_MAX_COLOR_ATTACHMENTS                      0x8CDF
#define RL_MAX_VIEWPORT_DIMS                          0x0D3A

#define RL_FRAMEBUFFER_COMPLETE                       0x8CD5
#define RL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT          0x8CD6
#define RL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT  0x8CD7
#define RL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS          0x8CD9
#define RL_FRAMEBUFFER_INCOMPLETE_FORMATS             0x8CDA

#define RL_FRAMEBUFFER_BINDING                        0x8CA6

/* Map Buffer */
#define RL_READ_ONLY                                  0x88B8
#define RL_WRITE_ONLY                                 0x88B9
#define RL_READ_WRITE                                 0x88BA

/* Texture 3D */
#define RL_TEXTURE_WRAP_R                             0x8072
#define RL_TEXTURE_3D                                 0x806F
#define RL_TEXTURE_BINDING_3D                         0x806A
#define RL_SAMPLER_3D                                 0x8B5F

/* Primitive Objects */
#define RL_PRIMITIVE                                  0x10000
#define RL_PRIMITIVE_BINDING                          0x10001

#define RL_PRIMITIVE_COMPLETE                         0x18CD5
#define RL_PRIMITIVE_INCOMPLETE_PROGRAM               0x18CD6
#define RL_PRIMITIVE_INCOMPLETE_MISSING_PROGRAM       0x18CD7

#define RL_PRIMITIVE_IS_VISIBLE                       0x10100
#define RL_PRIMITIVE_ANIMATION_HINT                   0x10101
#define RL_PRIMITIVE_IS_OCCLUDER                      0x10102
#define RL_PRIMITIVE_ELEMENTS                         0x10103
#define RL_PRIMITIVE_MODE                             0x10104
#define RL_PRIMITIVE_MAX_ELEMENTS                     0x10105

#define RL_PRIMITIVE_STATIC                           0x10106
#define RL_PRIMITIVE_DYNAMIC_TRANSFORM                0x10107
#define RL_PRIMITIVE_DYNAMIC_VERTICES                 0x10108

#define RL_PRIMITIVE_TRANSFORM_MATRIX                 0x10110

#define RL_PRIMITIVE_NAME                             0x10111
#define RL_TEXTURE_NAME                               0x10112
#define RL_BUFFER_NAME                                0x10113
#define RL_FRAMEBUFFER_NAME                           0x10114
#define RL_SHADER_NAME                                0x10115
#define RL_PROGRAM_NAME                               0x10116

#define RL_COMPILE_LOG                                0x10117
#define RL_LINK_LOG                                   0x10118
#define RL_SHADER_SOURCE                              0x10119

/* Uniform blocks */
#define RL_MAX_UNIFORM_BLOCKS                         0x10120
#define RL_ACTIVE_UNIFORM_BLOCKS                      0x10121
#define RL_UNIFORM_BLOCK_BUFFER                       0x10122
#define RL_UNIFORM_BLOCK_BUFFER_BINDING               0x10123

/* Pixel buffer objects */
#define RL_PIXEL_PACK_BUFFER                          0x88EB
#define RL_PIXEL_UNPACK_BUFFER                        0x88EC
#define RL_PIXEL_PACK_BUFFER_BINDING                  0x88ED
#define RL_PIXEL_UNPACK_BUFFER_BINDING                0x88EF

/* rlGetBooleanv parameters */
#define RL_IS_HARDWARE_ACCELERATED                    0x8000

/* Global parameters */
#define RL_MAX_OUTPUT_RAY_COUNT                       0x10300
#define RL_MAX_RAY_DEPTH_LIMIT                        0x10302
#define RL_MAX_RAY_CLASSES                            0x10304

#define RL_MIN_FILTER_RADIUS                          0x10313
#define RL_MAX_FILTER_RADIUS                          0x10314
#define RL_MAX_FILTER_TABLE_WIDTH                     0x10315

/* Stats parameters */
#define RL_TOTAL_FRAME_TIME                           0x10305
#define RL_PREPARE_FRAME_TIME                         0x10306
#define RL_RENDER_FRAME_TIME                          0x10307
#define RL_TRIANGLE_COUNT                             0x10308
#define RL_PRIMITIVE_COUNT                            0x10309
#define RL_EMITTED_RAY_COUNT                          0x1030A
#define RL_UNUSED_RAY_COUNT                           0x1030B
#define RL_OUTPUT_RAY_USAGE                           0x1030C
#define RL_REPORT                                     0x1030D
#define RL_PROFILE                                    0x1030E

/* Frame parameters */
#define RL_FRAME_RAY_DEPTH_LIMIT                      0x10303
#define RL_FRAME_FILTER_RADIUS                        0x10310
#define RL_FRAME_FILTER_TABLE_WIDTH                   0x10311
#define RL_FRAME_FILTER_TABLE                         0x10312

/* Framebuffer attachment parameters */
#define RL_FRAMEBUFFER_ATTACHMENT_FILTER_ENABLED      0x10320
#define RL_FRAMEBUFFER_ATTACHMENT_BLEND_MODE          0x10321

#define RL_BLEND_ADD                                  0x10322
#define RL_BLEND_MULTIPLY                             0x10323
#define RL_BLEND_MIN                                  0x10324
#define RL_BLEND_MAX                                  0x10325

/* Pixel store parameters */
#define RL_UNPACK_CLIENT_STORAGE                      0x10330

/*-------------------------------------------------------------------------
 * RL core functions.
 *-----------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

RL_APICALL void        RL_APIENTRY rlAttachShader (RLprogram program, RLshader shader);
RL_APICALL void        RL_APIENTRY rlBindBuffer (RLenum target, RLbuffer buffer);
RL_APICALL void        RL_APIENTRY rlBindFramebuffer (RLenum target, RLframebuffer framebuffer);
RL_APICALL void        RL_APIENTRY rlBindPrimitive (RLenum target, RLprimitive primitive);
RL_APICALL void        RL_APIENTRY rlBindTexture (RLenum target, RLtexture texture);
RL_APICALL void        RL_APIENTRY rlBufferData (RLenum target, RLsize size, const void* data, RLenum usage);
RL_APICALL void        RL_APIENTRY rlBufferSubData (RLenum target, RLsize offset, RLsize size, const void* data);
RL_APICALL void        RL_APIENTRY rlBufferParameterString (RLenum target, RLenum pname, const char * param);
RL_APICALL RLenum      RL_APIENTRY rlCheckFramebufferStatus (RLenum target);
RL_APICALL RLenum      RL_APIENTRY rlCheckPrimitiveStatus (RLenum target);
RL_APICALL void        RL_APIENTRY rlClear (RLbitfield mask);
RL_APICALL void        RL_APIENTRY rlClearColor (RLfloat red, RLfloat green, RLfloat blue, RLfloat alpha);
RL_APICALL void        RL_APIENTRY rlCompileShader (RLshader shader);
RL_APICALL RLprogram   RL_APIENTRY rlCreateProgram (void);
RL_APICALL RLshader    RL_APIENTRY rlCreateShader (RLenum type);
RL_APICALL void        RL_APIENTRY rlDeleteBuffers (RLsize n, const RLbuffer* buffers);
RL_APICALL void        RL_APIENTRY rlDeleteFramebuffers (RLsize n, const RLframebuffer* framebuffers);
RL_APICALL void        RL_APIENTRY rlDeletePrimitives (RLsize n, const RLprimitive* primitives);
RL_APICALL void        RL_APIENTRY rlDeleteProgram (RLprogram program);
RL_APICALL void        RL_APIENTRY rlDeleteShader (RLshader shader);
RL_APICALL void        RL_APIENTRY rlDetachShader (RLprogram program, RLshader shader);
RL_APICALL void        RL_APIENTRY rlDeleteTextures (RLsize n, const RLtexture* textures);
RL_APICALL void        RL_APIENTRY rlDrawArrays (RLenum mode, RLsize first, RLsize count);
RL_APICALL void        RL_APIENTRY rlDrawElements (RLenum mode, RLsize count, RLenum type, RLsize offset);
RL_APICALL void        RL_APIENTRY rlDrawVertexAttribIndices (RLenum mode, RLsize count);
RL_APICALL void        RL_APIENTRY rlFramebufferAttachmentParameter1i (RLenum target, RLenum attachment, RLenum pname, RLint param);
RL_APICALL void        RL_APIENTRY rlFramebufferParameterString (RLenum target, RLenum pname, const char * param);
RL_APICALL void        RL_APIENTRY rlFramebufferTexture2D (RLenum target, RLenum attachment, RLenum textarget, RLtexture texture, RLint level);
RL_APICALL void        RL_APIENTRY rlFramebufferTexture3D (RLenum target, RLenum attachment, RLenum textarget, RLtexture texture, RLint level, RLint zoffset);
RL_APICALL void        RL_APIENTRY rlFrameParameter1i (RLenum pname, RLint value);
RL_APICALL void        RL_APIENTRY rlFrameParameter1f (RLenum pname, RLfloat param);
RL_APICALL void        RL_APIENTRY rlFrameParameterfv (RLenum pname, const RLfloat* param);
RL_APICALL void        RL_APIENTRY rlFrontFace (RLenum mode);
RL_APICALL void        RL_APIENTRY rlGenBuffers (RLsize n, RLbuffer* buffers);
RL_APICALL void        RL_APIENTRY rlGenerateMipmap (RLenum target);
RL_APICALL void        RL_APIENTRY rlGenFramebuffers (RLsize n, RLframebuffer* framebuffers);
RL_APICALL void        RL_APIENTRY rlGenPrimitives (RLsize n, RLprimitive* primitives);
RL_APICALL void        RL_APIENTRY rlGenTextures (RLsize n, RLtexture* textures);
RL_APICALL void        RL_APIENTRY rlGetActiveAttrib (RLprogram program, RLint index, RLsize bufsize, RLsize* length, RLint* size, RLenum* type, char* name);
RL_APICALL void        RL_APIENTRY rlGetActiveUniform (RLprogram program, RLint index, RLsize bufsize, RLsize* length, RLint* size, RLenum* type, char* name);
RL_APICALL void        RL_APIENTRY rlGetActiveUniformBlock (RLprogram program, RLint index, RLsize bufsize, RLsize* length, char* name, RLint* fieldCount, RLsize* blocksize);
RL_APICALL void        RL_APIENTRY rlGetActiveUniformBlockField (RLprogram program, RLint blockIndex, RLint fieldIndex, RLsize bufsize, RLsize* length, RLint* size, RLenum* type, char* name);
RL_APICALL void        RL_APIENTRY rlGetAttachedShaders (RLprogram program, RLint maxcount, RLint* count, RLshader* shaders);
RL_APICALL RLint       RL_APIENTRY rlGetAttribLocation (RLprogram program, const char* name);
RL_APICALL void        RL_APIENTRY rlGetBooleanv (RLenum pname, RLboolean* params);
RL_APICALL void        RL_APIENTRY rlGetBuffer (RLenum pname, RLbuffer* buffer);
RL_APICALL void        RL_APIENTRY rlGetBufferParameteriv (RLenum target, RLenum pname, RLint* params);
RL_APICALL void        RL_APIENTRY rlGetBufferParametersv (RLenum target, RLenum pname, RLsize* params);
RL_APICALL void        RL_APIENTRY rlGetBufferParameterString (RLenum target, RLenum pname, const char * * param);
RL_APICALL void        RL_APIENTRY rlGetDoublev (RLenum pname, RLdouble* params);
RL_APICALL RLenum      RL_APIENTRY rlGetError (void);
RL_APICALL void        RL_APIENTRY rlGetFloatv (RLenum pname, RLfloat* params);
RL_APICALL void        RL_APIENTRY rlGetFramebuffer (RLenum pname, RLframebuffer* framebuffer);
RL_APICALL void        RL_APIENTRY rlGetFramebufferAttachmentParameteriv (RLenum target, RLenum attachment, RLenum pname, RLint* param);
RL_APICALL void        RL_APIENTRY rlGetFramebufferParameterString (RLenum target, RLenum pname, const char * * param);
RL_APICALL void        RL_APIENTRY rlGetFrameParameteriv (RLenum pname, RLint* params);
RL_APICALL void        RL_APIENTRY rlGetFrameParameterfv (RLenum pname, RLfloat* params);
RL_APICALL void        RL_APIENTRY rlGetIntegerv (RLenum pname, RLint* params);
RL_APICALL void        RL_APIENTRY rlGetLongv (RLenum pname, RLlong* params);
RL_APICALL void        RL_APIENTRY rlGetSizev (RLenum pname, RLsize* params);
RL_APICALL void        RL_APIENTRY rlGetPrimitive (RLenum pname, RLprimitive* primitive);
RL_APICALL void        RL_APIENTRY rlGetPrimitiveParameter1i (RLenum target, RLenum pname, RLint* param);
RL_APICALL void        RL_APIENTRY rlGetPrimitiveParameter1s (RLenum target, RLenum pname, RLsize* param);
RL_APICALL void        RL_APIENTRY rlGetPrimitiveParameterMatrixf (RLenum target, RLenum pname, RLfloat* param);
RL_APICALL void        RL_APIENTRY rlGetPrimitiveParameterString (RLenum target, RLenum pname, const char * * param);
RL_APICALL void        RL_APIENTRY rlGetProgram (RLenum pname, RLprogram* program);
RL_APICALL void        RL_APIENTRY rlGetProgramiv (RLprogram program, RLenum pname, RLint* params);
RL_APICALL void        RL_APIENTRY rlGetProgramString (RLprogram program, RLenum pname, const char * * param);
RL_APICALL void        RL_APIENTRY rlGetShaderiv (RLshader shader, RLenum pname, RLint* params);
RL_APICALL void        RL_APIENTRY rlGetShaderString (RLshader shader, RLenum pname, const char * * param);
RL_APICALL const char* RL_APIENTRY rlGetString (RLenum name);
RL_APICALL void        RL_APIENTRY rlGetTexImage (RLenum target, RLint level, RLenum format, RLenum type, RLvoid* pixels);
RL_APICALL void        RL_APIENTRY rlGetTexLevelParameteriv (RLenum target, RLint level, RLenum pname, RLint* params);
RL_APICALL void        RL_APIENTRY rlGetTexParameteriv (RLenum target, RLenum pname, RLint* params);
RL_APICALL void        RL_APIENTRY rlGetTexParameterString (RLenum target, RLenum pname, const char * * param);
RL_APICALL void        RL_APIENTRY rlGetTexture (RLenum pname, RLtexture* texture);
RL_APICALL void        RL_APIENTRY rlGetUniformfv (RLprogram program, RLint location, RLfloat* params);
RL_APICALL void        RL_APIENTRY rlGetUniformiv (RLprogram program, RLint location, RLint* params);
RL_APICALL void        RL_APIENTRY rlGetUniformpv (RLprogram program, RLint location, RLprimitive* params);
RL_APICALL void        RL_APIENTRY rlGetUniformtv (RLprogram program, RLint location, RLtexture* params);
RL_APICALL RLint       RL_APIENTRY rlGetUniformBlockFieldOffset (RLprogram program, RLint blockIndex, const char* name);
RL_APICALL RLint       RL_APIENTRY rlGetUniformBlockIndex (RLprogram program, const char* name);
RL_APICALL RLint       RL_APIENTRY rlGetUniformLocation (RLprogram program, const char* name);
RL_APICALL void        RL_APIENTRY rlGetUniformBlockBuffer (RLprogram program, RLint location, RLbuffer* param);
RL_APICALL void        RL_APIENTRY rlGetVertexAttribbv (RLint index, RLenum pname, RLbuffer* params);
RL_APICALL void        RL_APIENTRY rlGetVertexAttribfv (RLint index, RLenum pname, RLfloat* params);
RL_APICALL void        RL_APIENTRY rlGetVertexAttribiv (RLint index, RLenum pname, RLint* params);
RL_APICALL void        RL_APIENTRY rlGetVertexAttribsv (RLint index, RLenum pname, RLsize* params);
RL_APICALL RLboolean   RL_APIENTRY rlIsBuffer (RLbuffer buffer);
RL_APICALL RLboolean   RL_APIENTRY rlIsFramebuffer (RLframebuffer framebuffer);
RL_APICALL RLboolean   RL_APIENTRY rlIsPrimitive (RLprimitive primitive);
RL_APICALL RLboolean   RL_APIENTRY rlIsProgram (RLprogram program);
RL_APICALL RLboolean   RL_APIENTRY rlIsShader (RLshader shader);
RL_APICALL RLboolean   RL_APIENTRY rlIsTexture (RLtexture texture);
RL_APICALL void        RL_APIENTRY rlLinkProgram (RLprogram program);
RL_APICALL void*       RL_APIENTRY rlMapBuffer (RLenum target, RLenum access);
RL_APICALL void        RL_APIENTRY rlPixelStorei(RLenum pname, RLint param);
RL_APICALL void        RL_APIENTRY rlPrimitiveParameter1i (RLenum target, RLenum pname, RLint param);
RL_APICALL void        RL_APIENTRY rlPrimitiveParameterMatrixf (RLenum target, RLenum pname, RLfloat* param);
RL_APICALL void        RL_APIENTRY rlPrimitiveParameterString (RLenum target, RLenum pname, const char * param);
RL_APICALL void        RL_APIENTRY rlProgramString (RLprogram program, RLenum pname, const char * param);
RL_APICALL void        RL_APIENTRY rlRenderFrame (void);
RL_APICALL void        RL_APIENTRY rlShaderSource (RLshader shader, RLsize count, const char* const* string, const RLsize* length);
RL_APICALL void        RL_APIENTRY rlShaderString (RLshader shader, RLenum pname, const char * param);
RL_APICALL void        RL_APIENTRY rlTexImage2D (RLenum target, RLint level, RLenum internalformat, RLint width, RLint height, RLint border, RLenum format, RLenum type, const void* pixels);
RL_APICALL void        RL_APIENTRY rlTexImage3D (RLenum target, RLint level, RLenum internalformat, RLint width, RLint height, RLint depth, RLint border, RLenum format, RLenum type, const void* pixels);
RL_APICALL void        RL_APIENTRY rlTexParameteri (RLenum target, RLenum pname, RLint param);
RL_APICALL void        RL_APIENTRY rlTexParameteriv (RLenum target, RLenum pname, const RLint* params);
RL_APICALL void        RL_APIENTRY rlTexParameterString (RLenum target, RLenum pname, const char * param);
RL_APICALL void        RL_APIENTRY rlUniform1f (RLint location, RLfloat x);
RL_APICALL void        RL_APIENTRY rlUniform1fv (RLint location, RLsize count, const RLfloat* v);
RL_APICALL void        RL_APIENTRY rlUniform1i (RLint location, RLint x);
RL_APICALL void        RL_APIENTRY rlUniform1iv (RLint location, RLsize count, const RLint* v);
RL_APICALL void        RL_APIENTRY rlUniform2f (RLint location, RLfloat x, RLfloat y);
RL_APICALL void        RL_APIENTRY rlUniform2fv (RLint location, RLsize count, const RLfloat* v);
RL_APICALL void        RL_APIENTRY rlUniform2i (RLint location, RLint x, RLint y);
RL_APICALL void        RL_APIENTRY rlUniform2iv (RLint location, RLsize count, const RLint* v);
RL_APICALL void        RL_APIENTRY rlUniform3f (RLint location, RLfloat x, RLfloat y, RLfloat z);
RL_APICALL void        RL_APIENTRY rlUniform3fv (RLint location, RLsize count, const RLfloat* v);
RL_APICALL void        RL_APIENTRY rlUniform3i (RLint location, RLint x, RLint y, RLint z);
RL_APICALL void        RL_APIENTRY rlUniform3iv (RLint location, RLsize count, const RLint* v);
RL_APICALL void        RL_APIENTRY rlUniform4f (RLint location, RLfloat x, RLfloat y, RLfloat z, RLfloat w);
RL_APICALL void        RL_APIENTRY rlUniform4fv (RLint location, RLsize count, const RLfloat* v);
RL_APICALL void        RL_APIENTRY rlUniform4i (RLint location, RLint x, RLint y, RLint z, RLint w);
RL_APICALL void        RL_APIENTRY rlUniform4iv (RLint location, RLsize count, const RLint* v);
RL_APICALL void        RL_APIENTRY rlUniformp (RLint location, RLprimitive p);
RL_APICALL void        RL_APIENTRY rlUniformpv (RLint location, RLsize count, const RLprimitive* v);
RL_APICALL void        RL_APIENTRY rlUniformt (RLint location, RLtexture t);
RL_APICALL void        RL_APIENTRY rlUniformtv (RLint location, RLsize count, const RLtexture* v);
RL_APICALL void        RL_APIENTRY rlUniformMatrix2fv (RLint location, RLsize count, RLboolean transpose, const RLfloat* value);
RL_APICALL void        RL_APIENTRY rlUniformMatrix3fv (RLint location, RLsize count, RLboolean transpose, const RLfloat* value);
RL_APICALL void        RL_APIENTRY rlUniformMatrix4fv (RLint location, RLsize count, RLboolean transpose, const RLfloat* value);
RL_APICALL void        RL_APIENTRY rlUniformBlockBuffer (RLint index, RLbuffer buffer);
RL_APICALL RLboolean   RL_APIENTRY rlUnmapBuffer (RLenum target);
RL_APICALL void        RL_APIENTRY rlUseProgram (RLprogram program);
RL_APICALL void        RL_APIENTRY rlVertexAttrib1f (RLint indx, RLfloat x);
RL_APICALL void        RL_APIENTRY rlVertexAttrib1fv (RLint indx, const RLfloat* values);
RL_APICALL void        RL_APIENTRY rlVertexAttrib2f (RLint indx, RLfloat x, RLfloat y);
RL_APICALL void        RL_APIENTRY rlVertexAttrib2fv (RLint indx, const RLfloat* values);
RL_APICALL void        RL_APIENTRY rlVertexAttrib3f (RLint indx, RLfloat x, RLfloat y, RLfloat z);
RL_APICALL void        RL_APIENTRY rlVertexAttrib3fv (RLint indx, const RLfloat* values);
RL_APICALL void        RL_APIENTRY rlVertexAttrib4f (RLint indx, RLfloat x, RLfloat y, RLfloat z, RLfloat w);
RL_APICALL void        RL_APIENTRY rlVertexAttrib4fv (RLint indx, const RLfloat* values);
RL_APICALL void        RL_APIENTRY rlVertexAttribBuffer (RLint indx, RLint size, RLenum type, RLboolean normalized, RLsize stride, RLsize offset);
RL_APICALL void        RL_APIENTRY rlVertexAttribIndicesBuffer (RLint attrib, RLenum type, RLsize stride, RLsize offset);
RL_APICALL void        RL_APIENTRY rlViewport (RLint x, RLint y, RLint width, RLint height);

#ifdef __cplusplus
}
#endif


#endif /* __rl_h__ */
