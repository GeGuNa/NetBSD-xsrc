/* $XFree86$ */
/*
** The contents of this file are subject to the GLX Public License Version 1.0
** (the "License"). You may not use this file except in compliance with the
** License. You may obtain a copy of the License at Silicon Graphics, Inc.,
** attn: Legal Services, 2011 N. Shoreline Blvd., Mountain View, CA 94043
** or at http://www.sgi.com/software/opensource/glx/license.html.
**
** Software distributed under the License is distributed on an "AS IS"
** basis. ALL WARRANTIES ARE DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY
** IMPLIED WARRANTIES OF MERCHANTABILITY, OF FITNESS FOR A PARTICULAR
** PURPOSE OR OF NON- INFRINGEMENT. See the License for the specific
** language governing rights and limitations under the License.
**
** The Original Software is GLX version 1.2 source code, released February,
** 1999. The developer of the Original Software is Silicon Graphics, Inc.
** Those portions of the Subject Software created by Silicon Graphics, Inc.
** are Copyright (c) 1991-9 Silicon Graphics, Inc. All Rights Reserved.
**
** $SGI$
*/

#include "glxserver.h"

__GLXrenderSizeData __glXRenderSizeTable[] = {
	/* no such opcode      */       {   0,  0                         },
	/* CallList            */	{   8, 	0                         },
	/* CallLists           */	{  12, 	__glXCallListsReqSize     },
	/* ListBase            */	{   8, 	0                         },
	/* Begin               */	{   8, 	0                         },
	/* Bitmap              */	{  48, 	__glXBitmapReqSize        },
	/* Color3bv            */	{   8, 	0                         },
	/* Color3dv            */	{  28, 	0                         },
	/* Color3fv            */	{  16, 	0                         },
	/* Color3iv            */	{  16, 	0                         },
	/* Color3sv            */	{  12, 	0                         },
	/* Color3ubv           */	{   8, 	0                         },
	/* Color3uiv           */	{  16, 	0                         },
	/* Color3usv           */	{  12, 	0                         },
	/* Color4bv            */	{   8, 	0                         },
	/* Color4dv            */	{  36, 	0                         },
	/* Color4fv            */	{  20, 	0                         },
	/* Color4iv            */	{  20, 	0                         },
	/* Color4sv            */	{  12, 	0                         },
	/* Color4ubv           */	{   8, 	0                         },
	/* Color4uiv           */	{  20, 	0                         },
	/* Color4usv           */	{  12, 	0                         },
	/* EdgeFlagv           */	{   8, 	0                         },
	/* End                 */	{   4, 	0                         },
	/* Indexdv             */	{  12, 	0                         },
	/* Indexfv             */	{   8, 	0                         },
	/* Indexiv             */	{   8, 	0                         },
	/* Indexsv             */	{   8, 	0                         },
	/* Normal3bv           */	{   8, 	0                         },
	/* Normal3dv           */	{  28, 	0                         },
	/* Normal3fv           */	{  16, 	0                         },
	/* Normal3iv           */	{  16, 	0                         },
	/* Normal3sv           */	{  12, 	0                         },
	/* RasterPos2dv        */	{  20, 	0                         },
	/* RasterPos2fv        */	{  12, 	0                         },
	/* RasterPos2iv        */	{  12, 	0                         },
	/* RasterPos2sv        */	{   8, 	0                         },
	/* RasterPos3dv        */	{  28, 	0                         },
	/* RasterPos3fv        */	{  16, 	0                         },
	/* RasterPos3iv        */	{  16, 	0                         },
	/* RasterPos3sv        */	{  12, 	0                         },
	/* RasterPos4dv        */	{  36, 	0                         },
	/* RasterPos4fv        */	{  20, 	0                         },
	/* RasterPos4iv        */	{  20, 	0                         },
	/* RasterPos4sv        */	{  12, 	0                         },
	/* Rectdv              */	{  36, 	0                         },
	/* Rectfv              */	{  20, 	0                         },
	/* Rectiv              */	{  20, 	0                         },
	/* Rectsv              */	{  12, 	0                         },
	/* TexCoord1dv         */	{  12, 	0                         },
	/* TexCoord1fv         */	{   8, 	0                         },
	/* TexCoord1iv         */	{   8, 	0                         },
	/* TexCoord1sv         */	{   8, 	0                         },
	/* TexCoord2dv         */	{  20, 	0                         },
	/* TexCoord2fv         */	{  12, 	0                         },
	/* TexCoord2iv         */	{  12, 	0                         },
	/* TexCoord2sv         */	{   8, 	0                         },
	/* TexCoord3dv         */	{  28, 	0                         },
	/* TexCoord3fv         */	{  16, 	0                         },
	/* TexCoord3iv         */	{  16, 	0                         },
	/* TexCoord3sv         */	{  12, 	0                         },
	/* TexCoord4dv         */	{  36, 	0                         },
	/* TexCoord4fv         */	{  20, 	0                         },
	/* TexCoord4iv         */	{  20, 	0                         },
	/* TexCoord4sv         */	{  12, 	0                         },
	/* Vertex2dv           */	{  20, 	0                         },
	/* Vertex2fv           */	{  12, 	0                         },
	/* Vertex2iv           */	{  12, 	0                         },
	/* Vertex2sv           */	{   8, 	0                         },
	/* Vertex3dv           */	{  28, 	0                         },
	/* Vertex3fv           */	{  16, 	0                         },
	/* Vertex3iv           */	{  16, 	0                         },
	/* Vertex3sv           */	{  12, 	0                         },
	/* Vertex4dv           */	{  36, 	0                         },
	/* Vertex4fv           */	{  20, 	0                         },
	/* Vertex4iv           */	{  20, 	0                         },
	/* Vertex4sv           */	{  12, 	0                         },
	/* ClipPlane           */	{  40, 	0                         },
	/* ColorMaterial       */	{  12, 	0                         },
	/* CullFace            */	{   8, 	0                         },
	/* Fogf                */	{  12, 	0                         },
	/* Fogfv               */	{   8, 	__glXFogfvReqSize         },
	/* Fogi                */	{  12, 	0                         },
	/* Fogiv               */	{   8, 	__glXFogivReqSize         },
	/* FrontFace           */	{   8, 	0                         },
	/* Hint                */	{  12, 	0                         },
	/* Lightf              */	{  16, 	0                         },
	/* Lightfv             */	{  12, 	__glXLightfvReqSize       },
	/* Lighti              */	{  16, 	0                         },
	/* Lightiv             */	{  12, 	__glXLightivReqSize       },
	/* LightModelf         */	{  12, 	0                         },
	/* LightModelfv        */	{   8, 	__glXLightModelfvReqSize  },
	/* LightModeli         */	{  12, 	0                         },
	/* LightModeliv        */	{   8, 	__glXLightModelivReqSize  },
	/* LineStipple         */	{  12, 	0                         },
	/* LineWidth           */	{   8, 	0                         },
	/* Materialf           */	{  16, 	0                         },
	/* Materialfv          */	{  12, 	__glXMaterialfvReqSize    },
	/* Materiali           */	{  16, 	0                         },
	/* Materialiv          */	{  12, 	__glXMaterialivReqSize    },
	/* PointSize           */	{   8, 	0                         },
	/* PolygonMode         */	{  12, 	0                         },
	/* PolygonStipple      */	{ 152, 	0                         },
	/* Scissor             */	{  20, 	0                         },
	/* ShadeModel          */	{   8, 	0                         },
	/* TexParameterf       */	{  16, 	0                         },
	/* TexParameterfv      */	{  12, 	__glXTexParameterfvReqSize },
	/* TexParameteri       */	{  16, 	0                         },
	/* TexParameteriv      */	{  12, 	__glXTexParameterivReqSize },
	/* TexImage1D          */	{  56, 	__glXTexImage1DReqSize    },
	/* TexImage2D          */	{  56, 	__glXTexImage2DReqSize    },
	/* TexEnvf             */	{  16, 	0                         },
	/* TexEnvfv            */	{  12, 	__glXTexEnvfvReqSize      },
	/* TexEnvi             */	{  16, 	0                         },
	/* TexEnviv            */	{  12, 	__glXTexEnvivReqSize      },
	/* TexGend             */	{  20, 	0                         },
	/* TexGendv            */	{  12, 	__glXTexGendvReqSize      },
	/* TexGenf             */	{  16, 	0                         },
	/* TexGenfv            */	{  12, 	__glXTexGenfvReqSize      },
	/* TexGeni             */	{  16, 	0                         },
	/* TexGeniv            */	{  12, 	__glXTexGenivReqSize      },
	/* InitNames           */	{   4, 	0                         },
	/* LoadName            */	{   8, 	0                         },
	/* PassThrough         */	{   8, 	0                         },
	/* PopName             */	{   4, 	0                         },
	/* PushName            */	{   8, 	0                         },
	/* DrawBuffer          */	{   8, 	0                         },
	/* Clear               */	{   8, 	0                         },
	/* ClearAccum          */	{  20, 	0                         },
	/* ClearIndex          */	{   8, 	0                         },
	/* ClearColor          */	{  20, 	0                         },
	/* ClearStencil        */	{   8, 	0                         },
	/* ClearDepth          */	{  12, 	0                         },
	/* StencilMask         */	{   8, 	0                         },
	/* ColorMask           */	{   8, 	0                         },
	/* DepthMask           */	{   8, 	0                         },
	/* IndexMask           */	{   8, 	0                         },
	/* Accum               */	{  12, 	0                         },
	/* Disable             */	{   8, 	0                         },
	/* Enable              */	{   8, 	0                         },
	/* no such opcode      */	{   0, 	0                         },
	/* PopAttrib           */	{   4, 	0                         },
	/* PushAttrib          */	{   8, 	0                         },
	/* Map1d               */	{  28, 	__glXMap1dReqSize         },
	/* Map1f               */	{  20, 	__glXMap1fReqSize         },
	/* Map2d               */	{  48, 	__glXMap2dReqSize         },
	/* Map2f               */	{  32, 	__glXMap2fReqSize         },
	/* MapGrid1d           */	{  24, 	0                         },
	/* MapGrid1f           */	{  16, 	0                         },
	/* MapGrid2d           */	{  44, 	0                         },
	/* MapGrid2f           */	{  28, 	0                         },
	/* EvalCoord1dv        */	{  12, 	0                         },
	/* EvalCoord1fv        */	{   8, 	0                         },
	/* EvalCoord2dv        */	{  20, 	0                         },
	/* EvalCoord2fv        */	{  12, 	0                         },
	/* EvalMesh1           */	{  16, 	0                         },
	/* EvalPoint1          */	{   8, 	0                         },
	/* EvalMesh2           */	{  24, 	0                         },
	/* EvalPoint2          */	{  12, 	0                         },
	/* AlphaFunc           */	{  12, 	0                         },
	/* BlendFunc           */	{  12, 	0                         },
	/* LogicOp             */	{   8, 	0                         },
	/* StencilFunc         */	{  16, 	0                         },
	/* StencilOp           */	{  16, 	0                         },
	/* DepthFunc           */	{   8, 	0                         },
	/* PixelZoom           */	{  12, 	0                         },
	/* PixelTransferf      */	{  12, 	0                         },
	/* PixelTransferi      */	{  12, 	0                         },
	/* PixelMapfv          */	{  12, 	__glXPixelMapfvReqSize    },
	/* PixelMapuiv         */	{  12, 	__glXPixelMapuivReqSize   },
	/* PixelMapusv         */	{  12, 	__glXPixelMapusvReqSize   },
	/* ReadBuffer          */	{   8, 	0                         },
	/* CopyPixels          */	{  24, 	0                         },
	/* DrawPixels          */	{  40, 	__glXDrawPixelsReqSize    },
	/* DepthRange          */	{  20, 	0                         },
	/* Frustum             */	{  52, 	0                         },
	/* LoadIdentity        */	{   4, 	0                         },
	/* LoadMatrixf         */	{  68, 	0                         },
	/* LoadMatrixd         */	{ 132, 	0                         },
	/* MatrixMode          */	{   8, 	0                         },
	/* MultMatrixf         */	{  68, 	0                         },
	/* MultMatrixd         */	{ 132, 	0                         },
	/* Ortho               */	{  52, 	0                         },
	/* PopMatrix           */	{   4, 	0                         },
	/* PushMatrix          */	{   4, 	0                         },
	/* Rotated             */	{  36, 	0                         },
	/* Rotatef             */	{  20, 	0                         },
	/* Scaled              */	{  28, 	0                         },
	/* Scalef              */	{  16, 	0                         },
	/* Translated          */	{  28, 	0                         },
	/* Translatef          */	{  16, 	0                         },
	/* Viewport            */	{  20, 	0                         },
	/* PolygonOffset       */	{  12, 	0                         },
	/* DrawArrays          */	{  16, 	__glXDrawArraysSize       },
	/* Indexubv            */	{  8, 	0                         },
};
__GLXrenderSizeData __glXRenderSizeTable_EXT[] = {
	/* PolygonOffset       */	{  12, 	0                         },
	/* TexSubImage1D       */	{  60, 	__glXTexSubImage1DReqSize },
	/* TexSubImage2D       */	{  60, 	__glXTexSubImage2DReqSize },
	/* no such opcode      */	{   0, 	0                         },
	/* no such opcode      */	{   0, 	0                         },
	/* no such opcode      */	{   0, 	0                         },
	/* no such opcode      */	{   0, 	0                         },
	/* no such opcode      */	{   0, 	0                         },
	/* no such opcode      */	{   0, 	0                         },
	/* no such opcode      */	{   0, 	0                         },
	/* no such opcode      */	{   0, 	0                         },
	/* no such opcode      */	{   0, 	0                         },
	/* no such opcode      */	{   0, 	0                         },
	/* no such opcode      */	{   0, 	0                         },
	/* no such opcode      */	{   0, 	0                         },
	/* no such opcode      */	{   0, 	0                         },
	/* no such opcode      */	{   0, 	0                         },
	/* no such opcode      */	{   0, 	0                         },
	/* no such opcode      */	{   0, 	0                         },
	/* BindTexture         */	{  12, 	0                         },
	/* PrioritizeTextures  */	{   8, 	__glXPrioritizeTexturesReqSize },
	/* CopyTexImage1D      */	{  32, 	0 },
	/* CopyTexImage2D      */	{  36, 	0 },
	/* CopyTexSubImage1D   */	{  28, 	0 },
	/* CopyTexSubImage2D   */	{  36, 	0 },
};
