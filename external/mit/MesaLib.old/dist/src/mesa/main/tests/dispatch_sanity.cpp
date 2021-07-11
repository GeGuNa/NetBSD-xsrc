/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \name dispatch_sanity.cpp
 *
 * Verify that only set of functions that should be available in a particular
 * API are available in that API.
 *
 * The list of expected functions originally came from the functions set by
 * api_exec_es2.c.  This file no longer exists in Mesa (but api_exec_es1.c was
 * still generated at the time this test was written).  It was the generated
 * file that configured the dispatch table for ES2 contexts.  This test
 * verifies that all of the functions set by the old api_exec_es2.c (with the
 * recent addition of VAO functions) are set in the dispatch table and
 * everything else is a NOP.
 *
 * When adding extensions that add new functions, this test will need to be
 * modified to expect dispatch functions for the new extension functions.
 */

#include <gtest/gtest.h>

#include "GL/gl.h"
#include "GL/glext.h"
#include "main/compiler.h"
#include "main/api_exec.h"
#include "main/context.h"
#include "main/remap.h"
#include "main/vtxfmt.h"
#include "glapi/glapi.h"
#include "drivers/common/driverfuncs.h"

#include "swrast/swrast.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "swrast_setup/swrast_setup.h"

#ifndef GLAPIENTRYP
#define GLAPIENTRYP GL_APIENTRYP
#endif

#include "main/dispatch.h"

struct function {
   const char *name;
   unsigned int Version;
   int offset;
};

extern const struct function common_desktop_functions_possible[];
extern const struct function gl_compatibility_functions_possible[];
extern const struct function gl_core_functions_possible[];
extern const struct function gles11_functions_possible[];
extern const struct function gles2_functions_possible[];
extern const struct function gles3_functions_possible[];
extern const struct function gles31_functions_possible[];

class DispatchSanity_test : public ::testing::Test {
public:
   virtual void SetUp();
   void SetUpCtx(gl_api api, unsigned int version);

   struct gl_config visual;
   struct dd_function_table driver_functions;
   struct gl_context share_list;
   struct gl_context ctx;
   _glapi_proc *nop_table;
};

void
DispatchSanity_test::SetUp()
{
   memset(&visual, 0, sizeof(visual));
   memset(&driver_functions, 0, sizeof(driver_functions));
   memset(&share_list, 0, sizeof(share_list));
   memset(&ctx, 0, sizeof(ctx));

   _mesa_init_driver_functions(&driver_functions);

   const unsigned size = _glapi_get_dispatch_table_size();
   nop_table = (_glapi_proc *) _mesa_new_nop_table(size);
}

void
DispatchSanity_test::SetUpCtx(gl_api api, unsigned int version)
{
   _mesa_initialize_context(&ctx,
                            api,
                            &visual,
                            NULL, // share_list
                            &driver_functions);
   _vbo_CreateContext(&ctx);

   _mesa_override_extensions(&ctx);
   ctx.Version = version;

   _mesa_initialize_dispatch_tables(&ctx);
   _mesa_initialize_vbo_vtxfmt(&ctx);
}

static const char *
offset_to_proc_name_safe(unsigned offset)
{
   const char *name = _glapi_get_proc_name(offset);
   return name ? name : "???";
}

/* Scan through the dispatch table and check that all the functions in
 * _glapi_proc *table exist.
 */
static void
validate_functions(struct gl_context *ctx, const struct function *function_table,
                   const _glapi_proc *nop_table)
{
   _glapi_proc *table = (_glapi_proc *) ctx->Exec;

   for (unsigned i = 0; function_table[i].name != NULL; i++) {
      /* The context version is >= the GL version where the function was
       * introduced. Therefore, the function cannot be set to the nop
       * function.
       */
      const bool cant_be_nop = ctx->Version >= function_table[i].Version;

      const int offset = (function_table[i].offset != -1)
         ? function_table[i].offset
         : _glapi_get_proc_offset(function_table[i].name);

      ASSERT_NE(-1, offset)
         << "Function: " << function_table[i].name;
      ASSERT_EQ(offset,
                _glapi_get_proc_offset(function_table[i].name))
         << "Function: " << function_table[i].name;
      if (cant_be_nop) {
         EXPECT_NE(nop_table[offset], table[offset])
            << "Function: " << function_table[i].name
            << " at offset " << offset;
      }

      table[offset] = nop_table[offset];
   }
}

/* Scan through the table and ensure that there is nothing except
 * nop functions (as set by validate_functions().
 */
static void
validate_nops(struct gl_context *ctx, const _glapi_proc *nop_table)
{
   _glapi_proc *table = (_glapi_proc *) ctx->Exec;

   const unsigned size = _glapi_get_dispatch_table_size();
   for (unsigned i = 0; i < size; i++) {
      EXPECT_EQ(nop_table[i], table[i])
         << "i = " << i << " (" << offset_to_proc_name_safe(i) << ")";
   }
}

TEST_F(DispatchSanity_test, GL31_CORE)
{
   SetUpCtx(API_OPENGL_CORE, 31);
   validate_functions(&ctx, common_desktop_functions_possible, nop_table);
   validate_functions(&ctx, gl_core_functions_possible, nop_table);
   validate_nops(&ctx, nop_table);
}

TEST_F(DispatchSanity_test, GL30)
{
   SetUpCtx(API_OPENGL_COMPAT, 30);
   validate_functions(&ctx, common_desktop_functions_possible, nop_table);
   validate_functions(&ctx, gl_compatibility_functions_possible, nop_table);
   validate_nops(&ctx, nop_table);
}

TEST_F(DispatchSanity_test, GLES11)
{
   SetUpCtx(API_OPENGLES, 11);
   validate_functions(&ctx, gles11_functions_possible, nop_table);
   validate_nops(&ctx, nop_table);
}

TEST_F(DispatchSanity_test, GLES2)
{
   SetUpCtx(API_OPENGLES2, 20);
   validate_functions(&ctx, gles2_functions_possible, nop_table);
   validate_nops(&ctx, nop_table);
}

TEST_F(DispatchSanity_test, GLES3)
{
   SetUpCtx(API_OPENGLES2, 30);
   validate_functions(&ctx, gles2_functions_possible, nop_table);
   validate_functions(&ctx, gles3_functions_possible, nop_table);
   validate_nops(&ctx, nop_table);
}

TEST_F(DispatchSanity_test, GLES31)
{
   SetUpCtx(API_OPENGLES2, 31);
   validate_functions(&ctx, gles2_functions_possible, nop_table);
   validate_functions(&ctx, gles3_functions_possible, nop_table);
   validate_functions(&ctx, gles31_functions_possible, nop_table);
   validate_nops(&ctx, nop_table);
}

