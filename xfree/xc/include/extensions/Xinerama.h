/* $XFree86$ */

#ifndef _Xinerama_h
#define _Xinerama_h

typedef struct {
   int   screen_number;
   short x_org;
   short y_org;
   short width;
   short height;
} XineramaScreenInfo;

Bool XineramaQueryExtension (
   Display *dpy,
   int     *event_base,
   int     *error_base
);

Status XineramaQueryVersion(
   Display *dpy,
   int     *major,
   int     *minor
);

Bool XineramaIsActive(Display *dpy);


/* 
   Returns the number of heads and a pointer to an array of
   structures describing the position and size of the individual
   heads.  Returns NULL and number = 0 if Xinerama is not active.
  
   Returned array should be freed with XFree().
*/

XineramaScreenInfo * 
XineramaQueryScreens(
   Display *dpy,
   int     *number
);

#endif /* _Xinerama_h */

