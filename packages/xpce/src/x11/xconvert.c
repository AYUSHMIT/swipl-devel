/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1992 University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>
#include <h/graphics.h>
#include "include.h"
#include <memory.h>

#define O_GIFTOXPM 1

#ifdef HAVE_LIBXPM
#include <X11/xpm.h>
static XImage *readXpmFile(Image image, IOSTREAM *fd);
#ifdef O_GIFTOXPM
#include <img/gif.h>
static XImage *readGIFFile(Image image, IOSTREAM *fd);
#endif
#include <img/jpeg.h>
#ifdef HAVE_LIBJPEG
static XImage *readJPEGFile(Image image, IOSTREAM *fd);
#endif
#endif


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Read various bitmap-formats and  convert them into  an X11 bitmap-data
string to be used with XCreateBitmapFromData().  Functions provided:

char *read_bitmap_file(IOSTREAM *fd, int *w, int *h)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static unsigned char *read_x11_bitmap_file(IOSTREAM *, int *, int *);
static unsigned char *read_sun_icon_file(IOSTREAM *, int *, int *);

#define Round(n, r) ((((n) + (r) - 1) / (r)) * (r))

static unsigned char *
read_bitmap_data(IOSTREAM *fd, int *w, int *h)
{ long offset = Stell(fd);
  unsigned char *rval;

  if ( (rval = read_x11_bitmap_file(fd, w, h)) != NULL )
    return rval;
  Sseek(fd, offset, SIO_SEEK_SET);
  if ( (rval = read_sun_icon_file(fd, w, h)) != NULL )
    return rval;
  Sseek(fd, offset, SIO_SEEK_SET);

  return NULL;
}

		/********************************
		*          ENTRY POINT		*
		********************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
We should pass the display into these functions!
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static Display *
defaultXDisplay()
{ DisplayObj d = CurrentDisplay(NIL);
  DisplayWsXref r;

  openDisplay(d);
  r = d->ws_ref;

  return r->display_xref;
}


XImage *
CreateXImageFromData(unsigned char *data, int width, int height)
{ Display *disp = defaultXDisplay();
  XImage *image;

  image = XCreateImage(disp,
		       DefaultVisual(disp, DefaultScreen(disp)),
		       1,
		       XYBitmap,
		       0,
		       data,
		       width, height,
		       8, (width+7)/8);

  image->bits_per_pixel = 1;
  image->byte_order = LSBFirst;
  image->bitmap_unit = 8;
  image->bitmap_bit_order = LSBFirst;
  image->bitmap_pad = 8;

  return image;
}


XImage *
readImageFile(Image image, IOSTREAM *fd)
{ unsigned char *data;
  int w, h;
  XImage *img=NULL;

  if ( (data = read_bitmap_data(fd, &w, &h)) != NULL )
    return CreateXImageFromData(data, w, h);
#ifdef HAVE_LIBXPM
#ifdef HAVE_LIBJPEG
  if ( (img=readJPEGFile(image, fd)) )
    return img;
#endif  
#ifdef O_GIFTOXPM
  if ( (img=readGIFFile(image, fd)) )
    return img;
#endif  
  if ( (img=readXpmFile(image, fd)) )
    return img;
#endif

  return img;
}


		/********************************
		*         X10/X11 FORMAT	*
		********************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
The code below is copied from the MIT X11R5 distribution and modified for
the interface required in XPCE.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define MAX_SIZE 255

/* shared data for the image read/parse logic */
static short hexTable[256];		/* conversion value */
static int initialized = FALSE;		/* easier to fill in at run time */

#define HT_NODIGIT (-2)

/*
 *	Table index for the hex values. Initialized once, first time.
 *	Used for translation value or delimiter significance lookup.
 */
static void
initHexTable(void)
{ int i;

  for(i=0; i<(sizeof(hexTable)/sizeof(short)); i++)
    hexTable[i] = -HT_NODIGIT;

  hexTable['0'] = 0;	hexTable['1'] = 1;
  hexTable['2'] = 2;	hexTable['3'] = 3;
  hexTable['4'] = 4;	hexTable['5'] = 5;
  hexTable['6'] = 6;	hexTable['7'] = 7;
  hexTable['8'] = 8;	hexTable['9'] = 9;
  hexTable['A'] = 10;	hexTable['B'] = 11;
  hexTable['C'] = 12;	hexTable['D'] = 13;
  hexTable['E'] = 14;	hexTable['F'] = 15;
  hexTable['a'] = 10;	hexTable['b'] = 11;
  hexTable['c'] = 12;	hexTable['d'] = 13;
  hexTable['e'] = 14;	hexTable['f'] = 15;

  /* delimiters of significance are flagged w/ negative value */
  hexTable[' '] = -1;	hexTable[','] = -1;
  hexTable['}'] = -1;	hexTable['\n'] = -1;
  hexTable['\t'] = -1;
	
  initialized = TRUE;
}