const struct function common_desktop_functions_possible[] = {
   { "glBindRenderbufferEXT", 10, -1 },
   { "glBindFramebufferEXT", 10, -1 },
   { "glCullFace", 10, -1 },
   { "glFrontFace", 10, -1 },
   { "glHint", 10, -1 },
   { "glLineWidth", 10, -1 },
   { "glPointSize", 10, -1 },
   { "glPolygonMode", 10, -1 },
   { "glScissor", 10, -1 },
   { "glTexParameterf", 10, -1 },
   { "glTexParameterfv", 10, -1 },
   { "glTexParameteri", 10, -1 },
   { "glTexParameteriv", 10, -1 },
   { "glTexImage1D", 10, _gloffset_TexImage1D },
   { "glTexImage2D", 10, _gloffset_TexImage2D },
   { "glDrawBuffer", 10, -1 },
   { "glClear", 10, -1 },
   { "glClearColor", 10, -1 },
   { "glClearStencil", 10, -1 },
   { "glClearDepth", 10, -1 },
   { "glStencilMask", 10, -1 },
   { "glColorMask", 10, -1 },
   { "glDepthMask", 10, -1 },
   { "glDisable", 10, -1 },
   { "glEnable", 10, -1 },
   { "glFinish", 10, -1 },
   { "glFlush", 10, -1 },
   { "glBlendFunc", 10, -1 },
   { "glLogicOp", 10, -1 },
   { "glStencilFunc", 10, -1 },
   { "glStencilOp", 10, -1 },
   { "glDepthFunc", 10, -1 },
   { "glPixelStoref", 10, -1 },
   { "glPixelStorei", 10, -1 },
   { "glReadBuffer", 10, -1 },
   { "glReadPixels", 10, -1 },
   { "glGetBooleanv", 10, -1 },
   { "glGetDoublev", 10, -1 },
   { "glGetError", 10, -1 },
   { "glGetFloatv", 10, -1 },
   { "glGetIntegerv", 10, -1 },
   { "glGetString", 10, -1 },
   { "glGetTexImage", 10, -1 },
   { "glGetTexParameterfv", 10, -1 },
   { "glGetTexParameteriv", 10, -1 },
   { "glGetTexLevelParameterfv", 10, -1 },
   { "glGetTexLevelParameteriv", 10, -1 },
   { "glIsEnabled", 10, -1 },
   { "glDepthRange", 10, -1 },
   { "glViewport", 10, -1 },

   /* GL 1.1 */
   { "glDrawArrays", 11, -1 },
   { "glDrawElements", 11, -1 },
   { "glGetPointerv", 11, -1 },
   { "glPolygonOffset", 11, -1 },
   { "glCopyTexImage1D", 11, -1 },
   { "glCopyTexImage2D", 11, -1 },
   { "glCopyTexSubImage1D", 11, -1 },
   { "glCopyTexSubImage2D", 11, -1 },
   { "glTexSubImage1D", 11, -1 },
   { "glTexSubImage2D", 11, -1 },
   { "glBindTexture", 11, -1 },
   { "glDeleteTextures", 11, -1 },
   { "glGenTextures", 11, -1 },
   { "glIsTexture", 11, -1 },

   /* GL 1.2 */
   { "glBlendColor", 12, -1 },
   { "glBlendEquation", 12, -1 },
   { "glDrawRangeElements", 12, -1 },
   { "glTexImage3D", 12, -1 },
   { "glTexSubImage3D", 12, -1 },
   { "glCopyTexSubImage3D", 12, -1 },

   /* GL 1.3 */
   { "glActiveTexture", 13, -1 },
   { "glSampleCoverage", 13, -1 },
   { "glCompressedTexImage3D", 13, -1 },
   { "glCompressedTexImage2D", 13, -1 },
   { "glCompressedTexImage1D", 13, -1 },
   { "glCompressedTexSubImage3D", 13, -1 },
   { "glCompressedTexSubImage2D", 13, -1 },
   { "glCompressedTexSubImage1D", 13, -1 },
   { "glGetCompressedTexImage", 13, -1 },

   /* GL 1.4 */
   { "glBlendFuncSeparate", 14, -1 },
   { "glMultiDrawArrays", 14, -1 },
   { "glMultiDrawElements", 14, -1 },
   { "glPointParameterf", 14, -1 },
   { "glPointParameterfv", 14, -1 },
   { "glPointParameteri", 14, -1 },
   { "glPointParameteriv", 14, -1 },

   /* GL 1.5 */
   { "glGenQueries", 15, -1 },
   { "glDeleteQueries", 15, -1 },
   { "glIsQuery", 15, -1 },
   { "glBeginQuery", 15, -1 },
   { "glEndQuery", 15, -1 },
   { "glGetQueryiv", 15, -1 },
   { "glGetQueryObjectiv", 15, -1 },
   { "glGetQueryObjectuiv", 15, -1 },
   { "glBindBuffer", 15, -1 },
   { "glDeleteBuffers", 15, -1 },
   { "glGenBuffers", 15, -1 },
   { "glIsBuffer", 15, -1 },
   { "glBufferData", 15, -1 },
   { "glBufferSubData", 15, -1 },
   { "glGetBufferSubData", 15, -1 },
   { "glMapBuffer", 15, -1 },
   { "glUnmapBuffer", 15, -1 },
   { "glGetBufferParameteriv", 15, -1 },
   { "glGetBufferPointerv", 15, -1 },

   /* GL 2.0 */
   { "glBlendEquationSeparate", 20, -1 },
   { "glDrawBuffers", 20, -1 },
   { "glStencilOpSeparate", 20, -1 },
   { "glStencilFuncSeparate", 20, -1 },
   { "glStencilMaskSeparate", 20, -1 },
   { "glAttachShader", 20, -1 },
   { "glBindAttribLocation", 20, -1 },
   { "glCompileShader", 20, -1 },
   { "glCreateProgram", 20, -1 },
   { "glCreateShader", 20, -1 },
   { "glDeleteProgram", 20, -1 },
   { "glDeleteShader", 20, -1 },
   { "glDetachShader", 20, -1 },
   { "glDisableVertexAttribArray", 20, -1 },
   { "glEnableVertexAttribArray", 20, -1 },
   { "glGetActiveAttrib", 20, -1 },
   { "glGetActiveUniform", 20, -1 },
   { "glGetAttachedShaders", 20, -1 },
   { "glGetAttribLocation", 20, -1 },
   { "glGetProgramiv", 20, -1 },
   { "glGetProgramInfoLog", 20, -1 },
   { "glGetShaderiv", 20, -1 },
   { "glGetShaderInfoLog", 20, -1 },
   { "glGetShaderSource", 20, -1 },
   { "glGetUniformLocation", 20, -1 },
   { "glGetUniformfv", 20, -1 },
   { "glGetUniformiv", 20, -1 },
   { "glGetVertexAttribdv", 20, -1 },
   { "glGetVertexAttribfv", 20, -1 },
   { "glGetVertexAttribiv", 20, -1 },
   { "glGetVertexAttribPointerv", 20, -1 },
   { "glIsProgram", 20, -1 },
   { "glIsShader", 20, -1 },
   { "glLinkProgram", 20, -1 },
   { "glShaderSource", 20, -1 },
   { "glUseProgram", 20, -1 },
   { "glUniform1f", 20, -1 },
   { "glUniform2f", 20, -1 },
   { "glUniform3f", 20, -1 },
   { "glUniform4f", 20, -1 },
   { "glUniform1i", 20, -1 },
   { "glUniform2i", 20, -1 },
   { "glUniform3i", 20, -1 },
   { "glUniform4i", 20, -1 },
   { "glUniform1fv", 20, -1 },
   { "glUniform2fv", 20, -1 },
   { "glUniform3fv", 20, -1 },
   { "glUniform4fv", 20, -1 },
   { "glUniform1iv", 20, -1 },
   { "glUniform2iv", 20, -1 },
   { "glUniform3iv", 20, -1 },
   { "glUniform4iv", 20, -1 },
   { "glUniformMatrix2fv", 20, -1 },
   { "glUniformMatrix3fv", 20, -1 },
   { "glUniformMatrix4fv", 20, -1 },
   { "glValidateProgram", 20, -1 },
   { "glVertexAttrib1d", 20, -1 },
   { "glVertexAttrib1dv", 20, -1 },
   { "glVertexAttrib1f", 20, -1 },
   { "glVertexAttrib1fv", 20, -1 },
   { "glVertexAttrib1s", 20, -1 },
   { "glVertexAttrib1sv", 20, -1 },
   { "glVertexAttrib2d", 20, -1 },
   { "glVertexAttrib2dv", 20, -1 },
   { "glVertexAttrib2f", 20, -1 },
   { "glVertexAttrib2fv", 20, -1 },
   { "glVertexAttrib2s", 20, -1 },
   { "glVertexAttrib2sv", 20, -1 },
   { "glVertexAttrib3d", 20, -1 },
   { "glVertexAttrib3dv", 20, -1 },
   { "glVertexAttrib3f", 20, -1 },
   { "glVertexAttrib3fv", 20, -1 },
   { "glVertexAttrib3s", 20, -1 },
   { "glVertexAttrib3sv", 20, -1 },
   { "glVertexAttrib4Nbv", 20, -1 },
   { "glVertexAttrib4Niv", 20, -1 },
   { "glVertexAttrib4Nsv", 20, -1 },
   { "glVertexAttrib4Nub", 20, -1 },
   { "glVertexAttrib4Nubv", 20, -1 },
   { "glVertexAttrib4Nuiv", 20, -1 },
   { "glVertexAttrib4Nusv", 20, -1 },
   { "glVertexAttrib4bv", 20, -1 },
   { "glVertexAttrib4d", 20, -1 },
   { "glVertexAttrib4dv", 20, -1 },
   { "glVertexAttrib4f", 20, -1 },
   { "glVertexAttrib4fv", 20, -1 },
   { "glVertexAttrib4iv", 20, -1 },
   { "glVertexAttrib4s", 20, -1 },
   { "glVertexAttrib4sv", 20, -1 },
   { "glVertexAttrib4ubv", 20, -1 },
   { "glVertexAttrib4uiv", 20, -1 },
   { "glVertexAttrib4usv", 20, -1 },
   { "glVertexAttribPointer", 20, -1 },

   /* GL 2.1 */
   { "glUniformMatrix2x3fv", 21, -1 },
   { "glUniformMatrix3x2fv", 21, -1 },
   { "glUniformMatrix2x4fv", 21, -1 },
   { "glUniformMatrix4x2fv", 21, -1 },
   { "glUniformMatrix3x4fv", 21, -1 },
   { "glUniformMatrix4x3fv", 21, -1 },

   /* GL 3.0 */
   { "glColorMaski", 30, -1 },
   { "glGetBooleani_v", 30, -1 },
   { "glGetIntegeri_v", 30, -1 },
   { "glEnablei", 30, -1 },
   { "glDisablei", 30, -1 },
   { "glIsEnabledi", 30, -1 },
   { "glBeginTransformFeedback", 30, -1 },
   { "glEndTransformFeedback", 30, -1 },
   { "glBindBufferRange", 30, -1 },
   { "glBindBufferBase", 30, -1 },
   { "glTransformFeedbackVaryings", 30, -1 },
   { "glGetTransformFeedbackVarying", 30, -1 },
   { "glClampColor", 30, -1 },
   { "glBeginConditionalRender", 30, -1 },
   { "glEndConditionalRender", 30, -1 },
   { "glVertexAttribIPointer", 30, -1 },
   { "glGetVertexAttribIiv", 30, -1 },
   { "glGetVertexAttribIuiv", 30, -1 },
   { "glVertexAttribI1i", 30, -1 },
   { "glVertexAttribI2i", 30, -1 },
   { "glVertexAttribI3i", 30, -1 },
   { "glVertexAttribI4i", 30, -1 },
   { "glVertexAttribI1ui", 30, -1 },
   { "glVertexAttribI2ui", 30, -1 },
   { "glVertexAttribI3ui", 30, -1 },
   { "glVertexAttribI4ui", 30, -1 },
   { "glVertexAttribI1iv", 30, -1 },
   { "glVertexAttribI2iv", 30, -1 },
   { "glVertexAttribI3iv", 30, -1 },
   { "glVertexAttribI4iv", 30, -1 },
   { "glVertexAttribI1uiv", 30, -1 },
   { "glVertexAttribI2uiv", 30, -1 },
   { "glVertexAttribI3uiv", 30, -1 },
   { "glVertexAttribI4uiv", 30, -1 },
   { "glVertexAttribI4bv", 30, -1 },
   { "glVertexAttribI4sv", 30, -1 },
   { "glVertexAttribI4ubv", 30, -1 },
   { "glVertexAttribI4usv", 30, -1 },
   { "glGetUniformuiv", 30, -1 },
   { "glBindFragDataLocation", 30, -1 },
   { "glGetFragDataLocation", 30, -1 },
   { "glUniform1ui", 30, -1 },
   { "glUniform2ui", 30, -1 },
   { "glUniform3ui", 30, -1 },
   { "glUniform4ui", 30, -1 },
   { "glUniform1uiv", 30, -1 },
   { "glUniform2uiv", 30, -1 },
   { "glUniform3uiv", 30, -1 },
   { "glUniform4uiv", 30, -1 },
   { "glTexParameterIiv", 30, -1 },
   { "glTexParameterIuiv", 30, -1 },
   { "glGetTexParameterIiv", 30, -1 },
   { "glGetTexParameterIuiv", 30, -1 },
   { "glClearBufferiv", 30, -1 },
   { "glClearBufferuiv", 30, -1 },
   { "glClearBufferfv", 30, -1 },
   { "glClearBufferfi", 30, -1 },
   { "glGetStringi", 30, -1 },

   /* GL 3.1 */
   { "glDrawArraysInstanced", 31, -1 },
   { "glDrawElementsInstanced", 31, -1 },
   { "glPrimitiveRestartIndex", 31, -1 },
   { "glTexBuffer", 31, -1 },

   /* GL_ARB_texture_buffer_range */
   { "glTexBufferRange", 43, -1 },

   /* GL_ARB_shader_objects */
   { "glDeleteObjectARB", 31, -1 },
   { "glGetHandleARB", 31, -1 },
   { "glDetachObjectARB", 31, -1 },
   { "glCreateShaderObjectARB", 31, -1 },
   { "glCreateProgramObjectARB", 31, -1 },
   { "glAttachObjectARB", 31, -1 },
   { "glGetObjectParameterfvARB", 31, -1 },
   { "glGetObjectParameterivARB", 31, -1 },
   { "glGetInfoLogARB", 31, -1 },
   { "glGetAttachedObjectsARB", 31, -1 },

   /* GL_ARB_get_program_binary */
   { "glGetProgramBinary", 30, -1 },
   { "glProgramBinary", 30, -1 },
   { "glProgramParameteri", 30, -1 },

   /* GL_EXT_transform_feedback */
   { "glBindBufferOffsetEXT", 31, -1 },

   /* GL_IBM_multimode_draw_arrays */
   { "glMultiModeDrawArraysIBM", 31, -1 },
   { "glMultiModeDrawElementsIBM", 31, -1 },

   /* GL_EXT_depth_bounds_test */
   { "glDepthBoundsEXT", 31, -1 },

   /* GL_apple_object_purgeable */
   { "glObjectPurgeableAPPLE", 31, -1 },
   { "glObjectUnpurgeableAPPLE", 31, -1 },
   { "glGetObjectParameterivAPPLE", 31, -1 },

   /* GL_ARB_instanced_arrays */
   { "glVertexAttribDivisorARB", 31, -1 },

   /* GL_NV_texture_barrier */
   { "glTextureBarrierNV", 31, -1 },

   /* GL_EXT_texture_integer */
   { "glClearColorIiEXT", 31, -1 },
   { "glClearColorIuiEXT", 31, -1 },

   /* GL_OES_EGL_image */
   { "glEGLImageTargetRenderbufferStorageOES", 31, -1 },
   { "glEGLImageTargetTexture2DOES", 31, -1 },

   /* GL 3.2 */
   { "glGetInteger64i_v", 32, -1 },
   { "glGetBufferParameteri64v", 32, -1 },
   { "glFramebufferTexture", 32, -1 },
   { "glProgramParameteri", 32, -1 },
   { "glFramebufferTextureLayer", 32, -1 },

   /* GL 3.3 */
   { "glVertexAttribDivisor", 33, -1 },

   /* GL 4.0 */
   { "glMinSampleShading", 40, -1 },
   { "glPatchParameteri", 40, -1 },
   { "glPatchParameterfv", 40, -1 },
   { "glBlendEquationi", 40, -1 },
   { "glBlendEquationSeparatei", 40, -1 },
   { "glBlendFunci", 40, -1 },
   { "glBlendFuncSeparatei", 40, -1 },

   { "glGetSubroutineUniformLocation", 40, -1 },
   { "glGetSubroutineIndex", 40, -1 },
   { "glGetActiveSubroutineUniformiv", 40, -1 },
   { "glGetActiveSubroutineUniformName", 40, -1 },
   { "glGetActiveSubroutineName", 40, -1 },
   { "glUniformSubroutinesuiv", 40, -1 },
   { "glGetUniformSubroutineuiv", 40, -1 },
   { "glGetProgramStageiv", 40, -1 },

   { "glUniform1d", 40, -1 },
   { "glUniform2d", 40, -1 },
   { "glUniform3d", 40, -1 },
   { "glUniform4d", 40, -1 },
   { "glUniform1dv", 40, -1 },
   { "glUniform2dv", 40, -1 },
   { "glUniform3dv", 40, -1 },
   { "glUniform4dv", 40, -1 },
   { "glUniformMatrix2dv", 40, -1 },
   { "glUniformMatrix3dv", 40, -1 },
   { "glUniformMatrix4dv", 40, -1 },
   { "glUniformMatrix2x3dv", 40, -1 },
   { "glUniformMatrix2x4dv", 40, -1 },
   { "glUniformMatrix3x2dv", 40, -1 },
   { "glUniformMatrix3x4dv", 40, -1 },
   { "glUniformMatrix4x2dv", 40, -1 },
   { "glUniformMatrix4x3dv", 40, -1 },
   { "glGetUniformdv", 43, -1 },

   /* GL 4.1 */
   { "glVertexAttribL1d", 41, -1 },
   { "glVertexAttribL2d", 41, -1 },
   { "glVertexAttribL3d", 41, -1 },
   { "glVertexAttribL4d", 41, -1 },
   { "glVertexAttribL1dv", 41, -1 },
   { "glVertexAttribL2dv", 41, -1 },
   { "glVertexAttribL3dv", 41, -1 },
   { "glVertexAttribL4dv", 41, -1 },
   { "glVertexAttribLPointer", 41, -1 },
   { "glGetVertexAttribLdv", 41, -1 },

   /* GL 4.3 */
   { "glIsRenderbuffer", 43, -1 },
   { "glBindRenderbuffer", 43, -1 },
   { "glDeleteRenderbuffers", 43, -1 },
   { "glGenRenderbuffers", 43, -1 },
   { "glRenderbufferStorage", 43, -1 },
   { "glGetRenderbufferParameteriv", 43, -1 },
   { "glIsFramebuffer", 43, -1 },
   { "glBindFramebuffer", 43, -1 },
   { "glDeleteFramebuffers", 43, -1 },
   { "glGenFramebuffers", 43, -1 },
   { "glCheckFramebufferStatus", 43, -1 },
   { "glFramebufferTexture1D", 43, -1 },
   { "glFramebufferTexture2D", 43, -1 },
   { "glFramebufferTexture3D", 43, -1 },
   { "glFramebufferRenderbuffer", 43, -1 },
   { "glGetFramebufferAttachmentParameteriv", 43, -1 },
   { "glGenerateMipmap", 43, -1 },
   { "glBlitFramebuffer", 43, -1 },
   { "glRenderbufferStorageMultisample", 43, -1 },
   { "glFramebufferTextureLayer", 43, -1 },
   { "glMapBufferRange", 43, -1 },
   { "glFlushMappedBufferRange", 43, -1 },
   { "glBindVertexArray", 43, -1 },
   { "glDeleteVertexArrays", 43, -1 },
   { "glGenVertexArrays", 43, -1 },
   { "glIsVertexArray", 43, -1 },
   { "glGetUniformIndices", 43, -1 },
   { "glGetActiveUniformsiv", 43, -1 },
   { "glGetActiveUniformName", 43, -1 },
   { "glGetUniformBlockIndex", 43, -1 },
   { "glGetActiveUniformBlockiv", 43, -1 },
   { "glGetActiveUniformBlockName", 43, -1 },
   { "glUniformBlockBinding", 43, -1 },
   { "glCopyBufferSubData", 43, -1 },
   { "glDrawElementsBaseVertex", 43, -1 },
   { "glDrawRangeElementsBaseVertex", 43, -1 },
   { "glDrawElementsInstancedBaseVertex", 43, -1 },
   { "glMultiDrawElementsBaseVertex", 43, -1 },
   { "glProvokingVertex", 43, -1 },
   { "glFenceSync", 43, -1 },
   { "glIsSync", 43, -1 },
   { "glDeleteSync", 43, -1 },
   { "glClientWaitSync", 43, -1 },
   { "glWaitSync", 43, -1 },
   { "glGetInteger64v", 43, -1 },
   { "glGetSynciv", 43, -1 },
   { "glTexImage2DMultisample", 43, -1 },
   { "glTexImage3DMultisample", 43, -1 },
   { "glGetMultisamplefv", 43, -1 },
   { "glSampleMaski", 43, -1 },
   { "glBlendEquationiARB", 43, -1 },
   { "glBlendEquationSeparateiARB", 43, -1 },
   { "glBlendFunciARB", 43, -1 },
   { "glBlendFuncSeparateiARB", 43, -1 },
   { "glMinSampleShadingARB", 43, -1 },                 // XXX: Add to xml
// { "glNamedStringARB", 43, -1 },                      // XXX: Add to xml
// { "glDeleteNamedStringARB", 43, -1 },                // XXX: Add to xml
// { "glCompileShaderIncludeARB", 43, -1 },             // XXX: Add to xml
// { "glIsNamedStringARB", 43, -1 },                    // XXX: Add to xml
// { "glGetNamedStringARB", 43, -1 },                   // XXX: Add to xml
// { "glGetNamedStringivARB", 43, -1 },                 // XXX: Add to xml
   { "glBindFragDataLocationIndexed", 43, -1 },
   { "glGetFragDataIndex", 43, -1 },
   { "glGenSamplers", 43, -1 },
   { "glDeleteSamplers", 43, -1 },
   { "glIsSampler", 43, -1 },
   { "glBindSampler", 43, -1 },
   { "glSamplerParameteri", 43, -1 },
   { "glSamplerParameteriv", 43, -1 },
   { "glSamplerParameterf", 43, -1 },
   { "glSamplerParameterfv", 43, -1 },
   { "glSamplerParameterIiv", 43, -1 },
   { "glSamplerParameterIuiv", 43, -1 },
   { "glGetSamplerParameteriv", 43, -1 },
   { "glGetSamplerParameterIiv", 43, -1 },
   { "glGetSamplerParameterfv", 43, -1 },
   { "glGetSamplerParameterIuiv", 43, -1 },
   { "glQueryCounter", 43, -1 },
   { "glGetQueryObjecti64v", 43, -1 },
   { "glGetQueryObjectui64v", 43, -1 },
   { "glVertexP2ui", 43, -1 },
   { "glVertexP2uiv", 43, -1 },
   { "glVertexP3ui", 43, -1 },
   { "glVertexP3uiv", 43, -1 },
   { "glVertexP4ui", 43, -1 },
   { "glVertexP4uiv", 43, -1 },
   { "glTexCoordP1ui", 43, -1 },
   { "glTexCoordP1uiv", 43, -1 },
   { "glTexCoordP2ui", 43, -1 },
   { "glTexCoordP2uiv", 43, -1 },
   { "glTexCoordP3ui", 43, -1 },
   { "glTexCoordP3uiv", 43, -1 },
   { "glTexCoordP4ui", 43, -1 },
   { "glTexCoordP4uiv", 43, -1 },
   { "glMultiTexCoordP1ui", 43, -1 },
   { "glMultiTexCoordP1uiv", 43, -1 },
   { "glMultiTexCoordP2ui", 43, -1 },
   { "glMultiTexCoordP2uiv", 43, -1 },
   { "glMultiTexCoordP3ui", 43, -1 },
   { "glMultiTexCoordP3uiv", 43, -1 },
   { "glMultiTexCoordP4ui", 43, -1 },
   { "glMultiTexCoordP4uiv", 43, -1 },
   { "glNormalP3ui", 43, -1 },
   { "glNormalP3uiv", 43, -1 },
   { "glColorP3ui", 43, -1 },
   { "glColorP3uiv", 43, -1 },
   { "glColorP4ui", 43, -1 },
   { "glColorP4uiv", 43, -1 },
   { "glSecondaryColorP3ui", 43, -1 },
   { "glSecondaryColorP3uiv", 43, -1 },
   { "glVertexAttribP1ui", 43, -1 },
   { "glVertexAttribP1uiv", 43, -1 },
   { "glVertexAttribP2ui", 43, -1 },
   { "glVertexAttribP2uiv", 43, -1 },
   { "glVertexAttribP3ui", 43, -1 },
   { "glVertexAttribP3uiv", 43, -1 },
   { "glVertexAttribP4ui", 43, -1 },
   { "glVertexAttribP4uiv", 43, -1 },
   { "glDrawArraysIndirect", 43, -1 },
   { "glDrawElementsIndirect", 43, -1 },
   { "glBindTransformFeedback", 43, -1 },
   { "glDeleteTransformFeedbacks", 43, -1 },
   { "glGenTransformFeedbacks", 43, -1 },
   { "glIsTransformFeedback", 43, -1 },
   { "glPauseTransformFeedback", 43, -1 },
   { "glResumeTransformFeedback", 43, -1 },
   { "glDrawTransformFeedback", 43, -1 },
   { "glDrawTransformFeedbackStream", 43, -1 },
   { "glBeginQueryIndexed", 43, -1 },
   { "glEndQueryIndexed", 43, -1 },
   { "glGetQueryIndexediv", 43, -1 },
   { "glReleaseShaderCompiler", 43, -1 },
   { "glShaderBinary", 43, -1 },
   { "glGetShaderPrecisionFormat", 43, -1 },
   { "glDepthRangef", 43, -1 },
   { "glClearDepthf", 43, -1 },
   { "glGetProgramBinary", 43, -1 },
   { "glProgramBinary", 43, -1 },
   { "glProgramParameteri", 43, -1 },
   { "glUseProgramStages", 43, -1 },
   { "glActiveShaderProgram", 43, -1 },
   { "glCreateShaderProgramv", 43, -1 },
   { "glBindProgramPipeline", 43, -1 },
   { "glDeleteProgramPipelines", 43, -1 },
   { "glGenProgramPipelines", 43, -1 },
   { "glIsProgramPipeline", 43, -1 },
   { "glGetProgramPipelineiv", 43, -1 },
   { "glProgramUniform1d", 43, -1 },
   { "glProgramUniform1dv", 43, -1 },
   { "glProgramUniform1i", 43, -1 },
   { "glProgramUniform1iv", 43, -1 },
   { "glProgramUniform1f", 43, -1 },
   { "glProgramUniform1fv", 43, -1 },
   { "glProgramUniform1ui", 43, -1 },
   { "glProgramUniform1uiv", 43, -1 },
   { "glProgramUniform2i", 43, -1 },
   { "glProgramUniform2iv", 43, -1 },
   { "glProgramUniform2f", 43, -1 },
   { "glProgramUniform2fv", 43, -1 },
   { "glProgramUniform2d", 40, -1 },
   { "glProgramUniform2dv", 40, -1 },
   { "glProgramUniform2ui", 43, -1 },
   { "glProgramUniform2uiv", 43, -1 },
   { "glProgramUniform3i", 43, -1 },
   { "glProgramUniform3iv", 43, -1 },
   { "glProgramUniform3f", 43, -1 },
   { "glProgramUniform3fv", 43, -1 },
   { "glProgramUniform3d", 40, -1 },
   { "glProgramUniform3dv", 40, -1 },
   { "glProgramUniform3ui", 43, -1 },
   { "glProgramUniform3uiv", 43, -1 },
   { "glProgramUniform4i", 43, -1 },
   { "glProgramUniform4iv", 43, -1 },
   { "glProgramUniform4d", 43, -1 },
   { "glProgramUniform4dv", 43, -1 },
   { "glProgramUniform4f", 43, -1 },
   { "glProgramUniform4fv", 43, -1 },
   { "glProgramUniform4ui", 43, -1 },
   { "glProgramUniform4uiv", 43, -1 },
   { "glProgramUniformMatrix2dv", 43, -1 },
   { "glProgramUniformMatrix2fv", 43, -1 },
   { "glProgramUniformMatrix3dv", 43, -1 },
   { "glProgramUniformMatrix3fv", 43, -1 },
   { "glProgramUniformMatrix4dv", 43, -1 },
   { "glProgramUniformMatrix4fv", 43, -1 },
   { "glProgramUniformMatrix2x3dv", 43, -1 },
   { "glProgramUniformMatrix2x3fv", 43, -1 },
   { "glProgramUniformMatrix3x2dv", 43, -1 },
   { "glProgramUniformMatrix3x2fv", 43, -1 },
   { "glProgramUniformMatrix2x4dv", 43, -1 },
   { "glProgramUniformMatrix2x4fv", 43, -1 },
   { "glProgramUniformMatrix4x2dv", 43, -1 },
   { "glProgramUniformMatrix4x2fv", 43, -1 },
   { "glProgramUniformMatrix3x4dv", 43, -1 },
   { "glProgramUniformMatrix3x4fv", 43, -1 },
   { "glProgramUniformMatrix4x3dv", 43, -1 },
   { "glProgramUniformMatrix4x3fv", 43, -1 },
   { "glValidateProgramPipeline", 43, -1 },
   { "glGetProgramPipelineInfoLog", 43, -1 },
   { "glGetFloati_v", 43, -1 },
   { "glGetDoublei_v", 43, -1 },
// { "glCreateSyncFromCLeventARB", 43, -1 },            // XXX: Add to xml
   { "glGetGraphicsResetStatusARB", 43, -1 },
   { "glGetnMapdvARB", 43, -1 },
   { "glGetnMapfvARB", 43, -1 },
   { "glGetnMapivARB", 43, -1 },
   { "glGetnPixelMapfvARB", 43, -1 },
   { "glGetnPixelMapuivARB", 43, -1 },
   { "glGetnPixelMapusvARB", 43, -1 },
   { "glGetnPolygonStippleARB", 43, -1 },
   { "glGetnColorTableARB", 43, -1 },
   { "glGetnConvolutionFilterARB", 43, -1 },
   { "glGetnSeparableFilterARB", 43, -1 },
   { "glGetnHistogramARB", 43, -1 },
   { "glGetnMinmaxARB", 43, -1 },
   { "glGetnTexImageARB", 43, -1 },
   { "glReadnPixelsARB", 43, -1 },
   { "glGetnCompressedTexImageARB", 43, -1 },
   { "glGetnUniformfvARB", 43, -1 },
   { "glGetnUniformivARB", 43, -1 },
   { "glGetnUniformuivARB", 43, -1 },
   { "glGetnUniformdvARB", 43, -1 },
   { "glDrawArraysInstancedBaseInstance", 43, -1 },
   { "glDrawElementsInstancedBaseInstance", 43, -1 },
   { "glDrawElementsInstancedBaseVertexBaseInstance", 43, -1 },
   { "glDrawTransformFeedbackInstanced", 43, -1 },
   { "glDrawTransformFeedbackStreamInstanced", 43, -1 },
// { "glGetInternalformativ", 43, -1 },                 // XXX: Add to xml
   { "glGetActiveAtomicCounterBufferiv", 43, -1 },
   { "glBindImageTexture", 43, -1 },
   { "glMemoryBarrier", 43, -1 },
   { "glTexStorage1D", 43, -1 },
   { "glTexStorage2D", 43, -1 },
   { "glTexStorage3D", 43, -1 },
   { "glTextureStorage1DEXT", 43, -1 },
   { "glTextureStorage2DEXT", 43, -1 },
   { "glTextureStorage3DEXT", 43, -1 },
   { "glClearBufferData", 43, -1 },
   { "glClearBufferSubData", 43, -1 },
// { "glClearNamedBufferDataEXT", 43, -1 },             // XXX: Add to xml
// { "glClearNamedBufferSubDataEXT", 43, -1 },          // XXX: Add to xml
   { "glCopyImageSubData", 43, -1 },
   { "glTextureView", 43, -1 },
   { "glBindVertexBuffer", 43, -1 },
   { "glVertexAttribFormat", 43, -1 },
   { "glVertexAttribIFormat", 43, -1 },
   { "glVertexAttribLFormat", 43, -1 },
   { "glVertexAttribBinding", 43, -1 },
   { "glVertexBindingDivisor", 43, -1 },
// { "glVertexArrayBindVertexBufferEXT", 43, -1 },      // XXX: Add to xml
// { "glVertexArrayVertexAttribFormatEXT", 43, -1 },    // XXX: Add to xml
// { "glVertexArrayVertexAttribIFormatEXT", 43, -1 },   // XXX: Add to xml
// { "glVertexArrayVertexAttribBindingEXT", 43, -1 },   // XXX: Add to xml
// { "glVertexArrayVertexBindingDivisorEXT", 43, -1 },  // XXX: Add to xml
   { "glFramebufferParameteri", 43, -1 },
   { "glGetFramebufferParameteriv", 43, -1 },
// { "glNamedFramebufferParameteriEXT", 43, -1 },       // XXX: Add to xml
// { "glGetNamedFramebufferParameterivEXT", 43, -1 },   // XXX: Add to xml
// { "glGetInternalformati64v", 43, -1 },               // XXX: Add to xml
   { "glInvalidateTexSubImage", 43, -1 },
   { "glInvalidateTexImage", 43, -1 },
   { "glInvalidateBufferSubData", 43, -1 },
   { "glInvalidateBufferData", 43, -1 },
   { "glInvalidateFramebuffer", 43, -1 },
   { "glInvalidateSubFramebuffer", 43, -1 },
   { "glMultiDrawArraysIndirect", 43, -1 },
   { "glMultiDrawElementsIndirect", 43, -1 },
   { "glGetProgramInterfaceiv", 43, -1 },
   { "glGetProgramResourceIndex", 43, -1 },
   { "glGetProgramResourceName", 43, -1 },
   { "glGetProgramResourceiv", 43, -1 },
   { "glGetProgramResourceLocation", 43, -1 },
   { "glGetProgramResourceLocationIndex", 43, -1 },
   { "glShaderStorageBlockBinding", 43, -1 },
// { "glTextureBufferRangeEXT", 43, -1 },               // XXX: Add to xml
   { "glTexStorage2DMultisample", 43, -1 },
   { "glTexStorage3DMultisample", 43, -1 },
// { "glTextureStorage2DMultisampleEXT", 43, -1 },      // XXX: Add to xml
// { "glTextureStorage3DMultisampleEXT", 43, -1 },      // XXX: Add to xml

   { "glViewportArrayv", 43, -1 },
   { "glViewportIndexedf", 43, -1 },
   { "glViewportIndexedfv", 43, -1 },
   { "glScissorArrayv", 43, -1 },
   { "glScissorIndexed", 43, -1 },
   { "glScissorIndexedv", 43, -1 },
   { "glDepthRangeArrayv", 43, -1 },
   { "glDepthRangeIndexed", 43, -1 },

/* GL 4.5 */
   /* aliased versions checked above */
   //{ "glGetGraphicsResetStatus", 45, -1 },
   //{ "glReadnPixels", 45, -1 },
   //{ "glGetnUniformfv", 45, -1 },
   //{ "glGetnUniformiv", 45, -1 },
   //{ "glGetnUniformuiv", 45, -1 },
   { "glMemoryBarrierByRegion", 45, -1 },

   /* GL_ARB_direct_state_access */
   { "glCreateTransformFeedbacks", 45, -1 },
   { "glTransformFeedbackBufferBase", 45, -1 },
   { "glTransformFeedbackBufferRange", 45, -1 },
   { "glGetTransformFeedbackiv", 45, -1 },
   { "glGetTransformFeedbacki_v", 45, -1 },
   { "glGetTransformFeedbacki64_v", 45, -1 },
   { "glCreateBuffers", 45, -1 },
   { "glNamedBufferStorage", 45, -1 },
   { "glNamedBufferData", 45, -1 },
   { "glNamedBufferSubData", 45, -1 },
   { "glCopyNamedBufferSubData", 45, -1 },
   { "glClearNamedBufferData", 45, -1 },
   { "glClearNamedBufferSubData", 45, -1 },
   { "glMapNamedBuffer", 45, -1 },
   { "glMapNamedBufferRange", 45, -1 },
   { "glUnmapNamedBuffer", 45, -1 },
   { "glFlushMappedNamedBufferRange", 45, -1 },
   { "glGetNamedBufferParameteriv", 45, -1 },
   { "glGetNamedBufferParameteri64v", 45, -1 },
   { "glGetNamedBufferPointerv", 45, -1 },
   { "glGetNamedBufferSubData", 45, -1 },
   { "glCreateFramebuffers", 45, -1 },
   { "glNamedFramebufferRenderbuffer", 45, -1 },
   { "glNamedFramebufferParameteri", 45, -1 },
   { "glNamedFramebufferTexture", 45, -1 },
   { "glNamedFramebufferTextureLayer", 45, -1 },
   { "glNamedFramebufferDrawBuffer", 45, -1 },
   { "glNamedFramebufferDrawBuffers", 45, -1 },
   { "glNamedFramebufferReadBuffer", 45, -1 },
   { "glInvalidateNamedFramebufferSubData", 45, -1 },
   { "glInvalidateNamedFramebufferData", 45, -1 },
   { "glClearNamedFramebufferiv", 45, -1 },
   { "glClearNamedFramebufferuiv", 45, -1 },
   { "glClearNamedFramebufferfv", 45, -1 },
   { "glClearNamedFramebufferfi", 45, -1 },
   { "glBlitNamedFramebuffer", 45, -1 },
   { "glCheckNamedFramebufferStatus", 45, -1 },
   { "glGetNamedFramebufferParameteriv", 45, -1 },
   { "glGetNamedFramebufferAttachmentParameteriv", 45, -1 },
   { "glCreateRenderbuffers", 45, -1 },
   { "glNamedRenderbufferStorage", 45, -1 },
   { "glNamedRenderbufferStorageMultisample", 45, -1 },
   { "glGetNamedRenderbufferParameteriv", 45, -1 },
   { "glCreateTextures", 45, -1 },
   { "glTextureStorage1D", 45, -1 },
   { "glTextureStorage2D", 45, -1 },
   { "glTextureStorage3D", 45, -1 },
   { "glTextureSubImage1D", 45, -1 },
   { "glTextureSubImage2D", 45, -1 },
   { "glTextureSubImage3D", 45, -1 },
   { "glBindTextureUnit", 45, -1 },
   { "glTextureParameterf", 45, -1 },
   { "glTextureParameterfv", 45, -1 },
   { "glTextureParameteri", 45, -1 },
   { "glTextureParameterIiv", 45, -1 },
   { "glTextureParameterIuiv", 45, -1 },
   { "glTextureParameteriv", 45, -1 },
   { "glGetTextureLevelParameterfv", 45, -1 },
   { "glGetTextureLevelParameteriv", 45, -1 },
   { "glGetTextureParameterfv", 45, -1 },
   { "glGetTextureParameterIiv", 45, -1 },
   { "glGetTextureParameterIuiv", 45, -1 },
   { "glGetTextureParameteriv", 45, -1 },
   { "glCopyTextureSubImage1D", 45, -1 },
   { "glCopyTextureSubImage2D", 45, -1 },
   { "glCopyTextureSubImage3D", 45, -1 },
   { "glGetTextureImage", 45, -1 },
   { "glGetCompressedTextureImage", 45, -1 },
   { "glCompressedTextureSubImage1D", 45, -1 },
   { "glCompressedTextureSubImage2D", 45, -1 },
   { "glCompressedTextureSubImage3D", 45, -1 },
   { "glGenerateTextureMipmap", 45, -1 },
   { "glTextureStorage2DMultisample", 45, -1 },
   { "glTextureStorage3DMultisample", 45, -1 },
   { "glTextureBuffer", 45, -1 },
   { "glTextureBufferRange", 45, -1 },
   { "glCreateVertexArrays", 45, -1 },
   { "glDisableVertexArrayAttrib", 45, -1 },
   { "glEnableVertexArrayAttrib", 45, -1 },
   { "glVertexArrayElementBuffer", 45, -1 },
   { "glVertexArrayVertexBuffer", 45, -1 },
   { "glVertexArrayVertexBuffers", 45, -1 },
   { "glVertexArrayAttribFormat", 45, -1 },
   { "glVertexArrayAttribIFormat", 45, -1 },
   { "glVertexArrayAttribLFormat", 45, -1 },
   { "glVertexArrayAttribBinding", 45, -1 },
   { "glVertexArrayBindingDivisor", 45, -1 },
   { "glGetVertexArrayiv", 45, -1 },
   { "glGetVertexArrayIndexediv", 45, -1 },
   { "glGetVertexArrayIndexed64iv", 45, -1 },
   { "glCreateSamplers", 45, -1 },
   { "glCreateProgramPipelines", 45, -1 },
   { "glCreateQueries", 45, -1 },
   { "glGetQueryBufferObjectiv", 45, -1 },
   { "glGetQueryBufferObjectuiv", 45, -1 },
   { "glGetQueryBufferObjecti64v", 45, -1 },
   { "glGetQueryBufferObjectui64v", 45, -1 },

   /* GL_ARB_internalformat_query */
   { "glGetInternalformativ", 30, -1 },

   /* GL_ARB_internalformat_query */
   { "glGetInternalformati64v", 30, -1 },

   /* GL_ARB_multi_bind */
   { "glBindBuffersBase", 44, -1 },
   { "glBindBuffersRange", 44, -1 },
   { "glBindTextures", 44, -1 },
   { "glBindSamplers", 44, -1 },
   { "glBindImageTextures", 44, -1 },
   { "glBindVertexBuffers", 44, -1 },

   /* GL_KHR_debug/GL_ARB_debug_output */
   { "glPushDebugGroup", 11, -1 },
   { "glPopDebugGroup", 11, -1 },
   { "glDebugMessageCallback", 11, -1 },
   { "glDebugMessageControl", 11, -1 },
   { "glDebugMessageInsert", 11, -1 },
   { "glGetDebugMessageLog", 11, -1 },
   { "glGetObjectLabel", 11, -1 },
   { "glGetObjectPtrLabel", 11, -1 },
   { "glObjectLabel", 11, -1 },
   { "glObjectPtrLabel", 11, -1 },
   /* aliased versions checked above */
   //{ "glDebugMessageControlARB", 11, -1 },
   //{ "glDebugMessageInsertARB", 11, -1 },
   //{ "glDebugMessageCallbackARB", 11, -1 },
   //{ "glGetDebugMessageLogARB", 11, -1 },

   /* GL_AMD_performance_monitor */
   { "glGetPerfMonitorGroupsAMD", 11, -1 },
   { "glGetPerfMonitorCountersAMD", 11, -1 },
   { "glGetPerfMonitorGroupStringAMD", 11, -1 },
   { "glGetPerfMonitorCounterStringAMD", 11, -1 },
   { "glGetPerfMonitorCounterInfoAMD", 11, -1 },
   { "glGenPerfMonitorsAMD", 11, -1 },
   { "glDeletePerfMonitorsAMD", 11, -1 },
   { "glSelectPerfMonitorCountersAMD", 11, -1 },
   { "glBeginPerfMonitorAMD", 11, -1 },
   { "glEndPerfMonitorAMD", 11, -1 },
   { "glGetPerfMonitorCounterDataAMD", 11, -1 },

   /* GL_INTEL_performance_query */
   { "glGetFirstPerfQueryIdINTEL", 30, -1 },
   { "glGetNextPerfQueryIdINTEL", 30, -1 },
   { "glGetPerfQueryIdByNameINTEL", 30, -1 },
   { "glGetPerfQueryInfoINTEL", 30, -1 },
   { "glGetPerfCounterInfoINTEL", 30, -1 },
   { "glCreatePerfQueryINTEL", 30, -1 },
   { "glDeletePerfQueryINTEL", 30, -1 },
   { "glBeginPerfQueryINTEL", 30, -1 },
   { "glEndPerfQueryINTEL", 30, -1 },
   { "glGetPerfQueryDataINTEL", 30, -1 },

   /* GL_NV_vdpau_interop */
   { "glVDPAUInitNV", 11, -1 },
   { "glVDPAUFiniNV", 11, -1 },
   { "glVDPAURegisterVideoSurfaceNV", 11, -1 },
   { "glVDPAURegisterOutputSurfaceNV", 11, -1 },
   { "glVDPAUIsSurfaceNV", 11, -1 },
   { "glVDPAUUnregisterSurfaceNV", 11, -1 },
   { "glVDPAUGetSurfaceivNV", 11, -1 },
   { "glVDPAUSurfaceAccessNV", 11, -1 },
   { "glVDPAUMapSurfacesNV", 11, -1 },
   { "glVDPAUUnmapSurfacesNV", 11, -1 },

   /* GL_ARB_buffer_storage */
   { "glBufferStorage", 43, -1 },

   /* GL_ARB_clear_texture */
   { "glClearTexImage", 13, -1 },
   { "glClearTexSubImage", 13, -1 },

   /* GL_ARB_clip_control */
   { "glClipControl", 45, -1 },

   /* GL_ARB_compute_shader */
   { "glDispatchCompute", 43, -1 },
   { "glDispatchComputeIndirect", 43, -1 },

   /* GL_ARB_compute_variable_group_size */
   { "glDispatchComputeGroupSizeARB", 43, -1 },

   /* GL_EXT_polygon_offset_clamp */
   { "glPolygonOffsetClampEXT", 11, -1 },

   /* GL_ARB_get_texture_sub_image */
   { "glGetTextureSubImage", 20, -1 },
   { "glGetCompressedTextureSubImage", 20, -1 },

   /* GL_GREMEDY_string_marker */
   { "glStringMarkerGREMEDY", 15, -1 },

   /* GL_EXT_window_rectangles */
   { "glWindowRectanglesEXT", 30, -1 },

   /* GL_KHR_blend_equation_advanced */
   { "glBlendBarrierKHR", 20, -1 },

   /* GL_ARB_sparse_buffer */
   { "glBufferPageCommitmentARB", 43, -1 },
   { "glNamedBufferPageCommitmentARB", 43, -1 },

   /* GL_ARB_bindless_texture */
   { "glGetTextureHandleARB", 40, -1 },
   { "glGetTextureSamplerHandleARB", 40, -1 },
   { "glMakeTextureHandleResidentARB", 40, -1 },
   { "glMakeTextureHandleNonResidentARB", 40, -1 },
   { "glIsTextureHandleResidentARB", 40, -1 },
   { "glGetImageHandleARB", 40, -1 },
   { "glMakeImageHandleResidentARB", 40, -1 },
   { "glMakeImageHandleNonResidentARB", 40, -1 },
   { "glIsImageHandleResidentARB", 40, -1 },
   { "glUniformHandleui64ARB", 40, -1 },
   { "glUniformHandleui64vARB", 40, -1 },
   { "glProgramUniformHandleui64ARB", 40, -1 },
   { "glProgramUniformHandleui64vARB", 40, -1 },
   { "glVertexAttribL1ui64ARB", 40, -1 },
   { "glVertexAttribL1ui64vARB", 40, -1 },
   { "glGetVertexAttribLui64vARB", 40, -1 },

   /* GL_EXT_external_objects */
   { "glGetUnsignedBytevEXT", 45, -1 },
   { "glGetUnsignedBytei_vEXT", 45, -1 },
   { "glDeleteMemoryObjectsEXT", 45, -1 },
   { "glIsMemoryObjectEXT", 45, -1 },
   { "glCreateMemoryObjectsEXT", 45, -1 },
   { "glMemoryObjectParameterivEXT", 45, -1 },
   { "glGetMemoryObjectParameterivEXT", 45, -1 },
   { "glTexStorageMem2DEXT", 45, -1 },
   { "glTexStorageMem2DMultisampleEXT", 45, -1 },
   { "glTexStorageMem3DEXT", 45, -1 },
   { "glTexStorageMem3DMultisampleEXT", 45, -1 },
   { "glBufferStorageMemEXT", 45, -1 },
   { "glTextureStorageMem2DEXT", 45, -1 },
   { "glTextureStorageMem2DMultisampleEXT", 45, -1 },
   { "glTextureStorageMem3DEXT", 45, -1 },
   { "glTextureStorageMem3DMultisampleEXT", 45, -1 },
   { "glNamedBufferStorageMemEXT", 45, -1 },
   { "glTexStorageMem1DEXT", 45, -1 },
   { "glTextureStorageMem1DEXT", 45, -1 },
   { "glGenSemaphoresEXT", 45, -1 },
   { "glDeleteSemaphoresEXT", 45, -1 },
   { "glIsSemaphoreEXT", 45, -1 },
   { "glSemaphoreParameterui64vEXT", 45, -1 },
   { "glGetSemaphoreParameterui64vEXT", 45, -1 },
   { "glWaitSemaphoreEXT", 45, -1 },
   { "glSignalSemaphoreEXT", 45, -1 },

   /* GL_EXT_external_objects_fd */
   { "glImportMemoryFdEXT", 45, -1 },
   { "glImportSemaphoreFdEXT", 45, -1 },

   /* GL_ARB_gl_spirv */
   { "glSpecializeShaderARB", 45, -1 },

   /* GL_EXT_shader_framebuffer_fetch_non_coherent */
   { "glFramebufferFetchBarrierEXT", 20, -1 },

   /* GL_NV_conservative_raster */
   { "glSubpixelPrecisionBiasNV", 10, -1 },

   /* GL_NV_conservative_raster_dilate */
   { "glConservativeRasterParameterfNV", 10, -1 },

   /* GL_NV_conservative_raster_pre_snap_triangles */
   { "glConservativeRasterParameteriNV", 10, -1 },

   /* GL_ARB_sample_locations */
   { "glFramebufferSampleLocationsfvARB", 30, -1 },
   { "glNamedFramebufferSampleLocationsfvARB", 30, -1 },
   { "glEvaluateDepthValuesARB", 30, -1 },

   /* GL_ARB_indirect_parameters */
   { "glMultiDrawArraysIndirectCountARB", 11, -1 },
   { "glMultiDrawElementsIndirectCountARB", 11, -1 },

   /* GL_AMD_framebuffer_multisample_advanced */
   { "glRenderbufferStorageMultisampleAdvancedAMD", 11, -1 },
   { "glNamedRenderbufferStorageMultisampleAdvancedAMD", 11, -1 },

   { "glMaxShaderCompilerThreadsKHR", 11, -1 },

   { NULL, 0, -1 }
};

