/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/s3_virge/s3fcach.c,v 3.3 1996/10/08 13:11:57 dawes Exp $ */
/*
 * Copyright 1992 by Kevin E. Martin, Chapel Hill, North Carolina.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Kevin E. Martin not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  Kevin E. Martin makes no
 * representations about the suitability of this software for any purpose. It
 * is provided "as is" without express or implied warranty.
 *
 * KEVIN E. MARTIN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEVIN E. MARTIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/*
 * Modified by Amancio Hasty and Jon Tombs
 *
 */
/* $XConsortium: s3fcach.c /main/8 1995/12/29 10:11:14 kaleb $ */


#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"cfb.h"
#include	"misc.h"
#include        "xf86.h"
#include	"windowstr.h"
#include	"gcstruct.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"s3v.h"
#include        "xf86bcache.h"
#include	"xf86fcache.h"
#include	"xf86text.h"

#define XCONFIG_FLAGS_ONLY
#include        "xf86_Config.h"

static unsigned long s3FontAge;
#define NEXT_FONT_AGE  ++s3FontAge

extern int xf86Verbose;

#define ALIGNMENT 8
#define MAX_PIXMAP_WIDTH 64
#define MIN_PIXMAP_WIDTH 8
#define MIN_FONTCACHE_HEIGHT 13
#define MIN_FONTCACHE_WIDTH (32 * 6)

void
s3FontCache8Init()
{
   static int first = TRUE;
   int x, y, w, h, pmwidth = 0, pmx=0, pmy=0;
   int x2, y2, w2, h2;
   int BitPlane;
   CachePool FontPool;

   if (OFLG_ISSET(OPTION_NO_FONT_CACHE, &s3InfoRec.options)) {
      if (first) {
	 ErrorF("%s %s: Font cache disabled\n", XCONFIG_GIVEN, s3InfoRec.name);
      }
   }

   if (OFLG_ISSET(OPTION_NO_PIXMAP_CACHE, &s3InfoRec.options)) {
      if (first) {
	 ErrorF("%s %s: Pixmap expansion disabled\n", XCONFIG_GIVEN,
		s3InfoRec.name);
      }
   }

   /* y now includes the cursor space */
   x = x2 = 0;
   y = y2 = s3CursorStartY + s3CursorLines;
   h = h2 = s3ScissB + 1 - y;
   w = w2 = s3DisplayWidth;

   /*
    * If a full-size pixmap expansion area will fit to the right, put it
    * there.
    */
   if (0 && !OFLG_ISSET(OPTION_NO_PIXMAP_CACHE, &s3InfoRec.options)) {
    if (s3DisplayWidth - s3InfoRec.virtualX >= MAX_PIXMAP_WIDTH ||
       s3DisplayWidth - s3InfoRec.virtualX >= h) {
      /* use area at right of screen */
      if (s3DisplayWidth - s3InfoRec.virtualX > MAX_PIXMAP_WIDTH)
	 pmwidth = MAX_PIXMAP_WIDTH;
      else
	 pmwidth = s3DisplayWidth - s3InfoRec.virtualX;
      pmx = s3InfoRec.virtualX;
      pmy = 0;
      /* Decide if font cache goes to the right or at the bottom */
      if (((s3CursorStartY - pmwidth) * (s3DisplayWidth - s3InfoRec.virtualX) >
           w * h) &&
          ((s3DisplayWidth - s3InfoRec.virtualX) > MIN_FONTCACHE_WIDTH)) {
	 /* There are two possible rectangular areas to the right */
	 x = s3InfoRec.virtualX;
	 w = s3DisplayWidth - x;
	 y = pmwidth;
	 h = s3CursorStartY - y;
	 x2 = x + pmwidth;
	 w2 = w - pmwidth;
	 y2 = y - pmwidth;
	 h2 = h + pmwidth;
      }
    } else if (h >= MIN_PIXMAP_WIDTH) {
      /* use area below screen, right most location */
      if (h > MAX_PIXMAP_WIDTH )
	pmwidth = MAX_PIXMAP_WIDTH;
      else
	pmwidth = h;
      pmx = s3DisplayWidth - pmwidth;
      pmy = y;
      /*
       * If the area to the right is too narrow for the pixmap, it is also
       * too narrow for the font cache, so it goes at the bottom.  There are
       * two possible rectangular areas to the bottom.
       */
      /* x, y, h unchanged */
      w -= pmwidth;
      /* x2, w2 unchanged */
      y2 += pmwidth;
      h2 -= pmwidth;
    }

    /* Initialise the pixmap expansion area */
    if (pmwidth > 0) {
      s3InitFrect(pmx, pmy, pmwidth);
      if (first) {
	 ErrorF(
	  "%s %s: Using a single %dx%d area at (%d,%d) for expanding pixmaps\n",
	  XCONFIG_PROBED, s3InfoRec.name, pmwidth, pmwidth, pmx, pmy);
      }
    } else {
      if (first) {
	 ErrorF("%s %s: No pixmap expanding area available\n",
		XCONFIG_PROBED, s3InfoRec.name);
      }
    }
   }

   if (1 || OFLG_ISSET(OPTION_NO_FONT_CACHE, &s3InfoRec.options)) {
      if (first)
	 xf86InitText( NULL, s3NoCPolyText, s3NoCImageText );
   } else {
    /* Choose the largest font cache area */
    if ((w2 * h2 > w * h) && (w2 > MIN_FONTCACHE_WIDTH) &&
       (h2 > MIN_FONTCACHE_HEIGHT)) {
      x = x2;
      w = w2;
      y = y2;
      h = h2;
    }

    /*
     * Don't allow a font cache if we don't have room for at least
     * a complete 6x13 font.
     */
    if (w >= 6*32 && h >= MIN_FONTCACHE_HEIGHT) {
      if( first ) {
         FontPool = xf86CreateCachePool(ALIGNMENT);
         for( BitPlane = s3InfoRec.bitsPerPixel-1; BitPlane >= 0; BitPlane-- ) {
            xf86AddToCachePool(FontPool, x, y, w, h, 1<<BitPlane );
	 }

         xf86InitFontCache(FontPool, w, h, s3FontStipple);
         xf86InitText(s3GlyphWrite, s3NoCPolyText, s3NoCImageText );
         ErrorF("%s %s: Using %d planes of %dx%d at (%d,%d) aligned %d as font cache\n",
                XCONFIG_PROBED, s3InfoRec.name,
                s3InfoRec.bitsPerPixel, w, h, x, y, ALIGNMENT );
      }
      else {
        xf86ReleaseFontCache();
      }
    } else if (first) {

      /*
       * Crash and burn if the cached glyph write function gets called.
       */
      xf86InitText( NULL, s3NoCPolyText, s3NoCImageText );
      ErrorF( "%s %s: No font cache available\n",
              XCONFIG_PROBED, s3InfoRec.name );
    }
   }
   first = 0;
   return;
}

