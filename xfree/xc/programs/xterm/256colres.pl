#! /usr/bin/perl
# Author: Thomas E. Dickey
# $XFree86: xc/programs/xterm/256colres.pl,v 1.5 2000/06/17 00:27:35 dawes Exp $

# Construct a header file defining default resources for the 256-color model
# of xterm.  This is modeled after the 256colors2.pl script.

# use the resources for colors 0-15 - usually more-or-less a
# reproduction of the standard ANSI colors, but possibly more
# pleasing shades

print <<EOF;
/*
 * This header file was generated by $0
 */
/* \$XFree86\$ */
EOF

$line1="COLOR_RES(\"color%d\",";
$line2="\tscreen.Acolors[%d],";
$line3="\tDFT_COLOR(\"rgb:%2.2x/%2.2x/%2.2x\")),\n";

# colors 16-231 are a 6x6x6 color cube
for ($red = 0; $red < 6; $red++) {
    for ($green = 0; $green < 6; $green++) {
	for ($blue = 0; $blue < 6; $blue++) {
	    $code = 16 + ($red * 36) + ($green * 6) + $blue;
	    printf($line1, $code);
	    printf($line2, $code);
	    printf($line3,
		   int ($red * 42.5),
		   int ($green * 42.5),
		   int ($blue * 42.5));
	}
    }
}

# colors 232-255 are a grayscale ramp, intentionally leaving out
# black and white
$code=232;
for ($gray = 0; $gray < 24; $gray++) {
    $level = ($gray * 10) + 8;
    $code = 232 + $gray;
    printf($line1, $code);
    printf($line2, $code);
    printf($line3,
	   $level, $level, $level);
}