const struct function gl_compatibility_functions_possible[] = {
   { "glNewList", 10, _gloffset_NewList },
   { "glEndList", 10, _gloffset_EndList },
   { "glCallList", 10, _gloffset_CallList },
   { "glCallLists", 10, _gloffset_CallLists },
   { "glDeleteLists", 10, _gloffset_DeleteLists },
   { "glGenLists", 10, _gloffset_GenLists },
   { "glListBase", 10, _gloffset_ListBase },
   { "glBegin", 10, _gloffset_Begin },
   { "glBitmap", 10, _gloffset_Bitmap },
   { "glColor3b", 10, _gloffset_Color3b },
   { "glColor3bv", 10, _gloffset_Color3bv },
   { "glColor3d", 10, _gloffset_Color3d },
   { "glColor3dv", 10, _gloffset_Color3dv },
   { "glColor3f", 10, _gloffset_Color3f },
   { "glColor3fv", 10, _gloffset_Color3fv },
   { "glColor3i", 10, _gloffset_Color3i },
   { "glColor3iv", 10, _gloffset_Color3iv },
   { "glColor3s", 10, _gloffset_Color3s },
   { "glColor3sv", 10, _gloffset_Color3sv },
   { "glColor3ub", 10, _gloffset_Color3ub },
   { "glColor3ubv", 10, _gloffset_Color3ubv },
   { "glColor3ui", 10, _gloffset_Color3ui },
   { "glColor3uiv", 10, _gloffset_Color3uiv },
   { "glColor3us", 10, _gloffset_Color3us },
   { "glColor3usv", 10, _gloffset_Color3usv },
   { "glColor4b", 10, _gloffset_Color4b },
   { "glColor4bv", 10, _gloffset_Color4bv },
   { "glColor4d", 10, _gloffset_Color4d },
   { "glColor4dv", 10, _gloffset_Color4dv },
   { "glColor4f", 10, _gloffset_Color4f },
   { "glColor4fv", 10, _gloffset_Color4fv },
   { "glColor4i", 10, _gloffset_Color4i },
   { "glColor4iv", 10, _gloffset_Color4iv },
   { "glColor4s", 10, _gloffset_Color4s },
   { "glColor4sv", 10, _gloffset_Color4sv },
   { "glColor4ub", 10, _gloffset_Color4ub },
   { "glColor4ubv", 10, _gloffset_Color4ubv },
   { "glColor4ui", 10, _gloffset_Color4ui },
   { "glColor4uiv", 10, _gloffset_Color4uiv },
   { "glColor4us", 10, _gloffset_Color4us },
   { "glColor4usv", 10, _gloffset_Color4usv },
   { "glEdgeFlag", 10, _gloffset_EdgeFlag },
   { "glEdgeFlagv", 10, _gloffset_EdgeFlagv },
   { "glEnd", 10, _gloffset_End },
   { "glIndexd", 10, _gloffset_Indexd },
   { "glIndexdv", 10, _gloffset_Indexdv },
   { "glIndexf", 10, _gloffset_Indexf },
   { "glIndexfv", 10, _gloffset_Indexfv },
   { "glIndexi", 10, _gloffset_Indexi },
   { "glIndexiv", 10, _gloffset_Indexiv },
   { "glIndexs", 10, _gloffset_Indexs },
   { "glIndexsv", 10, _gloffset_Indexsv },
   { "glNormal3b", 10, _gloffset_Normal3b },
   { "glNormal3bv", 10, _gloffset_Normal3bv },
   { "glNormal3d", 10, _gloffset_Normal3d },
   { "glNormal3dv", 10, _gloffset_Normal3dv },
   { "glNormal3f", 10, _gloffset_Normal3f },
   { "glNormal3fv", 10, _gloffset_Normal3fv },
   { "glNormal3i", 10, _gloffset_Normal3i },
   { "glNormal3iv", 10, _gloffset_Normal3iv },
   { "glNormal3s", 10, _gloffset_Normal3s },
   { "glNormal3sv", 10, _gloffset_Normal3sv },
   { "glRasterPos2d", 10, _gloffset_RasterPos2d },
   { "glRasterPos2dv", 10, _gloffset_RasterPos2dv },
   { "glRasterPos2f", 10, _gloffset_RasterPos2f },
   { "glRasterPos2fv", 10, _gloffset_RasterPos2fv },
   { "glRasterPos2i", 10, _gloffset_RasterPos2i },
   { "glRasterPos2iv", 10, _gloffset_RasterPos2iv },
   { "glRasterPos2s", 10, _gloffset_RasterPos2s },
   { "glRasterPos2sv", 10, _gloffset_RasterPos2sv },
   { "glRasterPos3d", 10, _gloffset_RasterPos3d },
   { "glRasterPos3dv", 10, _gloffset_RasterPos3dv },
   { "glRasterPos3f", 10, _gloffset_RasterPos3f },
   { "glRasterPos3fv", 10, _gloffset_RasterPos3fv },
   { "glRasterPos3i", 10, _gloffset_RasterPos3i },
   { "glRasterPos3iv", 10, _gloffset_RasterPos3iv },
   { "glRasterPos3s", 10, _gloffset_RasterPos3s },
   { "glRasterPos3sv", 10, _gloffset_RasterPos3sv },
   { "glRasterPos4d", 10, _gloffset_RasterPos4d },
   { "glRasterPos4dv", 10, _gloffset_RasterPos4dv },
   { "glRasterPos4f", 10, _gloffset_RasterPos4f },
   { "glRasterPos4fv", 10, _gloffset_RasterPos4fv },
   { "glRasterPos4i", 10, _gloffset_RasterPos4i },
   { "glRasterPos4iv", 10, _gloffset_RasterPos4iv },
   { "glRasterPos4s", 10, _gloffset_RasterPos4s },
   { "glRasterPos4sv", 10, _gloffset_RasterPos4sv },
   { "glRectd", 10, _gloffset_Rectd },
   { "glRectdv", 10, _gloffset_Rectdv },
   { "glRectf", 10, _gloffset_Rectf },
   { "glRectfv", 10, _gloffset_Rectfv },
   { "glRecti", 10, _gloffset_Recti },
   { "glRectiv", 10, _gloffset_Rectiv },
   { "glRects", 10, _gloffset_Rects },
   { "glRectsv", 10, _gloffset_Rectsv },
   { "glTexCoord1d", 10, _gloffset_TexCoord1d },
   { "glTexCoord1dv", 10, _gloffset_TexCoord1dv },
   { "glTexCoord1f", 10, _gloffset_TexCoord1f },
   { "glTexCoord1fv", 10, _gloffset_TexCoord1fv },
   { "glTexCoord1i", 10, _gloffset_TexCoord1i },
   { "glTexCoord1iv", 10, _gloffset_TexCoord1iv },
   { "glTexCoord1s", 10, _gloffset_TexCoord1s },
   { "glTexCoord1sv", 10, _gloffset_TexCoord1sv },
   { "glTexCoord2d", 10, _gloffset_TexCoord2d },
   { "glTexCoord2dv", 10, _gloffset_TexCoord2dv },
   { "glTexCoord2f", 10, _gloffset_TexCoord2f },
   { "glTexCoord2fv", 10, _gloffset_TexCoord2fv },
   { "glTexCoord2i", 10, _gloffset_TexCoord2i },
   { "glTexCoord2iv", 10, _gloffset_TexCoord2iv },
   { "glTexCoord2s", 10, _gloffset_TexCoord2s },
   { "glTexCoord2sv", 10, _gloffset_TexCoord2sv },
   { "glTexCoord3d", 10, _gloffset_TexCoord3d },
   { "glTexCoord3dv", 10, _gloffset_TexCoord3dv },
   { "glTexCoord3f", 10, _gloffset_TexCoord3f },
   { "glTexCoord3fv", 10, _gloffset_TexCoord3fv },
   { "glTexCoord3i", 10, _gloffset_TexCoord3i },
   { "glTexCoord3iv", 10, _gloffset_TexCoord3iv },
   { "glTexCoord3s", 10, _gloffset_TexCoord3s },
   { "glTexCoord3sv", 10, _gloffset_TexCoord3sv },
   { "glTexCoord4d", 10, _gloffset_TexCoord4d },
   { "glTexCoord4dv", 10, _gloffset_TexCoord4dv },
   { "glTexCoord4f", 10, _gloffset_TexCoord4f },
   { "glTexCoord4fv", 10, _gloffset_TexCoord4fv },
   { "glTexCoord4i", 10, _gloffset_TexCoord4i },
   { "glTexCoord4iv", 10, _gloffset_TexCoord4iv },
   { "glTexCoord4s", 10, _gloffset_TexCoord4s },
   { "glTexCoord4sv", 10, _gloffset_TexCoord4sv },
   { "glVertex2d", 10, _gloffset_Vertex2d },
   { "glVertex2dv", 10, _gloffset_Vertex2dv },
   { "glVertex2f", 10, _gloffset_Vertex2f },
   { "glVertex2fv", 10, _gloffset_Vertex2fv },
   { "glVertex2i", 10, _gloffset_Vertex2i },
   { "glVertex2iv", 10, _gloffset_Vertex2iv },
   { "glVertex2s", 10, _gloffset_Vertex2s },
   { "glVertex2sv", 10, _gloffset_Vertex2sv },
   { "glVertex3d", 10, _gloffset_Vertex3d },
   { "glVertex3dv", 10, _gloffset_Vertex3dv },
   { "glVertex3f", 10, _gloffset_Vertex3f },
   { "glVertex3fv", 10, _gloffset_Vertex3fv },
   { "glVertex3i", 10, _gloffset_Vertex3i },
   { "glVertex3iv", 10, _gloffset_Vertex3iv },
   { "glVertex3s", 10, _gloffset_Vertex3s },
   { "glVertex3sv", 10, _gloffset_Vertex3sv },
   { "glVertex4d", 10, _gloffset_Vertex4d },
   { "glVertex4dv", 10, _gloffset_Vertex4dv },
   { "glVertex4f", 10, _gloffset_Vertex4f },
   { "glVertex4fv", 10, _gloffset_Vertex4fv },
   { "glVertex4i", 10, _gloffset_Vertex4i },
   { "glVertex4iv", 10, _gloffset_Vertex4iv },
   { "glVertex4s", 10, _gloffset_Vertex4s },
   { "glVertex4sv", 10, _gloffset_Vertex4sv },
   { "glClipPlane", 10, _gloffset_ClipPlane },
   { "glColorMaterial", 10, _gloffset_ColorMaterial },
   { "glFogf", 10, _gloffset_Fogf },
   { "glFogfv", 10, _gloffset_Fogfv },
   { "glFogi", 10, _gloffset_Fogi },
   { "glFogiv", 10, _gloffset_Fogiv },
   { "glLightf", 10, _gloffset_Lightf },
   { "glLightfv", 10, _gloffset_Lightfv },
   { "glLighti", 10, _gloffset_Lighti },
   { "glLightiv", 10, _gloffset_Lightiv },
   { "glLightModelf", 10, _gloffset_LightModelf },
   { "glLightModelfv", 10, _gloffset_LightModelfv },
   { "glLightModeli", 10, _gloffset_LightModeli },
   { "glLightModeliv", 10, _gloffset_LightModeliv },
   { "glLineStipple", 10, _gloffset_LineStipple },
   { "glMaterialf", 10, _gloffset_Materialf },
   { "glMaterialfv", 10, _gloffset_Materialfv },
   { "glMateriali", 10, _gloffset_Materiali },
   { "glMaterialiv", 10, _gloffset_Materialiv },
   { "glPolygonStipple", 10, _gloffset_PolygonStipple },
   { "glShadeModel", 10, _gloffset_ShadeModel },
   { "glTexEnvf", 10, _gloffset_TexEnvf },
   { "glTexEnvfv", 10, _gloffset_TexEnvfv },
   { "glTexEnvi", 10, _gloffset_TexEnvi },
   { "glTexEnviv", 10, _gloffset_TexEnviv },
   { "glTexGend", 10, _gloffset_TexGend },
   { "glTexGendv", 10, _gloffset_TexGendv },
   { "glTexGenf", 10, _gloffset_TexGenf },
   { "glTexGenfv", 10, _gloffset_TexGenfv },
   { "glTexGeni", 10, _gloffset_TexGeni },
   { "glTexGeniv", 10, _gloffset_TexGeniv },
   { "glFeedbackBuffer", 10, _gloffset_FeedbackBuffer },
   { "glSelectBuffer", 10, _gloffset_SelectBuffer },
   { "glRenderMode", 10, _gloffset_RenderMode },
   { "glInitNames", 10, _gloffset_InitNames },
   { "glLoadName", 10, _gloffset_LoadName },
   { "glPassThrough", 10, _gloffset_PassThrough },
   { "glPopName", 10, _gloffset_PopName },
   { "glPushName", 10, _gloffset_PushName },
   { "glClearAccum", 10, _gloffset_ClearAccum },
   { "glClearIndex", 10, _gloffset_ClearIndex },
   { "glIndexMask", 10, _gloffset_IndexMask },
   { "glAccum", 10, _gloffset_Accum },
   { "glPopAttrib", 10, _gloffset_PopAttrib },
   { "glPushAttrib", 10, _gloffset_PushAttrib },
   { "glMap1d", 10, _gloffset_Map1d },
   { "glMap1f", 10, _gloffset_Map1f },
   { "glMap2d", 10, _gloffset_Map2d },
   { "glMap2f", 10, _gloffset_Map2f },
   { "glMapGrid1d", 10, _gloffset_MapGrid1d },
   { "glMapGrid1f", 10, _gloffset_MapGrid1f },
   { "glMapGrid2d", 10, _gloffset_MapGrid2d },
   { "glMapGrid2f", 10, _gloffset_MapGrid2f },
   { "glEvalCoord1d", 10, _gloffset_EvalCoord1d },
   { "glEvalCoord1dv", 10, _gloffset_EvalCoord1dv },
   { "glEvalCoord1f", 10, _gloffset_EvalCoord1f },
   { "glEvalCoord1fv", 10, _gloffset_EvalCoord1fv },
   { "glEvalCoord2d", 10, _gloffset_EvalCoord2d },
   { "glEvalCoord2dv", 10, _gloffset_EvalCoord2dv },
   { "glEvalCoord2f", 10, _gloffset_EvalCoord2f },
   { "glEvalCoord2fv", 10, _gloffset_EvalCoord2fv },
   { "glEvalMesh1", 10, _gloffset_EvalMesh1 },
   { "glEvalPoint1", 10, _gloffset_EvalPoint1 },
   { "glEvalMesh2", 10, _gloffset_EvalMesh2 },
   { "glEvalPoint2", 10, _gloffset_EvalPoint2 },
   { "glAlphaFunc", 10, _gloffset_AlphaFunc },
   { "glPixelZoom", 10, _gloffset_PixelZoom },
   { "glPixelTransferf", 10, _gloffset_PixelTransferf },
   { "glPixelTransferi", 10, _gloffset_PixelTransferi },
   { "glPixelMapfv", 10, _gloffset_PixelMapfv },
   { "glPixelMapuiv", 10, _gloffset_PixelMapuiv },
   { "glPixelMapusv", 10, _gloffset_PixelMapusv },
   { "glCopyPixels", 10, _gloffset_CopyPixels },
   { "glDrawPixels", 10, _gloffset_DrawPixels },
   { "glGetClipPlane", 10, _gloffset_GetClipPlane },
   { "glGetLightfv", 10, _gloffset_GetLightfv },
   { "glGetLightiv", 10, _gloffset_GetLightiv },
   { "glGetMapdv", 10, _gloffset_GetMapdv },
   { "glGetMapfv", 10, _gloffset_GetMapfv },
   { "glGetMapiv", 10, _gloffset_GetMapiv },
   { "glGetMaterialfv", 10, _gloffset_GetMaterialfv },
   { "glGetMaterialiv", 10, _gloffset_GetMaterialiv },
   { "glGetPixelMapfv", 10, _gloffset_GetPixelMapfv },
   { "glGetPixelMapuiv", 10, _gloffset_GetPixelMapuiv },
   { "glGetPixelMapusv", 10, _gloffset_GetPixelMapusv },
   { "glGetPolygonStipple", 10, _gloffset_GetPolygonStipple },
   { "glGetTexEnvfv", 10, _gloffset_GetTexEnvfv },
   { "glGetTexEnviv", 10, _gloffset_GetTexEnviv },
   { "glGetTexGendv", 10, _gloffset_GetTexGendv },
   { "glGetTexGenfv", 10, _gloffset_GetTexGenfv },
   { "glGetTexGeniv", 10, _gloffset_GetTexGeniv },
   { "glIsList", 10, _gloffset_IsList },
   { "glFrustum", 10, _gloffset_Frustum },
   { "glLoadIdentity", 10, _gloffset_LoadIdentity },
   { "glLoadMatrixf", 10, _gloffset_LoadMatrixf },
   { "glLoadMatrixd", 10, _gloffset_LoadMatrixd },
   { "glMatrixMode", 10, _gloffset_MatrixMode },
   { "glMultMatrixf", 10, _gloffset_MultMatrixf },
   { "glMultMatrixd", 10, _gloffset_MultMatrixd },
   { "glOrtho", 10, _gloffset_Ortho },
   { "glPopMatrix", 10, _gloffset_PopMatrix },
   { "glPushMatrix", 10, _gloffset_PushMatrix },
   { "glRotated", 10, _gloffset_Rotated },
   { "glRotatef", 10, _gloffset_Rotatef },
   { "glScaled", 10, _gloffset_Scaled },
   { "glScalef", 10, _gloffset_Scalef },
   { "glTranslated", 10, _gloffset_Translated },
   { "glTranslatef", 10, _gloffset_Translatef },
   { "glArrayElement", 10, _gloffset_ArrayElement },
   { "glColorPointer", 10, _gloffset_ColorPointer },
   { "glDisableClientState", 10, _gloffset_DisableClientState },
   { "glEdgeFlagPointer", 10, _gloffset_EdgeFlagPointer },
   { "glEnableClientState", 10, _gloffset_EnableClientState },
   { "glIndexPointer", 10, _gloffset_IndexPointer },
   { "glInterleavedArrays", 10, _gloffset_InterleavedArrays },
   { "glNormalPointer", 10, _gloffset_NormalPointer },
   { "glTexCoordPointer", 10, _gloffset_TexCoordPointer },
   { "glVertexPointer", 10, _gloffset_VertexPointer },
   { "glAreTexturesResident", 10, _gloffset_AreTexturesResident },
   { "glPrioritizeTextures", 10, _gloffset_PrioritizeTextures },
   { "glIndexub", 10, _gloffset_Indexub },
   { "glIndexubv", 10, _gloffset_Indexubv },
   { "glPopClientAttrib", 10, _gloffset_PopClientAttrib },
   { "glPushClientAttrib", 10, _gloffset_PushClientAttrib },
   { "glColorTable", 10, _gloffset_ColorTable },
   { "glColorTableParameterfv", 10, _gloffset_ColorTableParameterfv },
   { "glColorTableParameteriv", 10, _gloffset_ColorTableParameteriv },
   { "glCopyColorTable", 10, _gloffset_CopyColorTable },
   { "glGetColorTable", 10, _gloffset_GetColorTable },
   { "glGetColorTableParameterfv", 10, _gloffset_GetColorTableParameterfv },
   { "glGetColorTableParameteriv", 10, _gloffset_GetColorTableParameteriv },
   { "glColorSubTable", 10, _gloffset_ColorSubTable },
   { "glCopyColorSubTable", 10, _gloffset_CopyColorSubTable },
   { "glConvolutionFilter1D", 10, _gloffset_ConvolutionFilter1D },
   { "glConvolutionFilter2D", 10, _gloffset_ConvolutionFilter2D },
   { "glConvolutionParameterf", 10, _gloffset_ConvolutionParameterf },
   { "glConvolutionParameterfv", 10, _gloffset_ConvolutionParameterfv },
   { "glConvolutionParameteri", 10, _gloffset_ConvolutionParameteri },
   { "glConvolutionParameteriv", 10, _gloffset_ConvolutionParameteriv },
   { "glCopyConvolutionFilter1D", 10, _gloffset_CopyConvolutionFilter1D },
   { "glCopyConvolutionFilter2D", 10, _gloffset_CopyConvolutionFilter2D },
   { "glGetConvolutionFilter", 10, _gloffset_GetConvolutionFilter },
   { "glGetConvolutionParameterfv", 10, _gloffset_GetConvolutionParameterfv },
   { "glGetConvolutionParameteriv", 10, _gloffset_GetConvolutionParameteriv },
   { "glGetSeparableFilter", 10, _gloffset_GetSeparableFilter },
   { "glSeparableFilter2D", 10, _gloffset_SeparableFilter2D },
   { "glGetHistogram", 10, _gloffset_GetHistogram },
   { "glGetHistogramParameterfv", 10, _gloffset_GetHistogramParameterfv },
   { "glGetHistogramParameteriv", 10, _gloffset_GetHistogramParameteriv },
   { "glGetMinmax", 10, _gloffset_GetMinmax },
   { "glGetMinmaxParameterfv", 10, _gloffset_GetMinmaxParameterfv },
   { "glGetMinmaxParameteriv", 10, _gloffset_GetMinmaxParameteriv },
   { "glHistogram", 10, _gloffset_Histogram },
   { "glMinmax", 10, _gloffset_Minmax },
   { "glResetHistogram", 10, _gloffset_ResetHistogram },
   { "glResetMinmax", 10, _gloffset_ResetMinmax },
   { "glClientActiveTexture", 10, _gloffset_ClientActiveTexture },
   { "glMultiTexCoord1d", 10, _gloffset_MultiTexCoord1d },
   { "glMultiTexCoord1dv", 10, _gloffset_MultiTexCoord1dv },
   { "glMultiTexCoord1f", 10, _gloffset_MultiTexCoord1fARB },
   { "glMultiTexCoord1fv", 10, _gloffset_MultiTexCoord1fvARB },
   { "glMultiTexCoord1i", 10, _gloffset_MultiTexCoord1i },
   { "glMultiTexCoord1iv", 10, _gloffset_MultiTexCoord1iv },
   { "glMultiTexCoord1s", 10, _gloffset_MultiTexCoord1s },
   { "glMultiTexCoord1sv", 10, _gloffset_MultiTexCoord1sv },
   { "glMultiTexCoord2d", 10, _gloffset_MultiTexCoord2d },
   { "glMultiTexCoord2dv", 10, _gloffset_MultiTexCoord2dv },
   { "glMultiTexCoord2f", 10, _gloffset_MultiTexCoord2fARB },
   { "glMultiTexCoord2fv", 10, _gloffset_MultiTexCoord2fvARB },
   { "glMultiTexCoord2i", 10, _gloffset_MultiTexCoord2i },
   { "glMultiTexCoord2iv", 10, _gloffset_MultiTexCoord2iv },
   { "glMultiTexCoord2s", 10, _gloffset_MultiTexCoord2s },
   { "glMultiTexCoord2sv", 10, _gloffset_MultiTexCoord2sv },
   { "glMultiTexCoord3d", 10, _gloffset_MultiTexCoord3d },
   { "glMultiTexCoord3dv", 10, _gloffset_MultiTexCoord3dv },
   { "glMultiTexCoord3f", 10, _gloffset_MultiTexCoord3fARB },
   { "glMultiTexCoord3fv", 10, _gloffset_MultiTexCoord3fvARB },
   { "glMultiTexCoord3i", 10, _gloffset_MultiTexCoord3i },
   { "glMultiTexCoord3iv", 10, _gloffset_MultiTexCoord3iv },
   { "glMultiTexCoord3s", 10, _gloffset_MultiTexCoord3s },
   { "glMultiTexCoord3sv", 10, _gloffset_MultiTexCoord3sv },
   { "glMultiTexCoord4d", 10, _gloffset_MultiTexCoord4d },
   { "glMultiTexCoord4dv", 10, _gloffset_MultiTexCoord4dv },
   { "glMultiTexCoord4f", 10, _gloffset_MultiTexCoord4fARB },
   { "glMultiTexCoord4fv", 10, _gloffset_MultiTexCoord4fvARB },
   { "glMultiTexCoord4i", 10, _gloffset_MultiTexCoord4i },
   { "glMultiTexCoord4iv", 10, _gloffset_MultiTexCoord4iv },
   { "glMultiTexCoord4s", 10, _gloffset_MultiTexCoord4s },
   { "glMultiTexCoord4sv", 10, _gloffset_MultiTexCoord4sv },
   { "glLoadTransposeMatrixf", 10, -1 },
   { "glLoadTransposeMatrixd", 10, -1 },
   { "glMultTransposeMatrixf", 10, -1 },
   { "glMultTransposeMatrixd", 10, -1 },
   { "glFogCoordf", 10, -1 },
   { "glFogCoordfv", 10, -1 },
   { "glFogCoordd", 10, -1 },
   { "glFogCoorddv", 10, -1 },
   { "glFogCoordPointer", 10, -1 },
   { "glSecondaryColor3b", 10, -1 },
   { "glSecondaryColor3bv", 10, -1 },
   { "glSecondaryColor3d", 10, -1 },
   { "glSecondaryColor3dv", 10, -1 },
   { "glSecondaryColor3f", 10, -1 },
   { "glSecondaryColor3fv", 10, -1 },
   { "glSecondaryColor3i", 10, -1 },
   { "glSecondaryColor3iv", 10, -1 },
   { "glSecondaryColor3s", 10, -1 },
   { "glSecondaryColor3sv", 10, -1 },
   { "glSecondaryColor3ub", 10, -1 },
   { "glSecondaryColor3ubv", 10, -1 },
   { "glSecondaryColor3ui", 10, -1 },
   { "glSecondaryColor3uiv", 10, -1 },
   { "glSecondaryColor3us", 10, -1 },
   { "glSecondaryColor3usv", 10, -1 },
   { "glSecondaryColorPointer", 10, -1 },
   { "glWindowPos2d", 10, -1 },
   { "glWindowPos2dv", 10, -1 },
   { "glWindowPos2f", 10, -1 },
   { "glWindowPos2fv", 10, -1 },
   { "glWindowPos2i", 10, -1 },
   { "glWindowPos2iv", 10, -1 },
   { "glWindowPos2s", 10, -1 },
   { "glWindowPos2sv", 10, -1 },
   { "glWindowPos3d", 10, -1 },
   { "glWindowPos3dv", 10, -1 },
   { "glWindowPos3f", 10, -1 },
   { "glWindowPos3fv", 10, -1 },
   { "glWindowPos3i", 10, -1 },
   { "glWindowPos3iv", 10, -1 },
   { "glWindowPos3s", 10, -1 },
   { "glWindowPos3sv", 10, -1 },
   { "glProgramStringARB", 10, -1 },
   { "glProgramEnvParameter4dARB", 10, -1 },
   { "glProgramEnvParameter4dvARB", 10, -1 },
   { "glProgramEnvParameter4fARB", 10, -1 },
   { "glProgramEnvParameter4fvARB", 10, -1 },
   { "glProgramLocalParameter4dARB", 10, -1 },
   { "glProgramLocalParameter4dvARB", 10, -1 },
   { "glProgramLocalParameter4fARB", 10, -1 },
   { "glProgramLocalParameter4fvARB", 10, -1 },
   { "glGetProgramEnvParameterdvARB", 10, -1 },
   { "glGetProgramEnvParameterfvARB", 10, -1 },
   { "glGetProgramLocalParameterdvARB", 10, -1 },
   { "glGetProgramLocalParameterfvARB", 10, -1 },
   { "glGetProgramivARB", 10, -1 },
   { "glGetProgramStringARB", 10, -1 },
   { "glColorPointerEXT", 10, -1 },
   { "glEdgeFlagPointerEXT", 10, -1 },
   { "glIndexPointerEXT", 10, -1 },
   { "glNormalPointerEXT", 10, -1 },
   { "glTexCoordPointerEXT", 10, -1 },
   { "glVertexPointerEXT", 10, -1 },
   { "glLockArraysEXT", 10, -1 },
   { "glUnlockArraysEXT", 10, -1 },
   { "glWindowPos4dMESA", 10, -1 },
   { "glWindowPos4dvMESA", 10, -1 },
   { "glWindowPos4fMESA", 10, -1 },
   { "glWindowPos4fvMESA", 10, -1 },
   { "glWindowPos4iMESA", 10, -1 },
   { "glWindowPos4ivMESA", 10, -1 },
   { "glWindowPos4sMESA", 10, -1 },
   { "glWindowPos4svMESA", 10, -1 },
   { "glBindProgramNV", 10, -1 },
   { "glDeleteProgramsNV", 10, -1 },
   { "glGenProgramsNV", 10, -1 },
   { "glIsProgramNV", 10, -1 },
   { "glVertexAttrib1sNV", 10, -1 },
   { "glVertexAttrib1svNV", 10, -1 },
   { "glVertexAttrib2sNV", 10, -1 },
   { "glVertexAttrib2svNV", 10, -1 },
   { "glVertexAttrib3sNV", 10, -1 },
   { "glVertexAttrib3svNV", 10, -1 },
   { "glVertexAttrib4sNV", 10, -1 },
   { "glVertexAttrib4svNV", 10, -1 },
   { "glVertexAttrib1fNV", 10, -1 },
   { "glVertexAttrib1fvNV", 10, -1 },
   { "glVertexAttrib2fNV", 10, -1 },
   { "glVertexAttrib2fvNV", 10, -1 },
   { "glVertexAttrib3fNV", 10, -1 },
   { "glVertexAttrib3fvNV", 10, -1 },
   { "glVertexAttrib4fNV", 10, -1 },
   { "glVertexAttrib4fvNV", 10, -1 },
   { "glVertexAttrib1dNV", 10, -1 },
   { "glVertexAttrib1dvNV", 10, -1 },
   { "glVertexAttrib2dNV", 10, -1 },
   { "glVertexAttrib2dvNV", 10, -1 },
   { "glVertexAttrib3dNV", 10, -1 },
   { "glVertexAttrib3dvNV", 10, -1 },
   { "glVertexAttrib4dNV", 10, -1 },
   { "glVertexAttrib4dvNV", 10, -1 },
   { "glVertexAttrib4ubNV", 10, -1 },
   { "glVertexAttrib4ubvNV", 10, -1 },
   { "glVertexAttribs1svNV", 10, -1 },
   { "glVertexAttribs2svNV", 10, -1 },
   { "glVertexAttribs3svNV", 10, -1 },
   { "glVertexAttribs4svNV", 10, -1 },
   { "glVertexAttribs1fvNV", 10, -1 },
   { "glVertexAttribs2fvNV", 10, -1 },
   { "glVertexAttribs3fvNV", 10, -1 },
   { "glVertexAttribs4fvNV", 10, -1 },
   { "glVertexAttribs1dvNV", 10, -1 },
   { "glVertexAttribs2dvNV", 10, -1 },
   { "glVertexAttribs3dvNV", 10, -1 },
   { "glVertexAttribs4dvNV", 10, -1 },
   { "glVertexAttribs4ubvNV", 10, -1 },
   { "glGenFragmentShadersATI", 10, -1 },
   { "glBindFragmentShaderATI", 10, -1 },
   { "glDeleteFragmentShaderATI", 10, -1 },
   { "glBeginFragmentShaderATI", 10, -1 },
   { "glEndFragmentShaderATI", 10, -1 },
   { "glPassTexCoordATI", 10, -1 },
   { "glSampleMapATI", 10, -1 },
   { "glColorFragmentOp1ATI", 10, -1 },
   { "glColorFragmentOp2ATI", 10, -1 },
   { "glColorFragmentOp3ATI", 10, -1 },
   { "glAlphaFragmentOp1ATI", 10, -1 },
   { "glAlphaFragmentOp2ATI", 10, -1 },
   { "glAlphaFragmentOp3ATI", 10, -1 },
   { "glSetFragmentShaderConstantATI", 10, -1 },
   { "glActiveStencilFaceEXT", 10, -1 },
   { "glStencilFuncSeparateATI", 10, -1 },
   { "glProgramEnvParameters4fvEXT", 10, -1 },
   { "glProgramLocalParameters4fvEXT", 10, -1 },
   { "glPrimitiveRestartNV", 10, -1 },

   { NULL, 0, -1 }
};

