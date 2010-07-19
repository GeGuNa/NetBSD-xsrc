/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/api_arrayelt.h"
#include "main/context.h"
#include "main/simple_list.h"
#include "main/imports.h"
#include "main/extensions.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "radeon_common.h"
#include "radeon_context.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "radeon_span.h"
#include "radeon_tex.h"
#include "radeon_swtcl.h"
#include "radeon_tcl.h"
#include "radeon_queryobj.h"
#include "radeon_blit.h"

#define need_GL_ARB_occlusion_query
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_fog_coord
#define need_GL_EXT_secondary_color
#define need_GL_EXT_framebuffer_object
#include "main/remap_helper.h"

#define DRIVER_DATE	"20061018"

#include "utils.h"
#include "xmlpool.h" /* for symbolic values of enum-type options */

/* Extension strings exported by the R100 driver.
 */
static const struct dri_extension card_extensions[] =
{
    { "GL_ARB_multitexture",               NULL },
    { "GL_ARB_occlusion_query",		   GL_ARB_occlusion_query_functions},
    { "GL_ARB_texture_border_clamp",       NULL },
    { "GL_ARB_texture_env_add",            NULL },
    { "GL_ARB_texture_env_combine",        NULL },
    { "GL_ARB_texture_env_crossbar",       NULL },
    { "GL_ARB_texture_env_dot3",           NULL },
    { "GL_ARB_texture_mirrored_repeat",    NULL },
    { "GL_EXT_blend_logic_op",             NULL },
    { "GL_EXT_blend_subtract",             GL_EXT_blend_minmax_functions },
    { "GL_EXT_fog_coord",                  GL_EXT_fog_coord_functions },
    { "GL_EXT_packed_depth_stencil",	   NULL},
    { "GL_EXT_secondary_color",            GL_EXT_secondary_color_functions },
    { "GL_EXT_stencil_wrap",               NULL },
    { "GL_EXT_texture_edge_clamp",         NULL },
    { "GL_EXT_texture_env_combine",        NULL },
    { "GL_EXT_texture_env_dot3",           NULL },
    { "GL_EXT_texture_filter_anisotropic", NULL },
    { "GL_EXT_texture_lod_bias",           NULL },
    { "GL_EXT_texture_mirror_clamp",       NULL },
    { "GL_ATI_texture_env_combine3",       NULL },
    { "GL_ATI_texture_mirror_once",        NULL },
    { "GL_MESA_ycbcr_texture",             NULL },
    { "GL_NV_blend_square",                NULL },
    { "GL_SGIS_generate_mipmap",           NULL },
    { NULL,                                NULL }
};

static const struct dri_extension mm_extensions[] = {
  { "GL_EXT_framebuffer_object", GL_EXT_framebuffer_object_functions },
  { NULL, NULL }
};

extern const struct tnl_pipeline_stage _radeon_render_stage;
extern const struct tnl_pipeline_stage _radeon_tcl_stage;

static const struct tnl_pipeline_stage *radeon_pipeline[] = {

   /* Try and go straight to t&l
    */
   &_radeon_tcl_stage,  

   /* Catch any t&l fallbacks
    */
   &_tnl_vertex_transform_stage,
   &_tnl_normal_transform_stage,
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,

   &_radeon_render_stage,
   &_tnl_render_stage,		/* FALLBACK:  */
   NULL,
};

static void r100_get_lock(radeonContextPtr radeon)
{
   r100ContextPtr rmesa = (r100ContextPtr)radeon;
   drm_radeon_sarea_t *sarea = radeon->sarea;

   RADEON_STATECHANGE(rmesa, ctx);
   if (rmesa->radeon.sarea->tiling_enabled) {
      rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH] |=
	 RADEON_COLOR_TILE_ENABLE;
   } else {
      rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH] &=
	 ~RADEON_COLOR_TILE_ENABLE;
   }
   
   if (sarea->ctx_owner != rmesa->radeon.dri.hwContext) {
      sarea->ctx_owner = rmesa->radeon.dri.hwContext;
      
      if (!radeon->radeonScreen->kernel_mm)
         radeon_bo_legacy_texture_age(radeon->radeonScreen->bom);
   }
}