static __inline__ void
Dos3CPolyText8(x, y, count, chars, fentry, pGC, pBox)
     int   x, y, count;
     unsigned char *chars;
     CacheFont8Ptr fentry;
     GCPtr pGC;
     BoxPtr pBox;
{
   int   gHeight;
   int blocki = 255;
   unsigned short height = 0;
   unsigned short width = 0;
   Pixel pmsk = 0;

   BLOCK_CURSOR;
   for (;count > 0; count--, chars++) {
      CharInfoPtr pci;
      short xoff;

      pci = fentry->pci[(int)*chars];

      if (pci != NULL) {

	 gHeight = GLYPHHEIGHTPIXELS(pci);
	 if (gHeight) {

	    if ((int) (*chars / 32) != blocki) {
	       bitMapBlockPtr block;

	       blocki = (int) (*chars / 32);
	       block = fentry->fblock[blocki];
	       if (block == NULL) {
		  /*
		   * Reset the GE context to a known state before calling
		   * the xf86loadfontblock function.
		   */
		  WaitQueue16_32(5,6);
		  SET_SCISSORS(0,0,(s3DisplayWidth - 1), s3ScissB);
		  SET_RD_MASK(~0);

		  xf86loadFontBlock(fentry, blocki);
		  block = fentry->fblock[blocki];

		  /*
		   * Restore the GE context.
		   */
		  WaitQueue(4);
		  SET_SCISSORS((short)pBox->x1,(short)pBox->y1,(short)(pBox->x2 - 1),
							(short)(pBox->y2 - 1));
		  WaitQueue16_32(6,8);
		  SET_FRGD_COLOR(pGC->fgPixel);
		  SET_PIX_CNTL(MIXSEL_EXPBLT | COLCMPOP_F);
		  SET_FRGD_MIX(FSS_FRGDCOL | s3alu[pGC->alu]);
		  if (s3Trio32FCBug) {
		     SET_BKGD_MIX(BSS_BKGDCOL | ROP_DSo);
		     SET_WRT_MASK(pGC->planemask);

		     WaitQueue16_32(1,2);
		     SET_BKGD_COLOR(0);
		  } else {
		     SET_BKGD_MIX(BSS_BKGDCOL | ROP_D);
		     SET_WRT_MASK(pGC->planemask);
		  }
		  height = width = pmsk = 0;
	       }
	       WaitQueue16_32(2,3);
	       SET_CUR_Y(block->y);

	       /*
		* Is the readmask altered
		*/
	       if (!pmsk || pmsk != block->id) {
		  pmsk = block->id;
	       	  SET_RD_MASK(pmsk);
	       }
	       xoff = block->x;
	       block->lru = NEXT_FONT_AGE;
   	    }

	    WaitQueue(6);

	    SET_CUR_X((short) (xoff + (*chars & 0x1f) * w));
		SET_DESTSTP((short)(x + pci->metrics.leftSideBearing),
	    							(short)(y - pci->metrics.ascent));
	    if (!width || (short)(GLYPHWIDTHPIXELS(pci) - 1) != width) {
	       width = (short)(GLYPHWIDTHPIXELS(pci) - 1);
	       SET_MAJ_AXIS_PCNT(width);
	    }
	    if (!height || (short)(gHeight - 1) != height) {
	       height = (short)(gHeight - 1);
	       SET_MIN_AXIS_PCNT(height);
	    }
	    SET_CMD(CMD_BITBLT | INC_X | INC_Y | DRAW | PLANAR | WRTDATA);
	 }
	 x += pci->metrics.characterWidth;
      }
   }
   UNBLOCK_CURSOR;

   return;
}