const struct function gl_core_functions_possible[] = {
   /* GL_ARB_ES3_2_compatibility */
   { "glPrimitiveBoundingBoxARB", 45, -1 },

   /* GL_ARB_gpu_shader_int64 */
   { "glUniform1i64ARB", 45, -1 },
   { "glUniform2i64ARB", 45, -1 },
   { "glUniform3i64ARB", 45, -1 },
   { "glUniform4i64ARB", 45, -1 },
   { "glUniform1ui64ARB", 45, -1 },
   { "glUniform2ui64ARB", 45, -1 },
   { "glUniform3ui64ARB", 45, -1 },
   { "glUniform4ui64ARB", 45, -1 },
   { "glUniform1i64vARB", 45, -1 },
   { "glUniform2i64vARB", 45, -1 },
   { "glUniform3i64vARB", 45, -1 },
   { "glUniform4i64vARB", 45, -1 },
   { "glUniform1ui64vARB", 45, -1 },
   { "glUniform2ui64vARB", 45, -1 },
   { "glUniform3ui64vARB", 45, -1 },
   { "glUniform4ui64vARB", 45, -1 },
   { "glGetUniformi64vARB", 45, -1 },
   { "glGetUniformui64vARB", 45, -1 },
   { "glGetnUniformi64vARB", 45, -1 },
   { "glGetnUniformui64vARB", 45, -1 },
   { "glProgramUniform1i64ARB", 45, -1 },
   { "glProgramUniform2i64ARB", 45, -1 },
   { "glProgramUniform3i64ARB", 45, -1 },
   { "glProgramUniform4i64ARB", 45, -1 },
   { "glProgramUniform1ui64ARB", 45, -1 },
   { "glProgramUniform2ui64ARB", 45, -1 },
   { "glProgramUniform3ui64ARB", 45, -1 },
   { "glProgramUniform4ui64ARB", 45, -1 },
   { "glProgramUniform1i64vARB", 45, -1 },
   { "glProgramUniform2i64vARB", 45, -1 },
   { "glProgramUniform3i64vARB", 45, -1 },
   { "glProgramUniform4i64vARB", 45, -1 },
   { "glProgramUniform1ui64vARB", 45, -1 },
   { "glProgramUniform2ui64vARB", 45, -1 },
   { "glProgramUniform3ui64vARB", 45, -1 },
   { "glProgramUniform4ui64vARB", 45, -1 },

   /* GL_ARB_gl_spirv */
   { "glSpecializeShaderARB", 45, -1 },

   { NULL, 0, -1 }
};