static void r100_vtbl_emit_cs_header(struct radeon_cs *cs, radeonContextPtr rmesa)
{
}

static void r100_vtbl_pre_emit_state(radeonContextPtr radeon)
{
   r100ContextPtr rmesa = (r100ContextPtr)radeon;
   
   /* r100 always needs to emit ZBS to avoid TCL lockups */
   rmesa->hw.zbs.dirty = 1;
   radeon->hw.is_dirty = 1;
}

static void r100_vtbl_free_context(GLcontext *ctx)
{
   r100ContextPtr rmesa = R100_CONTEXT(ctx);
   _mesa_vector4f_free( &rmesa->tcl.ObjClean );
}

static void r100_emit_query_finish(radeonContextPtr radeon)
{
   BATCH_LOCALS(radeon);
   struct radeon_query_object *query = radeon->query.current;

   BEGIN_BATCH_NO_AUTOSTATE(4);
   OUT_BATCH(CP_PACKET0(RADEON_RB3D_ZPASS_ADDR, 0));
   OUT_BATCH_RELOC(0, query->bo, query->curr_offset, 0, RADEON_GEM_DOMAIN_GTT, 0);
   END_BATCH();
   query->curr_offset += sizeof(uint32_t);
   assert(query->curr_offset < RADEON_QUERY_PAGE_SIZE);
   query->emitted_begin = GL_FALSE;
}

static void r100_init_vtbl(radeonContextPtr radeon)
{
   radeon->vtbl.get_lock = r100_get_lock;
   radeon->vtbl.update_viewport_offset = radeonUpdateViewportOffset;
   radeon->vtbl.emit_cs_header = r100_vtbl_emit_cs_header;
   radeon->vtbl.swtcl_flush = r100_swtcl_flush;
   radeon->vtbl.pre_emit_state = r100_vtbl_pre_emit_state;
   radeon->vtbl.fallback = radeonFallback;
   radeon->vtbl.free_context = r100_vtbl_free_context;
   radeon->vtbl.emit_query_finish = r100_emit_query_finish;
   radeon->vtbl.check_blit = r100_check_blit;
   radeon->vtbl.blit = r100_blit;
}

/* Create the device specific context.
 */