/*
 *	read next hex value in the input stream, return -1 if EOF
 */

static int
NextInt(IOSTREAM *fstream)
{ int value = 0;
  int gotone = 0;
  int done = 0;
    
    /* loop, accumulate hex value until find delimiter  */
    /* skip any initial delimiters found in read stream */

  while (!done)
  { int ch = Sgetc(fstream);

    if ( ch == EOF )
    { return -1;
    } else
    { int dvalue = hexTable[ch];

      if ( dvalue >= 0 )
      { value = (value << 4) + dvalue;
	gotone++;
      } else if ((hexTable[ch]) == -1 && gotone)
	done++;
    }
  }

  return value;
}


static unsigned char *
read_x11_bitmap_file(IOSTREAM *fd, int *w, int *h)
{ unsigned char *data = NULL;
  char line[LINESIZE];
  int size = 0;
  char name_and_type[LINESIZE];		/* an input line */
  char *type;				/* for parsing */
  int value;				/* from an input line */
  int version10p;			/* bool, old format */
  int padding;				/* to handle alignment */
  int bytes_per_line;			/* per scanline of data */
  unsigned int ww = 0;			/* width */
  unsigned int hh = 0;			/* height */
  int hx = -1;				/* x hotspot */
  int hy = -1;				/* y hotspot */

  if (initialized == FALSE)
    initHexTable();

#define	RETURN_ERROR { if (data) XFree(data); return NULL; }

  while (Sfgets(line, LINESIZE, fd))
  { if ( sscanf(line,"#define %s %d",name_and_type,&value) == 2)
    { if (!(type = strrchr(name_and_type, '_')))
	type = name_and_type;
      else
	type++;

      if (!strcmp("width", type))
	ww = (unsigned int) value;
      if (!strcmp("height", type))
	hh = (unsigned int) value;
      if (!strcmp("hot", type))
      { if (type-- == name_and_type || type-- == name_and_type)
	  continue;
	if (!strcmp("x_hot", type))
	  hx = value;
	if (!strcmp("y_hot", type))
	  hy = value;
      }
      continue;
    }
    
    if (sscanf(line, "static short %s = {", name_and_type) == 1)
      version10p = 1;
    else if (sscanf(line,"static unsigned char %s = {",name_and_type) == 1)
      version10p = 0;
    else if (sscanf(line, "static char %s = {", name_and_type) == 1)
      version10p = 0;
    else
      continue;

    if (!(type = strrchr(name_and_type, '_')))
      type = name_and_type;
    else
      type++;

    if (strcmp("bits[]", type))
      continue;
    
    if (!ww || !hh)
      RETURN_ERROR;

    if ((ww % 16) && ((ww % 16) < 9) && version10p)
      padding = 1;
    else
      padding = 0;

    bytes_per_line = (ww+7)/8 + padding;

    size = bytes_per_line * hh;
    data = (unsigned char *) XMalloc(size);

    if (version10p)
    { unsigned char *ptr;
      int bytes;

      for (bytes=0, ptr=data; bytes<size; (bytes += 2))
      { if ((value = NextInt(fd)) < 0)
	  RETURN_ERROR;
	*(ptr++) = value;
	if (!padding || ((bytes+2) % bytes_per_line))
	  *(ptr++) = value >> 8;
      }
    } else
    { unsigned char *ptr;
      int bytes;
      
      for (bytes=0, ptr=data; bytes<size; bytes++, ptr++)
      { if ((value = NextInt(fd)) < 0) 
	  RETURN_ERROR;
	*ptr=value;
      }
    }
  }

  if (data == NULL) {
    RETURN_ERROR;
  }

  *w = ww;
  *h = hh;

  return data;
}


		/********************************
		*        SUN-ICON EDITOR	*
		********************************/

static unsigned char *
read_sun_icon_file(IOSTREAM *fd, int *width, int *height)
{ unsigned char *data, *dst;
  int c, x, y, w, h;
  int size;
  int skip_last;
  char hdr[256];

  if ( !Sfgets(hdr, sizeof(hdr), fd) ||
       sscanf(hdr, 
"/* Format_version=1, Width=%d, Height=%d, Depth=1, Valid_bits_per_item=16", 
	&w, &h) != 2 )
    return NULL;
  while( (c=Sgetc(fd)) != EOF && c != '/' ) /* skip to end of comment */
    ;

  if (initialized == FALSE)
    initHexTable();

  size = Round(w, 8) * h;
  dst = data = (unsigned char *) XMalloc(size);
  
  skip_last = (w % 16) <= 8 && (w % 16) > 0;

  for(y = 0; y < h; y++)
  { for(x = ((w+15)/16) - 1; x >= 0 ; x--)
    { int s;
      int d = 0;
      int n;
      
      s = NextInt(fd);

      for( n=0; n < 16 ; n++ )		/* revert all bits in the short */
        d |= ((s >> n) & 1) << (15-n);

      *dst++ = d & 0xff;
      if ( x != 0 || !skip_last )
	*dst++ = (d >> 8) & 0xff;
    }
  }

  *width = w;
  *height = h;
  return data;
}