const struct function gles11_functions_possible[] = {
   { "glActiveTexture", 11, _gloffset_ActiveTexture },
   { "glAlphaFunc", 11, _gloffset_AlphaFunc },
   { "glAlphaFuncx", 11, -1 },
   { "glBindBuffer", 11, -1 },
   { "glBindFramebufferOES", 11, -1 },
   { "glBindRenderbufferOES", 11, -1 },
   { "glBindTexture", 11, _gloffset_BindTexture },
   { "glBlendEquationOES", 11, _gloffset_BlendEquation },
   { "glBlendEquationSeparateOES", 11, -1 },
   { "glBlendFunc", 11, _gloffset_BlendFunc },
   { "glBlendFuncSeparateOES", 11, -1 },
   { "glBufferData", 11, -1 },
   { "glBufferSubData", 11, -1 },
   { "glCheckFramebufferStatusOES", 11, -1 },
   { "glClear", 11, _gloffset_Clear },
   { "glClearColor", 11, _gloffset_ClearColor },
   { "glClearColorx", 11, -1 },
   { "glClearDepthf", 11, -1 },
   { "glClearDepthx", 11, -1 },
   { "glClearStencil", 11, _gloffset_ClearStencil },
   { "glClientActiveTexture", 11, _gloffset_ClientActiveTexture },
   { "glClipPlanef", 11, -1 },
   { "glClipPlanex", 11, -1 },
   { "glColor4f", 11, _gloffset_Color4f },
   { "glColor4ub", 11, _gloffset_Color4ub },
   { "glColor4x", 11, -1 },
   { "glColorMask", 11, _gloffset_ColorMask },
   { "glColorPointer", 11, _gloffset_ColorPointer },
   { "glCompressedTexImage2D", 11, -1 },
   { "glCompressedTexSubImage2D", 11, -1 },
   { "glCopyTexImage2D", 11, _gloffset_CopyTexImage2D },
   { "glCopyTexSubImage2D", 11, _gloffset_CopyTexSubImage2D },
   { "glCullFace", 11, _gloffset_CullFace },
   { "glDeleteBuffers", 11, -1 },
   { "glDeleteFramebuffersOES", 11, -1 },
   { "glDeleteRenderbuffersOES", 11, -1 },
   { "glDeleteTextures", 11, _gloffset_DeleteTextures },
   { "glDepthFunc", 11, _gloffset_DepthFunc },
   { "glDepthMask", 11, _gloffset_DepthMask },
   { "glDepthRangef", 11, -1 },
   { "glDepthRangex", 11, -1 },
   { "glDisable", 11, _gloffset_Disable },
   { "glDiscardFramebufferEXT", 11, -1 },
   { "glDisableClientState", 11, _gloffset_DisableClientState },
   { "glDrawArrays", 11, _gloffset_DrawArrays },
   { "glDrawElements", 11, _gloffset_DrawElements },
   { "glDrawTexfOES", 11, -1 },
   { "glDrawTexfvOES", 11, -1 },
   { "glDrawTexiOES", 11, -1 },
   { "glDrawTexivOES", 11, -1 },
   { "glDrawTexsOES", 11, -1 },
   { "glDrawTexsvOES", 11, -1 },
   { "glDrawTexxOES", 11, -1 },
   { "glDrawTexxvOES", 11, -1 },
   { "glEGLImageTargetRenderbufferStorageOES", 11, -1 },
   { "glEGLImageTargetTexture2DOES", 11, -1 },
   { "glEnable", 11, _gloffset_Enable },
   { "glEnableClientState", 11, _gloffset_EnableClientState },
   { "glFinish", 11, _gloffset_Finish },
   { "glFlush", 11, _gloffset_Flush },
   { "glFlushMappedBufferRangeEXT", 11, -1 },
   { "glFogf", 11, _gloffset_Fogf },
   { "glFogfv", 11, _gloffset_Fogfv },
   { "glFogx", 11, -1 },
   { "glFogxv", 11, -1 },
   { "glFramebufferRenderbufferOES", 11, -1 },
   { "glFramebufferTexture2DOES", 11, -1 },
   { "glFrontFace", 11, _gloffset_FrontFace },
   { "glFrustumf", 11, -1 },
   { "glFrustumx", 11, -1 },
   { "glGenBuffers", 11, -1 },
   { "glGenFramebuffersOES", 11, -1 },
   { "glGenRenderbuffersOES", 11, -1 },
   { "glGenTextures", 11, _gloffset_GenTextures },
   { "glGenerateMipmapOES", 11, -1 },
   { "glGetBooleanv", 11, _gloffset_GetBooleanv },
   { "glGetBufferParameteriv", 11, -1 },
   { "glGetBufferPointervOES", 11, -1 },
   { "glGetClipPlanef", 11, -1 },
   { "glGetClipPlanex", 11, -1 },
   { "glGetError", 11, _gloffset_GetError },
   { "glGetFixedv", 11, -1 },
   { "glGetFloatv", 11, _gloffset_GetFloatv },
   { "glGetFramebufferAttachmentParameterivOES", 11, -1 },
   { "glGetIntegerv", 11, _gloffset_GetIntegerv },
   { "glGetLightfv", 11, _gloffset_GetLightfv },
   { "glGetLightxv", 11, -1 },
   { "glGetMaterialfv", 11, _gloffset_GetMaterialfv },
   { "glGetMaterialxv", 11, -1 },
   // We check for the aliased -KHR version in GLES 1.1
// { "glGetPointerv", 11, _gloffset_GetPointerv },
   { "glGetRenderbufferParameterivOES", 11, -1 },
   { "glGetString", 11, _gloffset_GetString },
   { "glGetTexEnvfv", 11, _gloffset_GetTexEnvfv },
   { "glGetTexEnviv", 11, _gloffset_GetTexEnviv },
   { "glGetTexEnvxv", 11, -1 },
   { "glGetTexGenfvOES", 11, _gloffset_GetTexGenfv },
   { "glGetTexGenivOES", 11, _gloffset_GetTexGeniv },
   { "glGetTexGenxvOES", 11, -1 },
   { "glGetTexParameterfv", 11, _gloffset_GetTexParameterfv },
   { "glGetTexParameteriv", 11, _gloffset_GetTexParameteriv },
   { "glGetTexParameterxv", 11, -1 },
   { "glHint", 11, _gloffset_Hint },
   { "glIsBuffer", 11, -1 },
   { "glIsEnabled", 11, _gloffset_IsEnabled },
   { "glIsFramebufferOES", 11, -1 },
   { "glIsRenderbufferOES", 11, -1 },
   { "glIsTexture", 11, _gloffset_IsTexture },
   { "glLightModelf", 11, _gloffset_LightModelf },
   { "glLightModelfv", 11, _gloffset_LightModelfv },
   { "glLightModelx", 11, -1 },
   { "glLightModelxv", 11, -1 },
   { "glLightf", 11, _gloffset_Lightf },
   { "glLightfv", 11, _gloffset_Lightfv },
   { "glLightx", 11, -1 },
   { "glLightxv", 11, -1 },
   { "glLineWidth", 11, _gloffset_LineWidth },
   { "glLineWidthx", 11, -1 },
   { "glLoadIdentity", 11, _gloffset_LoadIdentity },
   { "glLoadMatrixf", 11, _gloffset_LoadMatrixf },
   { "glLoadMatrixx", 11, -1 },
   { "glLogicOp", 11, _gloffset_LogicOp },
   { "glMapBufferOES", 11, -1 },
   { "glMapBufferRangeEXT", 11, -1 },
   { "glMaterialf", 11, _gloffset_Materialf },
   { "glMaterialfv", 11, _gloffset_Materialfv },
   { "glMaterialx", 11, -1 },
   { "glMaterialxv", 11, -1 },
   { "glMatrixMode", 11, _gloffset_MatrixMode },
   { "glMultMatrixf", 11, _gloffset_MultMatrixf },
   { "glMultMatrixx", 11, -1 },
   { "glMultiDrawArraysEXT", 11, -1 },
   { "glMultiDrawElementsEXT", 11, -1 },
   { "glMultiTexCoord4f", 11, _gloffset_MultiTexCoord4fARB },
   { "glMultiTexCoord4x", 11, -1 },
   { "glNormal3f", 11, _gloffset_Normal3f },
   { "glNormal3x", 11, -1 },
   { "glNormalPointer", 11, _gloffset_NormalPointer },
   { "glOrthof", 11, -1 },
   { "glOrthox", 11, -1 },
   { "glPixelStorei", 11, _gloffset_PixelStorei },
   { "glPointParameterf", 11, -1 },
   { "glPointParameterfv", 11, -1 },
   { "glPointParameterx", 11, -1 },
   { "glPointParameterxv", 11, -1 },
   { "glPointSize", 11, _gloffset_PointSize },
   { "glPointSizePointerOES", 11, -1 },
   { "glPointSizex", 11, -1 },
   { "glPolygonOffset", 11, _gloffset_PolygonOffset },
   { "glPolygonOffsetx", 11, -1 },
   { "glPopMatrix", 11, _gloffset_PopMatrix },
   { "glPushMatrix", 11, _gloffset_PushMatrix },
   { "glQueryMatrixxOES", 11, -1 },
   { "glReadPixels", 11, _gloffset_ReadPixels },
   { "glRenderbufferStorageOES", 11, -1 },
   { "glRotatef", 11, _gloffset_Rotatef },
   { "glRotatex", 11, -1 },
   { "glSampleCoverage", 11, -1 },
   { "glSampleCoveragex", 11, -1 },
   { "glScalef", 11, _gloffset_Scalef },
   { "glScalex", 11, -1 },
   { "glScissor", 11, _gloffset_Scissor },
   { "glShadeModel", 11, _gloffset_ShadeModel },
   { "glStencilFunc", 11, _gloffset_StencilFunc },
   { "glStencilMask", 11, _gloffset_StencilMask },
   { "glStencilOp", 11, _gloffset_StencilOp },
   { "glTexCoordPointer", 11, _gloffset_TexCoordPointer },
   { "glTexEnvf", 11, _gloffset_TexEnvf },
   { "glTexEnvfv", 11, _gloffset_TexEnvfv },
   { "glTexEnvi", 11, _gloffset_TexEnvi },
   { "glTexEnviv", 11, _gloffset_TexEnviv },
   { "glTexEnvx", 11, -1 },
   { "glTexEnvxv", 11, -1 },
   { "glTexGenfOES", 11, _gloffset_TexGenf },
   { "glTexGenfvOES", 11, _gloffset_TexGenfv },
   { "glTexGeniOES", 11, _gloffset_TexGeni },
   { "glTexGenivOES", 11, _gloffset_TexGeniv },
   { "glTexGenxOES", 11, -1 },
   { "glTexGenxvOES", 11, -1 },
   { "glTexImage2D", 11, _gloffset_TexImage2D },
   { "glTexParameterf", 11, _gloffset_TexParameterf },
   { "glTexParameterfv", 11, _gloffset_TexParameterfv },
   { "glTexParameteri", 11, _gloffset_TexParameteri },
   { "glTexParameteriv", 11, _gloffset_TexParameteriv },
   { "glTexParameterx", 11, -1 },
   { "glTexParameterxv", 11, -1 },
   { "glTexSubImage2D", 11, _gloffset_TexSubImage2D },
   { "glTranslatef", 11, _gloffset_Translatef },
   { "glTranslatex", 11, -1 },
   { "glUnmapBufferOES", 11, -1 },
   { "glVertexPointer", 11, _gloffset_VertexPointer },
   { "glViewport", 11, _gloffset_Viewport },

   /* GL_KHR_debug */
   { "glPushDebugGroupKHR", 11, -1 },
   { "glPopDebugGroupKHR", 11, -1 },
   { "glDebugMessageCallbackKHR", 11, -1 },
   { "glDebugMessageControlKHR", 11, -1 },
   { "glDebugMessageInsertKHR", 11, -1 },
   { "glGetDebugMessageLogKHR", 11, -1 },
   { "glGetObjectLabelKHR", 11, -1 },
   { "glGetObjectPtrLabelKHR", 11, -1 },
   { "glGetPointervKHR", 11, _gloffset_GetPointerv },
   { "glObjectLabelKHR", 11, -1 },
   { "glObjectPtrLabelKHR", 11, -1 },

   /* GL_EXT_polygon_offset_clamp */
   { "glPolygonOffsetClampEXT", 11, -1 },

   /* GL_NV_conservative_raster */
   { "glSubpixelPrecisionBiasNV", 20, -1 },

   /* GL_NV_conservative_raster_dilate */
   { "glConservativeRasterParameterfNV", 20, -1 },

   /* GL_NV_conservative_raster_pre_snap_triangles */
   { "glConservativeRasterParameteriNV", 20, -1 },

   { NULL, 0, -1 }
};

