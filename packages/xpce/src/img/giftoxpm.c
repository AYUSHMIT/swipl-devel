/*  $Id$

    Part of XPCE
    Designed and implemented by Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1997 University of Amsterdam. All rights reserved.
*/

#ifdef WIN32
#include <msw/include.h>
#define  FOR_MSW 1
#include <msw/xpm.h>
#else
#include <h/kernel.h>
#include <X11/xpm.h>
#endif

#include <stdio.h>
#include "gif.h"
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#define XpmMalloc(size) (void *)malloc((size))
#ifndef SEEK_SET
#define SEEK_SET 0
#endif

int
alloc_colortable(int ncolors, void *closure)
{ XpmImage *img = closure;

  img->ncolors    = ncolors;
  img->colorTable = XpmMalloc(sizeof(XpmColor) * ncolors);

  if ( img->colorTable )
  { memset(img->colorTable, 0, sizeof(XpmColor) * ncolors);

    return GIF_OK;
  }

  return GIF_NOMEM;
}


int
alloc_color(int index, int r, int g, int b, void *closure)
{ XpmImage *img = closure;
  XpmColor *c   = &img->colorTable[index];
  
  if ( (c->c_color = XpmMalloc(8)) )
  { sprintf(c->c_color, "#%02x%02x%02x", r, g, b);
    return GIF_OK;
  }

  return GIF_NOMEM;
}


int
XpmReadGIF(FILE *fd, XpmImage *img)
{ long here = ftell(fd);

  img->ncolors    = 0;
  img->colorTable = NULL;
  img->data       = NULL;

  switch( GIFReadFD(fd,
		    &img->data,
		    &img->width,
		    &img->height,
		    alloc_colortable,
		    alloc_color,
		    img) )
  { case GIF_OK:
      return XpmSuccess;
    case GIF_NOMEM:
      fseek(fd, here, SEEK_SET);
      return XpmNoMemory;
    case GIF_INVALID:
    default:
      fseek(fd, here, SEEK_SET);
      return XpmFileInvalid;
  }
}