GLboolean
r100CreateContext( const __GLcontextModes *glVisual,
                     __DRIcontext *driContextPriv,
                     void *sharedContextPrivate)
{
   __DRIscreen *sPriv = driContextPriv->driScreenPriv;
   radeonScreenPtr screen = (radeonScreenPtr)(sPriv->private);
   struct dd_function_table functions;
   r100ContextPtr rmesa;
   GLcontext *ctx;
   int i;
   int tcl_mode, fthrottle_mode;

   assert(glVisual);
   assert(driContextPriv);
   assert(screen);

   /* Allocate the Radeon context */
   rmesa = (r100ContextPtr) CALLOC( sizeof(*rmesa) );
   if ( !rmesa )
      return GL_FALSE;

   rmesa->radeon.radeonScreen = screen;
   r100_init_vtbl(&rmesa->radeon);

   /* init exp fog table data */
   radeonInitStaticFogData();
   
   /* Parse configuration files.
    * Do this here so that initialMaxAnisotropy is set before we create
    * the default textures.
    */
   driParseConfigFiles (&rmesa->radeon.optionCache, &screen->optionCache,
			screen->driScreen->myNum, "radeon");
   rmesa->radeon.initialMaxAnisotropy = driQueryOptionf(&rmesa->radeon.optionCache,
                                                 "def_max_anisotropy");

   if ( driQueryOptionb( &rmesa->radeon.optionCache, "hyperz" ) ) {
      if ( sPriv->drm_version.minor < 13 )
	 fprintf( stderr, "DRM version 1.%d too old to support HyperZ, "
			  "disabling.\n", sPriv->drm_version.minor );
      else
	 rmesa->using_hyperz = GL_TRUE;
   }

   if ( sPriv->drm_version.minor >= 15 )
      rmesa->texmicrotile = GL_TRUE;

   /* Init default driver functions then plug in our Radeon-specific functions
    * (the texture functions are especially important)
    */
   _mesa_init_driver_functions( &functions );
   radeonInitTextureFuncs( &rmesa->radeon, &functions );
   radeonInitQueryObjFunctions(&functions);

   if (!radeonInitContext(&rmesa->radeon, &functions,
			  glVisual, driContextPriv,
			  sharedContextPrivate)) {
     FREE(rmesa);
     return GL_FALSE;
   }

   rmesa->radeon.swtcl.RenderIndex = ~0;
   rmesa->radeon.hw.all_dirty = GL_TRUE;

   /* Set the maximum texture size small enough that we can guarentee that
    * all texture units can bind a maximal texture and have all of them in
    * texturable memory at once. Depending on the allow_large_textures driconf
    * setting allow larger textures.
    */

   ctx = rmesa->radeon.glCtx;
   ctx->Const.MaxTextureUnits = driQueryOptioni (&rmesa->radeon.optionCache,
						 "texture_units");
   ctx->Const.MaxTextureImageUnits = ctx->Const.MaxTextureUnits;
   ctx->Const.MaxTextureCoordUnits = ctx->Const.MaxTextureUnits;
   ctx->Const.MaxCombinedTextureImageUnits = ctx->Const.MaxTextureUnits;

   i = driQueryOptioni( &rmesa->radeon.optionCache, "allow_large_textures");

   /* FIXME: When no memory manager is available we should set this 
    * to some reasonable value based on texture memory pool size */
   ctx->Const.MaxTextureLevels = 12;
   ctx->Const.Max3DTextureLevels = 9;
   ctx->Const.MaxCubeTextureLevels = 12;
   ctx->Const.MaxTextureRectSize = 2048;

   ctx->Const.MaxTextureMaxAnisotropy = 16.0;

   /* No wide points.
    */
   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSize = 1.0;
   ctx->Const.MaxPointSizeAA = 1.0;

   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 10.0;
   ctx->Const.MaxLineWidthAA = 10.0;
   ctx->Const.LineWidthGranularity = 0.0625;

   /* Set maxlocksize (and hence vb size) small enough to avoid
    * fallbacks in radeon_tcl.c.  ie. guarentee that all vertices can
    * fit in a single dma buffer for indexed rendering of quad strips,
    * etc.
    */
   ctx->Const.MaxArrayLockSize = 
      MIN2( ctx->Const.MaxArrayLockSize, 
 	    RADEON_BUFFER_SIZE / RADEON_MAX_TCL_VERTSIZE ); 

   rmesa->boxes = 0;

   ctx->Const.MaxDrawBuffers = 1;
   ctx->Const.MaxColorAttachments = 1;
   ctx->Const.MaxRenderbufferSize = 2048;

   _mesa_set_mvp_with_dp4( ctx, GL_TRUE );

   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext( ctx );
   _vbo_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );
   _ae_create_context( ctx );

   /* Install the customized pipeline:
    */
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, radeon_pipeline );

   /* Try and keep materials and vertices separate:
    */
