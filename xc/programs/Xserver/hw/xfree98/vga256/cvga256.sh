#!/bin/sh

# $XFree86: xc/programs/Xserver/hw/xfree98/vga256/cvga256.sh,v 3.1 1996/02/04 09:16:59 dawes Exp $
#
# This script generates vga256Conf.c
#
# usage: confvga256.sh driver1 driver2 ...
#
# $XConsortium: cvga256.sh /main/2 1995/12/29 11:51:14 kaleb $

VGACONF=./vga256Conf.c

cat > $VGACONF <<EOF
/*
 * This file is generated automatically -- DO NOT EDIT
 */

#include "xf86.h"
#include "vga.h"

extern vgaVideoChipRec
EOF
Args="`echo $* | tr '[a-z]' '[A-Z]'`"
set - $Args
while [ $# -gt 1 ]; do
  echo "        $1," >> $VGACONF
  shift
done
echo "        $1;" >> $VGACONF
cat >> $VGACONF <<EOF

vgaVideoChipPtr vgaDrivers[] =
{
EOF
for i in $Args; do
  echo "        &$i," >> $VGACONF
done
echo "        NULL" >> $VGACONF
echo "};" >> $VGACONF