#ifdef HAVE_LIBXPM
#include <sys/stat.h>

		 /*******************************
		 *          XPM SUPPORT		*
		 *******************************/

static void
setXpmAttributesImage(Image image, XImage *shape, XpmAttributes *atts)
{ if ( atts->valuemask & XpmHotspot )
    assign(image, hot_spot, newObject(ClassPoint,
				      toInt(atts->x_hotspot),
				      toInt(atts->y_hotspot), 0));
  else
    assign(image, hot_spot, NIL);

  if ( shape )
  { assign(image, mask, newObject(ClassImage, NIL,
				  toInt(shape->width),
				  toInt(shape->height),
				  NAME_bitmap, 0));
    setXImageImage(image->mask, shape);
  }
}


static XImage *
readXpmFile(Image image, IOSTREAM *fd)
{ int offset = Stell(fd);
  char *buffer = NULL;			/* make compiler happy */
  XImage *i = NULL;
  XImage *shape = NULL;
  int malloced = FALSE;
  Display *disp = defaultXDisplay();

  if ( offset == 0 )			/* only entire file for now */
  { int size;

    if ( (size = Ssize(fd)) >= 0 )
    { int as = XpmAttributesSize();
      XpmAttributes *atts = (XpmAttributes *)alloca(as);

      memset(atts, 0, as);
    
      if ( size < 10000 )
      { buffer = (char *)alloca(size+1);
      } else
      { buffer = pceMalloc(size+1);
	malloced = TRUE;
      }

      if ( Sfread(buffer, sizeof(char), size, fd) != size )
	goto out;

      buffer[size] = '\0';

      atts->exactColors = FALSE;
      atts->closeness   = (1<<16)-1;	/* always continue */
      atts->valuemask   = XpmExactColors|XpmCloseness;
      
      if ( XpmCreateImageFromBuffer(disp, buffer,
				    &i, &shape, atts) != XpmSuccess )
	i = NULL;

      setXpmAttributesImage(image, shape, atts);
    }
  }

out:
  if ( malloced )
    pceFree(buffer);
  if ( !i )
    Sseek(fd, offset, 0);

  return i;
}

#ifdef O_GIFTOXPM

static XImage *
readGIFFile(Image image, IOSTREAM *fd)
{ XpmImage img;

  switch( XpmReadGIF(fd, &img) )
  { case GIF_OK:
    { int as = XpmAttributesSize();
      XpmAttributes *atts = (XpmAttributes *)alloca(as);
      XImage *i = NULL;
      XImage *shape = NULL;
      Display *disp = defaultXDisplay();

      memset(atts, 0, as);
      atts->exactColors = FALSE;
      atts->closeness   = (1<<16)-1;	/* always continue */
      atts->valuemask   = XpmExactColors|XpmCloseness;

      if ( XpmCreateImageFromXpmImage(disp, &img, &i,
				      &shape, atts) != XpmSuccess )
	return NULL;
      setXpmAttributesImage(image, shape, atts);
      XpmFreeXpmImage(&img);

      return i;
    }
    case GIF_NOMEM:
    case GIF_INVALID:
    default:
      return NULL;
  }
}

#endif /*O_GIFTOXPM*/
#ifdef HAVE_LIBJPEG

static XImage *
readJPEGFile(Image image, IOSTREAM *fd)
{ XpmImage img;

  switch( readJPEGtoXpmImage(fd, &img) )
  { case GIF_OK:
    { int as = XpmAttributesSize();
      XpmAttributes *atts = (XpmAttributes *)alloca(as);
      XImage *i = NULL;
      XImage *shape = NULL;
      Display *disp = defaultXDisplay();

      memset(atts, 0, as);
      atts->exactColors = FALSE;
      atts->closeness   = (1<<16)-1;	/* always continue */
      atts->valuemask   = XpmExactColors|XpmCloseness;

      if ( XpmCreateImageFromXpmImage(disp, &img, &i,
				      &shape, atts) != XpmSuccess )
	return NULL;

      XpmFreeXpmImage(&img);

      return i;
    }
    case GIF_NOMEM:
    case GIF_INVALID:
    default:
      return NULL;
  }
}
#endif /*HAVE_LIBJPEG*/
#endif /*HAVE_LIBXPM*/
