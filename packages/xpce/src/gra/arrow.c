/*  $Id$ $

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1992 University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>
#include <h/graphics.h>
#include "math.h"

static status	drawArrow(int x1, int y1, int x2, int y2, int x3, int y3,
			  Image fill, int pen, Name texture, Name style);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Arrows are defined in terms of the point at the tip the length of the arrow 
head and the width of the two wings.  The direction of the arrow is 
determined by a reference point on an imaginery line through the reference 
point and the tip:


	         -     |\
	         |     |  \
	         |     |    \
	wing >=  |     |      \    <= tip (point)
	         |     |      /
	         |     |    /
	         |     |  /
	         -     |/

	               |------|    <= length

Although arrows are a sub-class of graphical this feature (in particular the 
area of the graphical) should not be used.  Changing the area of an arrow 
has no well-defined meaning.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static status
initialiseArrow(Arrow a, Int length, Int wing)
{ initialiseGraphical(a, ZERO, ZERO, ONE, ONE);
  assign(a, length, length);
  assign(a, wing, wing);
  assign(a, tip, newObject(ClassPoint, toInt(10), toInt(10), 0));
  assign(a, reference, newObject(ClassPoint, 0));
  assign(a, left, newObject(ClassPoint, 0));
  assign(a, right, newObject(ClassPoint, 0));
  assign(a, fill_pattern, DEFAULT);
  assign(a, style, DEFAULT);
  obtainResourcesObject(a);

  requestComputeGraphical(a, DEFAULT);

  succeed;
}


status
computeArrow(Arrow a)
{ if ( notNil(a->request_compute) )
  { int x1, y1, x2, y2;
    int x, y, w, h;
    int sx, sy, rx, ry;
    int xdiff, ydiff;
    int cdl1, sdl1, cl2, sl2;
    float l1, l2, d;
    float sin_theta, cos_theta;

    x1 = valInt(a->reference->x);
    y1 = valInt(a->reference->y);
    x2 = valInt(a->tip->x);
    y2 = valInt(a->tip->y);

    l1 = (float) (valInt(a->length));
    l2 = (float) (valInt(a->wing))/2.0;

    xdiff = x2 - x1;
    ydiff = y2 - y1;

    d = sqrt((float) (xdiff*xdiff + ydiff*ydiff));
    if (d < 0.0000001)
    { cos_theta = 1.0;
      sin_theta = 0.0;
    } else
    { cos_theta = (float) xdiff / d;
      sin_theta = (float) ydiff / d;
    }

    cdl1 = rfloat(cos_theta * (d-l1));
    sdl1 = rfloat(sin_theta * (d-l1));
    cl2 = rfloat(cos_theta * l2);
    sl2 = rfloat(sin_theta * l2);

    sx = x1 + cdl1 - sl2;
    sy = y1 + sdl1 + cl2;
    rx = x1 + cdl1 + sl2;
    ry = y1 + sdl1 - cl2;

    assign(a->left, x, toInt(sx));
    assign(a->left, y, toInt(sy));
    assign(a->right, x, toInt(rx));
    assign(a->right, y, toInt(ry));

    x = min(x2, min(sx, rx));
    y = min(y2, min(sy, ry));
    w = max(x2, max(sx, rx)) - x + 1;
    h = max(y2, max(sy, ry)) - y + 1;

    if ( notNil(a->device) )
      setGraphical(a, toInt(x), toInt(y), toInt(w), toInt(h));
    else
      setArea(a->area, toInt(x), toInt(y), toInt(w), toInt(h));

    assign(a, request_compute, NIL);
  }

  succeed;
}


static status
RedrawAreaArrow(Arrow a, Area area)
{ int x, y;
  int x1, y1, x2, y2, x3, y3;

  offsetDeviceGraphical(a, &x, &y);

  x1 = valInt(a->left->x) + x;
  y1 = valInt(a->left->y) + y;
  x2 = valInt(a->tip->x) + x;
  y2 = valInt(a->tip->y) + y;
  x3 = valInt(a->right->x) + x;
  y3 = valInt(a->right->y) + y;

  drawArrow(x1, y1, x2, y2, x3, y3, a->fill_pattern, valInt(a->pen), 
	    a->texture, a->style);

  return RedrawAreaGraphical(a, area);
}


static status
drawArrow(int x1, int y1, int x2, int y2, int x3, int y3,
	  Image fill, int pen, Name texture, Name style)
{ struct ipoint pts[3];

  if ( notNil(fill) )
  { pts[0].x = x1;
    pts[0].y = y1;
    pts[1].x = x2;
    pts[1].y = y2;
    pts[2].x = x3;
    pts[2].y = y3;
  
    r_fillpattern(fill);
    r_fill_polygon(pts, 3);
  } else
  { r_dash(texture);
    r_thickness(pen);
    r_line(x1, y1, x2, y2);
    r_line(x2, y2, x3, y3);
    if ( style == NAME_closed )
      r_line(x3, y3, x1, y1);
  }

  succeed;
}


static status
tipArrow(Arrow a, Point p)
{ return pointsArrow(a, p->x, p->y, DEFAULT, DEFAULT);
}

static status
tipXArrow(Arrow a, Int x)
{ return pointsArrow(a, x, DEFAULT, DEFAULT, DEFAULT);
}

static status
tipYArrow(Arrow a, Int y)
{ return pointsArrow(a, DEFAULT, y, DEFAULT, DEFAULT);
}


static status
referenceArrow(Arrow a, Point p)
{ return pointsArrow(a, DEFAULT, DEFAULT, p->x, p->y);
}

static status
referenceXArrow(Arrow a, Int x)
{ return pointsArrow(a, DEFAULT, DEFAULT, x, DEFAULT);
}

static status
referenceYArrow(Arrow a, Int y)
{ return pointsArrow(a, DEFAULT, DEFAULT, DEFAULT, y);
}


static Int
getTipXArrow(Arrow a)
{ answer(a->tip->x);
}

static Int
getTipYArrow(Arrow a)
{ answer(a->tip->y);
}

static Int
getReferenceXArrow(Arrow a)
{ answer(a->reference->x);
}


static Int
getReferenceYArrow(Arrow a)
{ answer(a->reference->y); 
}


static status
styleArrow(Arrow a, Name style)
{ if ( a->style != style )
  { CHANGING_GRAPHICAL(a,
	assign(a, style, style);
	changedEntireImageGraphical(a));
  }
  succeed;
}


static status
lengthArrow(Arrow a, Int l)
{ if ( a->length != l )
  { CHANGING_GRAPHICAL(a,
	assign(a, length, l);
	computeArrow(a);
	changedEntireImageGraphical(a));
  }
  succeed;
}


static status
wingArrow(Arrow a, Int w)
{ if (a->wing != w)
  { CHANGING_GRAPHICAL(a,
	assign(a, wing, w);
	computeArrow(a);
	changedEntireImageGraphical(a));
  }
  succeed;
}


status
pointsArrow(Arrow a, Int tx, Int ty, Int rx, Int ry)
{ Point tip = a->tip;
  Point ref = a->reference;
  
  if (isDefault(tx)) tx = tip->x;
  if (isDefault(ty)) ty = tip->y;
  if (isDefault(rx)) rx = ref->x;
  if (isDefault(ry)) ry = ref->y;

  if (tx != tip->x || ty != tip->y || rx != ref->x || ry != ref->y)
  { assign(tip, x, tx);
    assign(tip, y, ty);
    assign(ref, x, rx);
    assign(ref, y, ry);
    requestComputeGraphical(a, DEFAULT);
  }
  
  succeed;
}


status
paintArrow(Arrow a, Int tx, Int ty, Int rx, Int ry)
{ pointsArrow(a, tx, ty, rx, ry);
  computeArrow(a);

  drawArrow(valInt(a->left->x), valInt(a->left->y),
	    valInt(a->tip->x), valInt(a->tip->y),
	    valInt(a->right->x), valInt(a->right->y),
	    a->fill_pattern, valInt(a->pen), a->texture,
	    a->style);

  succeed;
}


		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declaractions */