/*    _tnl_isolate_materials( ctx, GL_TRUE ); */

   /* Configure swrast and T&L to match hardware characteristics:
    */
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );
   _tnl_allow_pixel_fog( ctx, GL_FALSE );
   _tnl_allow_vertex_fog( ctx, GL_TRUE );


   for ( i = 0 ; i < RADEON_MAX_TEXTURE_UNITS ; i++ ) {
      _math_matrix_ctr( &rmesa->TexGenMatrix[i] );
      _math_matrix_ctr( &rmesa->tmpmat[i] );
      _math_matrix_set_identity( &rmesa->TexGenMatrix[i] );
      _math_matrix_set_identity( &rmesa->tmpmat[i] );
   }

   driInitExtensions( ctx, card_extensions, GL_TRUE );
   if (rmesa->radeon.radeonScreen->kernel_mm)
     driInitExtensions(ctx, mm_extensions, GL_FALSE);
   if (rmesa->radeon.radeonScreen->drmSupportsCubeMapsR100)
      _mesa_enable_extension( ctx, "GL_ARB_texture_cube_map" );
   if (rmesa->radeon.glCtx->Mesa_DXTn) {
      _mesa_enable_extension( ctx, "GL_EXT_texture_compression_s3tc" );
      _mesa_enable_extension( ctx, "GL_S3_s3tc" );
   }
   else if (driQueryOptionb (&rmesa->radeon.optionCache, "force_s3tc_enable")) {
      _mesa_enable_extension( ctx, "GL_EXT_texture_compression_s3tc" );
   }

   if (rmesa->radeon.radeonScreen->kernel_mm || rmesa->radeon.dri.drmMinor >= 9)
      _mesa_enable_extension( ctx, "GL_NV_texture_rectangle");

   if (!rmesa->radeon.radeonScreen->kernel_mm)
      _mesa_disable_extension(ctx, "GL_ARB_occlusion_query");

   /* XXX these should really go right after _mesa_init_driver_functions() */
   radeon_fbo_init(&rmesa->radeon);
   radeonInitSpanFuncs( ctx );
   radeonInitIoctlFuncs( ctx );
   radeonInitStateFuncs( ctx , rmesa->radeon.radeonScreen->kernel_mm );
   radeonInitState( rmesa );
   radeonInitSwtcl( ctx );

   _mesa_vector4f_alloc( &rmesa->tcl.ObjClean, 0, 
			 ctx->Const.MaxArrayLockSize, 32 );

   fthrottle_mode = driQueryOptioni(&rmesa->radeon.optionCache, "fthrottle_mode");
   rmesa->radeon.iw.irq_seq = -1;
   rmesa->radeon.irqsEmitted = 0;
   rmesa->radeon.do_irqs = (rmesa->radeon.radeonScreen->irq != 0 &&
			    fthrottle_mode == DRI_CONF_FTHROTTLE_IRQS);

   rmesa->radeon.do_usleeps = (fthrottle_mode == DRI_CONF_FTHROTTLE_USLEEPS);


#if DO_DEBUG
   RADEON_DEBUG = driParseDebugString( getenv( "RADEON_DEBUG" ),
				       debug_control );
#endif

   tcl_mode = driQueryOptioni(&rmesa->radeon.optionCache, "tcl_mode");
   if (driQueryOptionb(&rmesa->radeon.optionCache, "no_rast")) {
      fprintf(stderr, "disabling 3D acceleration\n");
      FALLBACK(rmesa, RADEON_FALLBACK_DISABLE, 1);
   } else if (tcl_mode == DRI_CONF_TCL_SW ||
	      !(rmesa->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL)) {
      if (rmesa->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL) {
	 rmesa->radeon.radeonScreen->chip_flags &= ~RADEON_CHIPSET_TCL;
	 fprintf(stderr, "Disabling HW TCL support\n");
      }
      TCL_FALLBACK(rmesa->radeon.glCtx, RADEON_TCL_FALLBACK_TCL_DISABLE, 1);
   }

   if (rmesa->radeon.radeonScreen->chip_flags & RADEON_CHIPSET_TCL) {
/*       _tnl_need_dlist_norm_lengths( ctx, GL_FALSE ); */
   }
   return GL_TRUE;
}