const struct function gles2_functions_possible[] = {
   { "glActiveTexture", 20, _gloffset_ActiveTexture },
   { "glAttachShader", 20, -1 },
   { "glBindAttribLocation", 20, -1 },
   { "glBindBuffer", 20, -1 },
   { "glBindFramebuffer", 20, -1 },
   { "glBindRenderbuffer", 20, -1 },
   { "glBindTexture", 20, _gloffset_BindTexture },
   { "glBindVertexArrayOES", 20, -1 },
   { "glBlendColor", 20, _gloffset_BlendColor },
   { "glBlendEquation", 20, _gloffset_BlendEquation },
   { "glBlendEquationSeparate", 20, -1 },
   { "glBlendFunc", 20, _gloffset_BlendFunc },
   { "glBlendFuncSeparate", 20, -1 },
   { "glBufferData", 20, -1 },
   { "glBufferSubData", 20, -1 },
   { "glCheckFramebufferStatus", 20, -1 },
   { "glClear", 20, _gloffset_Clear },
   { "glClearColor", 20, _gloffset_ClearColor },
   { "glClearDepthf", 20, -1 },
   { "glClearStencil", 20, _gloffset_ClearStencil },
   { "glColorMask", 20, _gloffset_ColorMask },
   { "glCompileShader", 20, -1 },
   { "glCompressedTexImage2D", 20, -1 },
   { "glCompressedTexImage3DOES", 20, -1 },
   { "glCompressedTexSubImage2D", 20, -1 },
   { "glCompressedTexSubImage3DOES", 20, -1 },
   { "glCopyTexImage2D", 20, _gloffset_CopyTexImage2D },
   { "glCopyTexSubImage2D", 20, _gloffset_CopyTexSubImage2D },
   { "glCopyTexSubImage3DOES", 20, _gloffset_CopyTexSubImage3D },
   { "glCreateProgram", 20, -1 },
   { "glCreateShader", 20, -1 },
   { "glCullFace", 20, _gloffset_CullFace },
   { "glDeleteBuffers", 20, -1 },
   { "glDeleteFramebuffers", 20, -1 },
   { "glDeleteProgram", 20, -1 },
   { "glDeleteRenderbuffers", 20, -1 },
   { "glDeleteShader", 20, -1 },
   { "glDeleteTextures", 20, _gloffset_DeleteTextures },
   { "glDeleteVertexArraysOES", 20, -1 },
   { "glDepthFunc", 20, _gloffset_DepthFunc },
   { "glDepthMask", 20, _gloffset_DepthMask },
   { "glDepthRangef", 20, -1 },
   { "glDetachShader", 20, -1 },
   { "glDisable", 20, _gloffset_Disable },
   { "glDiscardFramebufferEXT", 20, -1 },
   { "glDisableVertexAttribArray", 20, -1 },
   { "glDrawArrays", 20, _gloffset_DrawArrays },
   { "glDrawBuffersNV", 20, -1 },
   { "glDrawElements", 20, _gloffset_DrawElements },
   { "glEGLImageTargetRenderbufferStorageOES", 20, -1 },
   { "glEGLImageTargetTexture2DOES", 20, -1 },
   { "glEnable", 20, _gloffset_Enable },
   { "glEnableVertexAttribArray", 20, -1 },
   { "glFinish", 20, _gloffset_Finish },
   { "glFlush", 20, _gloffset_Flush },
   { "glFlushMappedBufferRangeEXT", 20, -1 },
   { "glFramebufferRenderbuffer", 20, -1 },
   { "glFramebufferTexture2D", 20, -1 },
   { "glFramebufferTexture3DOES", 20, -1 },
   { "glFrontFace", 20, _gloffset_FrontFace },
   { "glGenBuffers", 20, -1 },
   { "glGenFramebuffers", 20, -1 },
   { "glGenRenderbuffers", 20, -1 },
   { "glGenTextures", 20, _gloffset_GenTextures },
   { "glGenVertexArraysOES", 20, -1 },
   { "glGenerateMipmap", 20, -1 },
   { "glGetActiveAttrib", 20, -1 },
   { "glGetActiveUniform", 20, -1 },
   { "glGetAttachedShaders", 20, -1 },
   { "glGetAttribLocation", 20, -1 },
   { "glGetBooleanv", 20, _gloffset_GetBooleanv },
   { "glGetBufferParameteriv", 20, -1 },
   { "glGetBufferPointervOES", 20, -1 },
   { "glGetError", 20, _gloffset_GetError },
   { "glGetFloatv", 20, _gloffset_GetFloatv },
   { "glGetFramebufferAttachmentParameteriv", 20, -1 },
   { "glGetIntegerv", 20, _gloffset_GetIntegerv },
   { "glGetProgramInfoLog", 20, -1 },
   { "glGetProgramiv", 20, -1 },
   { "glGetRenderbufferParameteriv", 20, -1 },
   { "glGetShaderInfoLog", 20, -1 },
   { "glGetShaderPrecisionFormat", 20, -1 },
   { "glGetShaderSource", 20, -1 },
   { "glGetShaderiv", 20, -1 },
   { "glGetString", 20, _gloffset_GetString },
   { "glGetTexParameterfv", 20, _gloffset_GetTexParameterfv },
   { "glGetTexParameteriv", 20, _gloffset_GetTexParameteriv },
   { "glGetUniformLocation", 20, -1 },
   { "glGetUniformfv", 20, -1 },
   { "glGetUniformiv", 20, -1 },
   { "glGetVertexAttribPointerv", 20, -1 },
   { "glGetVertexAttribfv", 20, -1 },
   { "glGetVertexAttribiv", 20, -1 },
   { "glHint", 20, _gloffset_Hint },
   { "glIsBuffer", 20, -1 },
   { "glIsEnabled", 20, _gloffset_IsEnabled },
   { "glIsFramebuffer", 20, -1 },
   { "glIsProgram", 20, -1 },
   { "glIsRenderbuffer", 20, -1 },
   { "glIsShader", 20, -1 },
   { "glIsTexture", 20, _gloffset_IsTexture },
   { "glIsVertexArrayOES", 20, -1 },
   { "glLineWidth", 20, _gloffset_LineWidth },
   { "glLinkProgram", 20, -1 },
   { "glMapBufferOES", 20, -1 },
   { "glMapBufferRangeEXT", 20, -1 },
   { "glMultiDrawArraysEXT", 20, -1 },
   { "glMultiDrawElementsEXT", 20, -1 },
   { "glPixelStorei", 20, _gloffset_PixelStorei },
   { "glPolygonOffset", 20, _gloffset_PolygonOffset },
   { "glReadBufferNV", 20, _gloffset_ReadBuffer },
   { "glReadPixels", 20, _gloffset_ReadPixels },
   { "glReleaseShaderCompiler", 20, -1 },
   { "glRenderbufferStorage", 20, -1 },
   { "glSampleCoverage", 20, -1 },
   { "glScissor", 20, _gloffset_Scissor },
   { "glShaderBinary", 20, -1 },
   { "glShaderSource", 20, -1 },
   { "glStencilFunc", 20, _gloffset_StencilFunc },
   { "glStencilFuncSeparate", 20, -1 },
   { "glStencilMask", 20, _gloffset_StencilMask },
   { "glStencilMaskSeparate", 20, -1 },
   { "glStencilOp", 20, _gloffset_StencilOp },
   { "glStencilOpSeparate", 20, -1 },
   { "glTexImage2D", 20, _gloffset_TexImage2D },
   { "glTexImage3DOES", 20, _gloffset_TexImage3D },
   { "glTexParameterf", 20, _gloffset_TexParameterf },
   { "glTexParameterfv", 20, _gloffset_TexParameterfv },
   { "glTexParameteri", 20, _gloffset_TexParameteri },
   { "glTexParameteriv", 20, _gloffset_TexParameteriv },
   { "glTexSubImage2D", 20, _gloffset_TexSubImage2D },
   { "glTexSubImage3DOES", 20, _gloffset_TexSubImage3D },
   { "glUniform1f", 20, -1 },
   { "glUniform1fv", 20, -1 },
   { "glUniform1i", 20, -1 },
   { "glUniform1iv", 20, -1 },
   { "glUniform2f", 20, -1 },
   { "glUniform2fv", 20, -1 },
   { "glUniform2i", 20, -1 },
   { "glUniform2iv", 20, -1 },
   { "glUniform3f", 20, -1 },
   { "glUniform3fv", 20, -1 },
   { "glUniform3i", 20, -1 },
   { "glUniform3iv", 20, -1 },
   { "glUniform4f", 20, -1 },
   { "glUniform4fv", 20, -1 },
   { "glUniform4i", 20, -1 },
   { "glUniform4iv", 20, -1 },
   { "glUniformMatrix2fv", 20, -1 },
   { "glUniformMatrix3fv", 20, -1 },
   { "glUniformMatrix4fv", 20, -1 },
   { "glUnmapBufferOES", 20, -1 },
   { "glUseProgram", 20, -1 },
   { "glValidateProgram", 20, -1 },
   { "glVertexAttrib1f", 20, -1 },
   { "glVertexAttrib1fv", 20, -1 },
   { "glVertexAttrib2f", 20, -1 },
   { "glVertexAttrib2fv", 20, -1 },
   { "glVertexAttrib3f", 20, -1 },
   { "glVertexAttrib3fv", 20, -1 },
   { "glVertexAttrib4f", 20, -1 },
   { "glVertexAttrib4fv", 20, -1 },
   { "glVertexAttribPointer", 20, -1 },
   { "glViewport", 20, _gloffset_Viewport },

   /* GL_OES_get_program_binary - Also part of OpenGL ES 3.0. */
   { "glGetProgramBinaryOES", 20, -1 },
   { "glProgramBinaryOES", 20, -1 },

   /* GL_EXT_separate_shader_objects - Also part of OpenGL ES 3.1. */
   { "glProgramParameteriEXT", 20, -1 },
   { "glUseProgramStagesEXT", 20, -1 },
   { "glActiveShaderProgramEXT", 20, -1 },
   { "glCreateShaderProgramvEXT", 20, -1 },
   { "glBindProgramPipelineEXT", 20, -1 },
   { "glDeleteProgramPipelinesEXT", 20, -1 },
   { "glGenProgramPipelinesEXT", 20, -1 },
   { "glIsProgramPipelineEXT", 20, -1 },
   { "glGetProgramPipelineivEXT", 20, -1 },
   { "glProgramUniform1iEXT", 20, -1 },
   { "glProgramUniform1ivEXT", 20, -1 },
   { "glProgramUniform1fEXT", 20, -1 },
   { "glProgramUniform1fvEXT", 20, -1 },
   { "glProgramUniform2iEXT", 20, -1 },
   { "glProgramUniform2ivEXT", 20, -1 },
   { "glProgramUniform2fEXT", 20, -1 },
   { "glProgramUniform2fvEXT", 20, -1 },
   { "glProgramUniform3iEXT", 20, -1 },
   { "glProgramUniform3ivEXT", 20, -1 },
   { "glProgramUniform3fEXT", 20, -1 },
   { "glProgramUniform3fvEXT", 20, -1 },
   { "glProgramUniform4iEXT", 20, -1 },
   { "glProgramUniform4ivEXT", 20, -1 },
   { "glProgramUniform4fEXT", 20, -1 },
   { "glProgramUniform4fvEXT", 20, -1 },
   { "glProgramUniformMatrix2fvEXT", 20, -1 },
   { "glProgramUniformMatrix3fvEXT", 20, -1 },
   { "glProgramUniformMatrix4fvEXT", 20, -1 },
   { "glProgramUniformMatrix2x3fvEXT", 20, -1 },
   { "glProgramUniformMatrix3x2fvEXT", 20, -1 },
   { "glProgramUniformMatrix2x4fvEXT", 20, -1 },
   { "glProgramUniformMatrix4x2fvEXT", 20, -1 },
   { "glProgramUniformMatrix3x4fvEXT", 20, -1 },
   { "glProgramUniformMatrix4x3fvEXT", 20, -1 },
   { "glValidateProgramPipelineEXT", 20, -1 },
   { "glGetProgramPipelineInfoLogEXT", 20, -1 },

   /* GL_AMD_performance_monitor */
   { "glGetPerfMonitorGroupsAMD", 20, -1 },
   { "glGetPerfMonitorCountersAMD", 20, -1 },
   { "glGetPerfMonitorGroupStringAMD", 20, -1 },
   { "glGetPerfMonitorCounterStringAMD", 20, -1 },
   { "glGetPerfMonitorCounterInfoAMD", 20, -1 },
   { "glGenPerfMonitorsAMD", 20, -1 },
   { "glDeletePerfMonitorsAMD", 20, -1 },
   { "glSelectPerfMonitorCountersAMD", 20, -1 },
   { "glBeginPerfMonitorAMD", 20, -1 },
   { "glEndPerfMonitorAMD", 20, -1 },
   { "glGetPerfMonitorCounterDataAMD", 20, -1 },

   /* GL_INTEL_performance_query */
   { "glGetFirstPerfQueryIdINTEL", 20, -1 },
   { "glGetNextPerfQueryIdINTEL", 20, -1 },
   { "glGetPerfQueryIdByNameINTEL", 20, -1 },
   { "glGetPerfQueryInfoINTEL", 20, -1 },
   { "glGetPerfCounterInfoINTEL", 20, -1 },
   { "glCreatePerfQueryINTEL", 20, -1 },
   { "glDeletePerfQueryINTEL", 20, -1 },
   { "glBeginPerfQueryINTEL", 20, -1 },
   { "glEndPerfQueryINTEL", 20, -1 },
   { "glGetPerfQueryDataINTEL", 20, -1 },

   /* GL_KHR_debug */
   { "glPushDebugGroupKHR", 20, -1 },
   { "glPopDebugGroupKHR", 20, -1 },
   { "glDebugMessageCallbackKHR", 20, -1 },
   { "glDebugMessageControlKHR", 20, -1 },
   { "glDebugMessageInsertKHR", 20, -1 },
   { "glGetDebugMessageLogKHR", 20, -1 },
   { "glGetObjectLabelKHR", 20, -1 },
   { "glGetObjectPtrLabelKHR", 20, -1 },
   { "glGetPointervKHR", 20, -1 },
   { "glObjectLabelKHR", 20, -1 },
   { "glObjectPtrLabelKHR", 20, -1 },

   /* GL_EXT_polygon_offset_clamp */
   { "glPolygonOffsetClampEXT", 11, -1 },

   /* GL_KHR_robustness */
   { "glGetGraphicsResetStatusKHR", 20, -1 },
   { "glReadnPixelsKHR", 20, -1 },
   { "glGetnUniformfvKHR", 20, -1 },
   { "glGetnUniformivKHR", 20, -1 },
   { "glGetnUniformuivKHR", 20, -1 },

   /* GL_KHR_blend_equation_advanced */
   { "glBlendBarrierKHR", 20, -1 },

   /* GL_EXT_occlusion_query_boolean */
   { "glGenQueriesEXT", 20, -1 },
   { "glDeleteQueriesEXT", 20, -1 },
   { "glIsQueryEXT", 20, -1 },
   { "glBeginQueryEXT", 20, -1 },
   { "glEndQueryEXT", 20, -1 },
   { "glGetQueryivEXT", 20, -1 },
   { "glGetQueryObjectivEXT", 20, -1 },
   { "glGetQueryObjectuivEXT", 20, -1 },

   /* GL_EXT_disjoint_timer_query */
   { "glGetQueryObjecti64vEXT", 20, -1 },
   { "glGetQueryObjectui64vEXT", 20, -1 },
   { "glQueryCounterEXT", 20, -1 },

   /* GL_EXT_shader_framebuffer_fetch_non_coherent */
   { "glFramebufferFetchBarrierEXT", 20, -1 },

   /* GL_NV_conditional_render */
   { "glBeginConditionalRenderNV", 20, -1 },
   { "glEndConditionalRenderNV", 20, -1 },

   /* GL_NV_conservative_raster */
   { "glSubpixelPrecisionBiasNV", 20, -1 },

   /* GL_NV_conservative_raster_dilate */
   { "glConservativeRasterParameterfNV", 20, -1 },

   /* GL_NV_conservative_raster_pre_snap_triangles */
   { "glConservativeRasterParameteriNV", 20, -1 },

   /* GL_EXT_multisampled_render_to_texture */
   { "glRenderbufferStorageMultisampleEXT", 20, -1 },
   { "glFramebufferTexture2DMultisampleEXT", 20, -1 },

   /* GL_KHR_parallel_shader_compile */
   { "glMaxShaderCompilerThreadsKHR", 20, -1 },

   { NULL, 0, -1 }
};

