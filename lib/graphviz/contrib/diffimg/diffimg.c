/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at http://www.graphviz.org/
 *************************************************************************/

/*
 * This program generates an image where each pixel is the
 * difference between the corresponding pixel in each of the
 * two source images.  Thus, if the source images are the same
 * the resulting image will be black, otherwise it will have
 * regions of non-black where the images differ.
 *
 * Currently supports: .png, .gif, .jpg, and .ps by using ghostscript
 *
 * John Ellson <ellson@research.att.com>
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#define EX_USAGE		64
#define EX_DATAERR		65
#define EX_NOINPUT		66
#define EX_UNAVAILABLE	69
#define EX_OSERR	71
#else
#include <sysexits.h>
#endif
#include <gd.h>
#include <stdbool.h>
#include <cgraph/agxbuf.h>
#include <util/alloc.h>
#include <util/exit.h>

static char *pstopng="gs -dNOPAUSE -sDEVICE=pngalpha -sOutputFile=- -q -";

static gdImagePtr imageLoad (char *filename)
{
    FILE *f;
    char *ext;
    gdImagePtr im;
    int rc;
    struct stat statbuf;

    ext = strrchr(filename, '.');
    if (!ext) {
        fprintf(stderr, "Filename \"%s\" has no file extension.\n", filename);
        graphviz_exit(EX_USAGE);
    }
    rc = stat(filename, &statbuf);
    if (rc) {
	 fprintf(stderr, "Failed to stat \"%s\"\n", filename);
         graphviz_exit(EX_NOINPUT);
    }
    if (strcasecmp(ext, ".ps") == 0) {
	ext = ".png";
	agxbuf fname = {0};
	agxbprint(&fname, "%s%s", filename, ext);
	const char *tmp = agxbuse(&fname);
	
	agxbuf cmd = {0};
	agxbprint(&cmd, "%s <%s >%s", pstopng, filename, tmp);
	rc = system(agxbuse(&cmd));
	agxbfree(&cmd);
	
        f = fopen(tmp, "rb");
	agxbfree(&fname);
        if (!f) {
            fprintf(stderr, "Failed to open converted \"%s%s\"\n", filename, ext);
            graphviz_exit(EX_NOINPUT);
        }
    }
    else {
        f = fopen(filename, "rb");
        if (!f) {
            fprintf(stderr, "Failed to open \"%s\"\n", filename);
            graphviz_exit(EX_NOINPUT);
        }
    }
    im = 0;
    if (strcasecmp(ext, ".png") == 0) {
#ifdef HAVE_GD_PNG
        im = gdImageCreateFromPng(f);
#else
        fprintf(stderr, "PNG support is not available\n");
        graphviz_exit(EX_UNAVAILABLE);
#endif
    }
    else if (strcasecmp(ext, ".gif") == 0) {
#ifdef HAVE_GD_GIF
        im = gdImageCreateFromGif(f);
#else
        fprintf(stderr, "GIF support is not available\n");
        graphviz_exit(EX_UNAVAILABLE);
#endif
    }
    else if (strcasecmp(ext, ".jpg") == 0) {
#ifdef HAVE_GD_JPEG
        im = gdImageCreateFromJpeg(f);
#else
        fprintf(stderr, "JPEG support is not available\n");
        graphviz_exit(EX_UNAVAILABLE);
#endif
    }
    fclose(f);
    if (!im) {
        fprintf(stderr, "Loading image from file  \"%s\" failed!\n", filename);
        graphviz_exit(EX_DATAERR);
    }
    return im;
}

static bool imageDiff (gdImagePtr A, gdImagePtr B, gdImagePtr C,
	int w, int h,
	int black, int white)
{
    int x, y;
    bool d, rc;

    rc = false;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
	    d = (bool)( gdImageGetTrueColorPixel(B,x,y)
		      - gdImageGetTrueColorPixel(A,x,y));
            gdImageSetPixel (C, x, y, (d ? white : black));
	    rc |= d;
        }
    }
    return rc;
}

int main(int argc, char **argv)
{
    gdImagePtr A, B, C;
    int black, white;
    int minSX, minSY, maxSX, maxSY;
    bool rc;
#ifdef HAVE_GD_PNG
    FILE *f;
#endif

    if (argc == 2 && strcmp(argv[1], "-?") == 0) {
        fprintf(stderr, "Usage: diffimg image1 image2 [outimage]\n");
        graphviz_exit(0);
    }
    if (argc < 3) {
        fprintf(stderr, "Usage: diffimg image1 image2 [outimage]\n");
        graphviz_exit(EX_USAGE);
    }
    A = imageLoad(argv[1]);
    B = imageLoad(argv[2]);

    minSX = (gdImageSX(A) < gdImageSX(B)) ? gdImageSX(A) : gdImageSX(B);
    minSY = (gdImageSY(A) < gdImageSY(B)) ? gdImageSY(A) : gdImageSY(B);
    maxSX = (gdImageSX(A) > gdImageSX(B)) ? gdImageSX(A) : gdImageSX(B);
    maxSY = (gdImageSY(A) > gdImageSY(B)) ? gdImageSY(A) : gdImageSY(B);
    
    C = gdImageCreatePalette (maxSX, maxSY);

    white = gdImageColorAllocate(C, gdRedMax, gdGreenMax, gdBlueMax);
    black = gdImageColorAllocate(C, 0, 0, 0);

    if (maxSX > minSX && maxSY > minSY)
	gdImageFilledRectangle(C, minSX, minSY, maxSX-1, maxSY-1, black);

    rc = imageDiff (A, B, C, minSX, minSY, black, white);

#ifdef HAVE_GD_PNG
    if ((argc > 3) && ((f = fopen(argv[3], "wb")))) {
	gdImagePng (C, f);
	fclose(f);
    }
    else
        gdImagePng (C, stdout);
#else

    fprintf(stderr, "PNG output support is not available\n");
#endif

    gdImageDestroy(A);
    gdImageDestroy(B);
    gdImageDestroy(C);

    graphviz_exit(rc ? EXIT_FAILURE : EXIT_SUCCESS);
}

