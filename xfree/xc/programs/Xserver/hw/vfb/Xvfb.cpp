.\" $TOG: Xvfb.man /main/12 1998/02/10 13:24:06 kaleb $
.\" Copyright 1993, 1998  The Open Group
.\" 
.\" All Rights Reserved.
.\" 
.\" The above copyright notice and this permission notice shall be included
.\" in all copies or substantial portions of the Software.
.\" 
.\" THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
.\" OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
.\" MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
.\" IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
.\" OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
.\" ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
.\" OTHER DEALINGS IN THE SOFTWARE.
.\" 
.\" Except as contained in this notice, the name of The Open Group shall
.\" not be used in advertising or otherwise to promote the sale, use or
.\" other dealings in this Software without prior written authorization
.\" from The Open Group.
.\"
.\" $XFree86: xc/programs/Xserver/hw/vfb/Xvfb.cpp,v 1.2 2000/12/12 18:06:51 dawes Exp $
.\"
.TH XVFB 1 "Release 6.4" "X Version 11"
.SH NAME
Xvfb \- virtual framebuffer X server for X Version 11
.SH SYNOPSIS
.B Xvfb
[ option ] ...
.SH DESCRIPTION
.I Xvfb
is an X server that can run on machines with no display hardware
and no physical input devices.  It emulates a dumb framebuffer using
virtual memory.
.PP
The primary use of this server was intended to be server testing.  The
mfb or cfb code for any depth can be exercised with this server
without the need for real hardware that supports the desired depths.
The X community has found many other novel uses for \fIXvfb\fP,
including testing clients against unusual depths and screen
configurations, doing batch processing with \fIXvfb\fP as a background
rendering engine, load testing, as an aid to porting the X server to a
new platform, and providing an unobtrusive way to run applications
that don't really need an X server but insist on having one anyway.
.SH BUILDING
To build \fIXvfb\fP, put the following in your host.def and remake.
.PP
#define BuildServer YES /* if you aren't already building other servers */
.br
#define XVirtualFramebufferServer YES

.SH OPTIONS
.PP
In addition to the normal server options described in the \fIXserver(1)\fP
manual page, \fIXvfb\fP accepts the following command line switches:
.TP 4
.B "\-screen \fIscreennum\fP \fIWxHxD\fP"
This option creates screen \fIscreennum\fP and sets its width, height,
and depth to W, H, and D respectively.  By default, only screen 0 exists
and has the dimensions 1280x1024x8.
.TP 4
.B "\-pixdepths \fIlist-of-depths\fP"
This option specifies a list of pixmap depths that the server should
support in addition to the depths implied by the supported screens.
\fIlist-of-depths\fP is a space-separated list of integers that can
have values from 1 to 32.
.TP 4
.B "\-fbdir \fIframebuffer-directory\fP"
This option specifies the directory in which the memory mapped files
containing the framebuffer memory should be created.
See FILES. 
This option only exists on machines that have the mmap and msync system
calls.
.TP 4
.B "\-shmem"
This option specifies that the framebuffer should be put in shared memory.
The shared memory ID for each screen will be printed by the server.
The shared memory is in xwd format.
This option only exists on machines that support the System V shared memory
interface.
.PP
If neither \fB\-shmem\fP nor \fB\-fbdir\fP is specified,
the framebuffer memory will be allocated with malloc().
.TP 4
.B "\-linebias \fIn\fP"
This option specifies how to adjust the pixelization of thin lines.
The value \fIn\fP is a bitmask of octants in which to prefer an axial
step when the Bresenham error term is exactly zero.  See the file
Xserver/mi/miline.h for more information.  This option is probably only useful
to server developers to experiment with the range of line pixelization
possible with the cfb and mfb code.
.TP 4
.B "\-blackpixel \fIpixel-value\fP, \-whitepixel \fIpixel-value\fP"
These options specify the black and white pixel values the server should use.
.SH FILES
The following files are created if the \-fbdir option is given.
.TP 4
\fIframebuffer-directory\fP/Xvfb_screen<n>
Memory mapped file containing screen n's framebuffer memory, one file
per screen.  The file is in xwd format.  Thus, taking a full-screen
snapshot can be done with a file copy command, and the resulting
snapshot will even contain the cursor image.
.SH EXAMPLES
.TP 8
Xvfb :1 -screen 0 1600x1200x32
The server will listen for connections as server number 1, and screen 0
will be depth 32 1600x1200.
.TP 8
Xvfb :1 -screen 1 1600x1200x16
The server will listen for connections as server number 1, will have the
default screen configuration (one screen, 1280x1024x8), and screen 1
will be depth 16 1600x1200.
.TP 8
Xvfb -pixdepths 3 27 -fbdir /usr/tmp
The server will listen for connections as server number 0, will have the
default screen configuration (one screen, 1280x1024x8),
will also support pixmap
depths of 3 and 27, 
and will use memory mapped files in /usr/tmp for the framebuffer.
.TP 8
xwud -in /usr/tmp/Xvfb_screen0
Displays screen 0 of the server started by the preceding example.
.SH "SEE ALSO"
.PP
X(__miscmansuffix__), Xserver(1), xwd(1), xwud(1), XWDFile.h
.SH AUTHORS
David P. Wiggins, The Open Group, Inc.