const struct function gles3_functions_possible[] = {
   // We check for the aliased -EXT version in GLES 2
   // { "glBeginQuery", 30, -1 },
   { "glBeginTransformFeedback", 30, -1 },
   { "glBindBufferBase", 30, -1 },
   { "glBindBufferRange", 30, -1 },
   { "glBindSampler", 30, -1 },
   { "glBindTransformFeedback", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glBindVertexArray", 30, -1 },
   { "glBlitFramebuffer", 30, -1 },
   { "glClearBufferfi", 30, -1 },
   { "glClearBufferfv", 30, -1 },
   { "glClearBufferiv", 30, -1 },
   { "glClearBufferuiv", 30, -1 },
   { "glClientWaitSync", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glCompressedTexImage3D", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glCompressedTexSubImage3D", 30, -1 },
   { "glCopyBufferSubData", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glCopyTexSubImage3D", 30, -1 },
   // We check for the aliased -EXT version in GLES 2
   // { "glDeleteQueries", 30, -1 },
   { "glDeleteSamplers", 30, -1 },
   { "glDeleteSync", 30, -1 },
   { "glDeleteTransformFeedbacks", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glDeleteVertexArrays", 30, -1 },
   { "glDrawArraysInstanced", 30, -1 },
   // We check for the aliased -NV version in GLES 2
   // { "glDrawBuffers", 30, -1 },
   { "glDrawElementsInstanced", 30, -1 },
   { "glDrawRangeElements", 30, -1 },
   // We check for the aliased -EXT version in GLES 2
   // { "glEndQuery", 30, -1 },
   { "glEndTransformFeedback", 30, -1 },
   { "glFenceSync", 30, -1 },
   // We check for the aliased -EXT version in GLES 2
   // { "glFlushMappedBufferRange", 30, -1 },
   { "glFramebufferTextureLayer", 30, -1 },
   // We check for the aliased -EXT version in GLES 2
   // { "glGenQueries", 30, -1 },
   { "glGenSamplers", 30, -1 },
   { "glGenTransformFeedbacks", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glGenVertexArrays", 30, -1 },
   { "glGetActiveUniformBlockiv", 30, -1 },
   { "glGetActiveUniformBlockName", 30, -1 },
   { "glGetActiveUniformsiv", 30, -1 },
   { "glGetBufferParameteri64v", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glGetBufferPointerv", 30, -1 },
   { "glGetFragDataLocation", 30, -1 },
   { "glGetInteger64i_v", 30, -1 },
   { "glGetInteger64v", 30, -1 },
   { "glGetIntegeri_v", 30, -1 },
   { "glGetInternalformativ", 30, -1 },
   { "glGetInternalformati64v", 30, -1 },
   // glGetProgramBinary aliases glGetProgramBinaryOES in GLES 2
   // We check for the aliased -EXT version in GLES 2
   // { "glGetQueryiv", 30, -1 },
   // We check for the aliased -EXT version in GLES 2
   // { "glGetQueryObjectuiv", 30, -1 },
   { "glGetSamplerParameterfv", 30, -1 },
   { "glGetSamplerParameteriv", 30, -1 },
   { "glGetStringi", 30, -1 },
   { "glGetSynciv", 30, -1 },
   { "glGetTransformFeedbackVarying", 30, -1 },
   { "glGetUniformBlockIndex", 30, -1 },
   { "glGetUniformIndices", 30, -1 },
   { "glGetUniformuiv", 30, -1 },
   { "glGetVertexAttribIiv", 30, -1 },
   { "glGetVertexAttribIuiv", 30, -1 },
   { "glInvalidateFramebuffer", 30, -1 },
   { "glInvalidateSubFramebuffer", 30, -1 },
   // We check for the aliased -EXT version in GLES 2
   // { "glIsQuery", 30, -1 },
   { "glIsSampler", 30, -1 },
   { "glIsSync", 30, -1 },
   { "glIsTransformFeedback", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glIsVertexArray", 30, -1 },
   // We check for the aliased -EXT version in GLES 2
   // { "glMapBufferRange", 30, -1 },
   { "glPauseTransformFeedback", 30, -1 },
   // glProgramBinary aliases glProgramBinaryOES in GLES 2
   // glProgramParameteri aliases glProgramParameteriEXT in GLES 2
   // We check for the aliased -NV version in GLES 2
   // { "glReadBuffer", 30, -1 },
   // glRenderbufferStorageMultisample aliases glRenderbufferStorageMultisampleEXT in GLES 2
   { "glResumeTransformFeedback", 30, -1 },
   { "glSamplerParameterf", 30, -1 },
   { "glSamplerParameterfv", 30, -1 },
   { "glSamplerParameteri", 30, -1 },
   { "glSamplerParameteriv", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glTexImage3D", 30, -1 },
   { "glTexStorage2D", 30, -1 },
   { "glTexStorage3D", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glTexSubImage3D", 30, -1 },
   { "glTransformFeedbackVaryings", 30, -1 },
   { "glUniform1ui", 30, -1 },
   { "glUniform1uiv", 30, -1 },
   { "glUniform2ui", 30, -1 },
   { "glUniform2uiv", 30, -1 },
   { "glUniform3ui", 30, -1 },
   { "glUniform3uiv", 30, -1 },
   { "glUniform4ui", 30, -1 },
   { "glUniform4uiv", 30, -1 },
   { "glUniformBlockBinding", 30, -1 },
   { "glUniformMatrix2x3fv", 30, -1 },
   { "glUniformMatrix2x4fv", 30, -1 },
   { "glUniformMatrix3x2fv", 30, -1 },
   { "glUniformMatrix3x4fv", 30, -1 },
   { "glUniformMatrix4x2fv", 30, -1 },
   { "glUniformMatrix4x3fv", 30, -1 },
   // We check for the aliased -OES version in GLES 2
   // { "glUnmapBuffer", 30, -1 },
   { "glVertexAttribDivisor", 30, -1 },
   { "glVertexAttribI4i", 30, -1 },
   { "glVertexAttribI4iv", 30, -1 },
   { "glVertexAttribI4ui", 30, -1 },
   { "glVertexAttribI4uiv", 30, -1 },
   { "glVertexAttribIPointer", 30, -1 },
   { "glWaitSync", 30, -1 },

   /* GL_EXT_separate_shader_objects - Also part of OpenGL ES 3.1. */
   { "glProgramUniform1uiEXT", 30, -1 },
   { "glProgramUniform1uivEXT", 30, -1 },
   { "glProgramUniform2uiEXT", 30, -1 },
   { "glProgramUniform2uivEXT", 30, -1 },
   { "glProgramUniform3uiEXT", 30, -1 },
   { "glProgramUniform3uivEXT", 30, -1 },
   { "glProgramUniform4uiEXT", 30, -1 },
   { "glProgramUniform4uivEXT", 30, -1 },

   /* GL_EXT_blend_func_extended */
   { "glBindFragDataLocationIndexedEXT", 30, -1 },
   { "glGetFragDataIndexEXT", 30, -1 },
   { "glBindFragDataLocationEXT", 30, -1 },

   /* GL_OES_texture_border_clamp */
   { "glTexParameterIivOES", 30, -1 },
   { "glTexParameterIuivOES", 30, -1 },
   { "glGetTexParameterIivOES", 30, -1 },
   { "glGetTexParameterIuivOES", 30, -1 },
   { "glSamplerParameterIivOES", 30, -1 },
   { "glSamplerParameterIuivOES", 30, -1 },
   { "glGetSamplerParameterIivOES", 30, -1 },
   { "glGetSamplerParameterIuivOES", 30, -1 },

   /* GL_OES_texture_buffer */
   { "glTexBufferOES", 31, -1 },
   { "glTexBufferRangeOES", 31, -1 },

   /* GL_OES_sample_shading */
   { "glMinSampleShadingOES", 30, -1 },

   /* GL_OES_copy_image */
   { "glCopyImageSubDataOES", 30, -1 },

   /* GL_OES_draw_buffers_indexed */
   { "glBlendFunciOES", 30, -1 },
   { "glBlendFuncSeparateiOES", 30, -1 },
   { "glBlendEquationiOES", 30, -1 },
   { "glBlendEquationSeparateiOES", 30, -1 },
   { "glColorMaskiOES", 30, -1 },
   { "glEnableiOES", 30, -1 },
   { "glDisableiOES", 30, -1 },
   { "glIsEnablediOES", 30, -1 },

   /* GL_EXT_base_instance */
   { "glDrawArraysInstancedBaseInstanceEXT", 30, -1 },
   { "glDrawElementsInstancedBaseInstanceEXT", 30, -1 },
   { "glDrawElementsInstancedBaseVertexBaseInstanceEXT", 30, -1 },

   /* GL_EXT_window_rectangles */
   { "glWindowRectanglesEXT", 30, -1 },

   /* GL_AMD_framebuffer_multisample_advanced */
   { "glRenderbufferStorageMultisampleAdvancedAMD", 11, -1 },
   { "glNamedRenderbufferStorageMultisampleAdvancedAMD", 11, -1 },

   { NULL, 0, -1 }
};

const struct function gles31_functions_possible[] = {
   { "glDispatchCompute", 31, -1 },
   { "glDispatchComputeIndirect", 31, -1 },
   { "glDrawArraysIndirect", 31, -1 },
   { "glDrawElementsIndirect", 31, -1 },

   { "glFramebufferParameteri", 31, -1 },
   { "glGetFramebufferParameteriv", 31, -1 },

   { "glGetProgramInterfaceiv", 31, -1 },
   { "glGetProgramResourceIndex", 31, -1 },
   { "glGetProgramResourceName", 31, -1 },
   { "glGetProgramResourceiv", 31, -1 },
   { "glGetProgramResourceLocation", 31, -1 },

   // We check for the aliased EXT versions in GLES 2
   // { "glUseProgramStages", 31, -1 },
   // { "glActiveShaderProgram", 31, -1 },
   // { "glCreateShaderProgramv", 31, -1 },
   // { "glBindProgramPipeline", 31, -1 },
   // { "glDeleteProgramPipelines", 31, -1 },
   // { "glGenProgramPipelines", 31, -1 },
   // { "glIsProgramPipeline", 31, -1 },
   // { "glGetProgramPipelineiv", 31, -1 },
   // { "glProgramUniform1i", 31, -1 },
   // { "glProgramUniform2i", 31, -1 },
   // { "glProgramUniform3i", 31, -1 },
   // { "glProgramUniform4i", 31, -1 },
   // { "glProgramUniform1f", 31, -1 },
   // { "glProgramUniform2f", 31, -1 },
   // { "glProgramUniform3f", 31, -1 },
   // { "glProgramUniform4f", 31, -1 },
   // { "glProgramUniform1iv", 31, -1 },
   // { "glProgramUniform2iv", 31, -1 },
   // { "glProgramUniform3iv", 31, -1 },
   // { "glProgramUniform4iv", 31, -1 },
   // { "glProgramUniform1fv", 31, -1 },
   // { "glProgramUniform2fv", 31, -1 },
   // { "glProgramUniform3fv", 31, -1 },
   // { "glProgramUniform4fv", 31, -1 },
   // { "glProgramUniformMatrix2fv", 31, -1 },
   // { "glProgramUniformMatrix3fv", 31, -1 },
   // { "glProgramUniformMatrix4fv", 31, -1 },
   // { "glProgramUniformMatrix2x3fv", 31, -1 },
   // { "glProgramUniformMatrix3x2fv", 31, -1 },
   // { "glProgramUniformMatrix2x4fv", 31, -1 },
   // { "glProgramUniformMatrix4x2fv", 31, -1 },
   // { "glProgramUniformMatrix3x4fv", 31, -1 },
   // { "glProgramUniformMatrix4x3fv", 31, -1 },
   // { "glValidateProgramPipeline", 31, -1 },
   // { "glGetProgramPipelineInfoLog", 31, -1 },

   // We check for the aliased EXT versions in GLES 3
   // { "glProgramUniform1ui", 31, -1 },
   // { "glProgramUniform2ui", 31, -1 },
   // { "glProgramUniform3ui", 31, -1 },
   // { "glProgramUniform4ui", 31, -1 },
   // { "glProgramUniform1uiv", 31, -1 },
   // { "glProgramUniform2uiv", 31, -1 },
   // { "glProgramUniform3uiv", 31, -1 },
   // { "glProgramUniform4uiv", 31, -1 },

   { "glBindImageTexture", 31, -1 },
   { "glGetBooleani_v", 31, -1 },
   { "glMemoryBarrier", 31, -1 },

   { "glMemoryBarrierByRegion", 31, -1 },

   { "glTexStorage2DMultisample", 31, -1 },
   { "glGetMultisamplefv", 31, -1 },
   { "glSampleMaski", 31, -1 },
   { "glGetTexLevelParameteriv", 31, -1 },
   { "glGetTexLevelParameterfv", 31, -1 },
   { "glBindVertexBuffer", 31, -1 },
   { "glVertexAttribFormat", 31, -1 },
   { "glVertexAttribIFormat", 31, -1 },
   { "glVertexAttribBinding", 31, -1 },
   { "glVertexBindingDivisor", 31, -1 },

   /* GL_OES_texture_storage_multisample_2d_array */
   { "glTexStorage3DMultisampleOES", 31, -1 },

   /* GL_OES_texture_view */
   { "glTextureViewOES", 31, -1 },

   /* GL_EXT_buffer_storage */
   { "glBufferStorageEXT", 31, -1 },

   /* GL_EXT_blend_func_extended */
   { "glGetProgramResourceLocationIndexEXT", 31, -1 },

   /* GL_OES_geometry_shader */
   { "glFramebufferTextureOES", 31, -1},

   /* GL_EXT_geometry_shader */
   // We check for the aliased OES version above
   // { "glFramebufferTextureEXT", 31, -1},

   /* GL_OES_tessellation_shader */
   { "glPatchParameteriOES", 31, -1 },

   /* GL_OES_primitive_bound_box */
   { "glPrimitiveBoundingBoxOES", 31, -1 },

   /* GL_OES_viewport_array */
   { "glViewportArrayvOES", 31, -1 },
   { "glViewportIndexedfOES", 31, -1 },
   { "glViewportIndexedfvOES", 31, -1 },
   { "glScissorArrayvOES", 31, -1 },
   { "glScissorIndexedOES", 31, -1 },
   { "glScissorIndexedvOES", 31, -1 },
   { "glDepthRangeArrayfvOES", 31, -1 },
   { "glDepthRangeIndexedfOES", 31, -1 },
   { "glGetFloati_vOES", 31, -1 },

   /* GL_ARB_sample_locations */
   { "glFramebufferSampleLocationsfvARB", 31, -1 },
   { "glNamedFramebufferSampleLocationsfvARB", 31, -1 },
   { "glEvaluateDepthValuesARB", 31, -1 },

   { NULL, 0, -1 },
 };