static char *T_initialise[] =
        { "length=[int]", "wing=[int]" };
static char *T_points[] =
        { "tip_x=[int]", "tip_y=[int]", "reference_x=[int]", "reference_y=[int]" };

/* Instance Variables */

static vardecl var_arrow[] =
{ SV(NAME_tip, "point", IV_GET|IV_STORE, tipArrow,
     NAME_area, "Tip of the arrow"),
  SV(NAME_reference, "point", IV_GET|IV_STORE, referenceArrow,
     NAME_area, "Where arrow points to"),
  SV(NAME_length, "int", IV_GET|IV_STORE, lengthArrow,
     NAME_area, "Distance tip to base"),
  SV(NAME_wing, "int", IV_GET|IV_STORE, wingArrow,
     NAME_area, "Length of base"),
  SV(NAME_fillPattern, "image|colour*", IV_GET|IV_STORE, fillPatternGraphical,
     NAME_appearance, "How it is filled"),
  SV(NAME_style, "{open,closed}", IV_GET|IV_STORE, styleArrow,
     NAME_appearance, "If `closed', the triangle is closed"),
  IV(NAME_left, "point", IV_NONE,
     NAME_internal, "Left-end of base"),
  IV(NAME_right, "point", IV_NONE,
     NAME_internal, "Right-end of base")
};