/*
 * Set the hardware scissors to match the clipping rectables and
 * call the glyph output routine.
 */
void
s3GlyphWrite(x, y, count, chars, fentry, pGC, pBox, numRects)
     int   x, y, count;
     unsigned char *chars;
     CacheFont8Ptr fentry;
     GCPtr pGC;
     BoxPtr pBox;
     int numRects;
{
   BLOCK_CURSOR;
   WaitQueue16_32(6,8);
   SET_FRGD_COLOR(pGC->fgPixel);
   SET_PIX_CNTL(MIXSEL_EXPBLT | COLCMPOP_F);
   SET_FRGD_MIX(FSS_FRGDCOL | s3alu[pGC->alu]);
   if (s3Trio32FCBug) {
      SET_BKGD_MIX(BSS_BKGDCOL | ROP_DSo);
      SET_WRT_MASK(pGC->planemask);

      WaitQueue16_32(1,2);
      SET_BKGD_COLOR(0);
   } else {
      SET_BKGD_MIX(BSS_BKGDCOL | ROP_D);
      SET_WRT_MASK(pGC->planemask);
   }

   for (; --numRects >= 0; ++pBox) {
      WaitQueue(4);
	  SET_SCISSORS((short)pBox->x1, (short)pBox->y1, (short)(pBox->x2 - 1),
      												(short)(pBox->y2 - 1));

      Dos3CPolyText8(x, y, count, chars, fentry, pGC, pBox);
   }

   WaitQueue(4);
   SET_SCISSORS(0,0,(s3DisplayWidth - 1), s3ScissB);

   WaitQueue16_32(5,6);
   SET_RD_MASK(~0);
   SET_PIX_CNTL(MIXSEL_FRGDMIX | COLCMPOP_F);
   SET_MIX(FSS_FRGDCOL | ROP_S, BSS_BKGDCOL | ROP_S);
   UNBLOCK_CURSOR;

   return;
}