/* Send Methods */

static senddecl send_arrow[] =
{ SM(NAME_compute, 0, NULL, computeArrow,
     DEFAULT, "Compute <-tip, <-left and <-right"),
  SM(NAME_initialise, 2, T_initialise, initialiseArrow,
     DEFAULT, "Create from length and wing"),
  SM(NAME_points, 4, T_points, pointsArrow,
     NAME_area, "Set XY of tip and reference"),
  SM(NAME_referenceX, 1, "int", referenceXArrow,
     NAME_area, "Set X of reference"),
  SM(NAME_referenceY, 1, "int", referenceYArrow,
     NAME_area, "Set Y of reference"),
  SM(NAME_tipX, 1, "int", tipXArrow,
     NAME_area, "Set X of tip"),
  SM(NAME_tipY, 1, "int", tipYArrow,
     NAME_area, "Set Y of tip"),
  SM(NAME_DrawPostScript, 0, NULL, drawPostScriptArrow,
     NAME_postscript, "Create PostScript")
};

/* Get Methods */

static getdecl get_arrow[] =
{ GM(NAME_referenceX, 0, "int", NULL, getReferenceXArrow,
     NAME_area, "X of reference point"),
  GM(NAME_referenceY, 0, "int", NULL, getReferenceYArrow,
     NAME_area, "Y of reference point"),
  GM(NAME_tipX, 0, "int", NULL, getTipXArrow,
     NAME_area, "X of tip point"),
  GM(NAME_tipY, 0, "int", NULL, getTipYArrow,
     NAME_area, "Y of tip  point")
};

/* Resources */

static resourcedecl rc_arrow[] =
{ RC(NAME_fillPattern, "image|colour", "@black_image",
     "Fill pattern for the triangle"),
  RC(NAME_length, "int", "10",
     "Distance tip to base (10)"),
  RC(NAME_style, "{open,closed}", "closed",
     "Whether or not the wing is closed"),
  RC(NAME_selectionHandles, RC_REFINE, "corner_handles",
     NULL),
  RC(NAME_wing, "int", "7",
     "Width of wing (7)")
};

/* Class Declaration */

static Name arrow_termnames[] = { NAME_length, NAME_wing };

ClassDecl(arrow_decls,
          var_arrow, send_arrow, get_arrow, rc_arrow,
          2, arrow_termnames,
          "$Rev$");


status
makeClassArrow(Class class)
{ declareClass(class, &arrow_decls);

  cloneStyleVariableClass(class, NAME_fillPattern, NAME_reference);
  setRedrawFunctionClass(class, RedrawAreaArrow);

  succeed;
}

