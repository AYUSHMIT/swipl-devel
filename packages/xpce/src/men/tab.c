/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1996 University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>
#include <h/dialog.h>


		/********************************
		*            CREATE		*
		********************************/

static status
initialiseTab(Tab t, Name name)
{
  assign(t, label_size,   DEFAULT);	/* Resource */
  assign(t, label_offset, ZERO);
  assign(t, status,	  NAME_onTop);
  assign(t, size,	  DEFAULT);

  initialiseDialogGroup((DialogGroup) t, name, DEFAULT);

  succeed;
}

		 /*******************************
		 *	      COMPUTE		*
		 *******************************/

static status
computeTab(Tab t)
{ if ( notNil(t->request_compute) )
  { int x, y, w, h;
    Area a = t->area;

    obtainResourcesObject(t);
    computeGraphicalsDevice((Device) t);
    
    if ( isDefault(t->size) )		/* implicit size */
    { Cell cell;

      clearArea(a);
      for_cell(cell, t->graphicals)
      { Graphical gr = cell->value;
	
	unionNormalisedArea(a, gr->area);
      }
      relativeMoveArea(a, t->offset);

      w = valInt(a->w) + 2 * valInt(t->gap->w);
      h = valInt(a->h) + 2 * valInt(t->gap->h);
    } else				/* explicit size */
    { w = valInt(t->size->w);
      h = valInt(t->size->h);
    }

    h += valInt(t->label_size->h);
    x = valInt(t->offset->x);
    y = valInt(t->offset->y) - valInt(t->label_size->h);

    CHANGING_GRAPHICAL(t,
	assign(a, x, toInt(x));
	assign(a, y, toInt(y));
	assign(a, w, toInt(w));
	assign(a, h, toInt(h)));

    assign(t, request_compute, NIL);
  }

  succeed;
}

		 /*******************************
		 *	       GEOMETRY		*
		 *******************************/

static status
geometryTab(Tab t, Int x, Int y, Int w, Int h)
{ if ( notDefault(w) || notDefault(h) )
  { Any size;

    if ( isDefault(w) )
      w = getWidthGraphical((Graphical) t);
    if ( isDefault(h) )
      h = getHeightGraphical((Graphical) t);

    size = newObject(ClassSize, w, h, 0);
    qadSendv(t, NAME_size, 1, &size);
  }

  geometryDevice((Device) t, x, y, w, h);
  requestComputeGraphical(t, DEFAULT);

  succeed;
}

		 /*******************************
		 *	     NAME/LABEL		*
		 *******************************/

static status
ChangedLabelTab(Tab t)
{ Elevation e = getResourceValueObject(t, NAME_elevation);
  Int eh = e->height;

  assign(t, request_compute, ON);
  computeTab(t);

  return changedImageGraphical(t,
			       t->label_offset, ZERO,
			       t->label_size->w,
			       add(t->label_size->h, eh));
}


static status
labelOffsetTab(Tab t, Int offset)
{ if ( t->label_offset != offset )
  { int chl, chr;

    chl = valInt(t->label_offset);
    chr = chl + valInt(t->label_size->w);

    assign(t, label_offset, offset);
    if ( valInt(offset) < chl )
      chl = valInt(offset);		/* shift left */
    else
      chr = valInt(offset) + valInt(t->label_size->w); /* shift right */

    changedImageGraphical(t,
			  toInt(chl), ZERO,
			  toInt(chr), t->label_size->h);
  }

  succeed;
}


static status
statusTab(Tab t, Name stat)
{ return assignGraphical(t, NAME_status, stat);
}



		/********************************
		*             REDRAW		*
		********************************/

#define GOTO(p, a, b)	 p->x = (a), p->y = (b), p++
#define RMOVE(p, dx, dy) p->x = p[-1].x + (dx), p->y = p[-1].y + (dy), p++

static status
RedrawAreaTab(Tab t, Area a)
{ int x, y, w, h;
  Elevation e = getResourceValueObject(t, NAME_elevation);
  int lh      = valInt(t->label_size->h);
  int lw      = valInt(t->label_size->w);
  int loff    = valInt(t->label_offset);
  int eh      = valInt(e->height);
  int ex      = valInt(getExFont(t->label_font));

  initialiseDeviceGraphical(t, &x, &y, &w, &h);

  if ( t->status == NAME_onTop )
  { ipoint pts[8];
    IPoint p = pts;
    
    if ( loff == 0 )
    { GOTO(p, x, y);
    } else
    { GOTO(p, x, y+lh);
      RMOVE(p, loff, 0);
      RMOVE(p, 0, -lh);
    }
    RMOVE(p, lw, 0);
    RMOVE(p, 0, lh);
    GOTO(p, x+w, y+lh);
    RMOVE(p, 0, h-lh);
    RMOVE(p, -w, 0);

    r_3d_rectangular_polygon(p-pts, pts, e, DRAW_3D_FILLED|DRAW_3D_CLOSED);

    str_string(&t->label->data, t->label_font,
	       x+loff+ex, y, lw-2*ex, lh, t->label_format, NAME_center);

    { Cell cell;
      Int ax = a->x, ay = a->y;
      Point offset = t->offset;
      int ox = valInt(offset->x);
      int oy = valInt(offset->y);

      assign(a, x, toInt(valInt(a->x) - ox));
      assign(a, y, toInt(valInt(a->y) - oy));
      r_offset(ox, oy);

      d_clip(x+eh, y+eh, w-2*eh, h-2*eh); /* check if needed! */
      for_cell(cell, t->graphicals)
      { Graphical gr = cell->value;

	if ( gr->displayed == ON && overlapArea(a, gr->area) )
	  RedrawArea(gr, a);
      }
      d_clip_done();	     

      r_offset(-ox, -oy);
      assign(a, x, ax);
      assign(a, y, ay);
    }
  } else /* if ( t->status == NAME_hidden ) */
  { ipoint pts[4];
    IPoint p = pts;

    GOTO(p, x+loff, y+lh);
    RMOVE(p, 0, -lh);
    RMOVE(p, lw, 0);
    RMOVE(p, 0, lh);

    r_3d_rectangular_polygon(p-pts, pts, e, DRAW_3D_FILLED);

    str_string(&t->label->data, t->label_font,
	       x+loff+ex, y, lw-2*ex, lh, t->label_format, NAME_center);
  }

  return RedrawAreaGraphical(t, a);
}


		 /*******************************
		 *	       EVENT		*
		 *******************************/

static status
inEventAreaTab(Tab t, Int X, Int Y)
{ int x = valInt(X) - valInt(t->offset->x);
  int y = valInt(Y) - valInt(t->offset->y);

  if ( y < 0 )				/* tab-bar */
  { if ( y > -valInt(t->label_size->h) &&
	 x > valInt(t->label_offset) &&
	 x < valInt(t->label_offset) + valInt(t->label_size->w) )
      succeed;
  } else
  { if ( t->status == NAME_onTop )
      succeed;
  }

  fail;
}


static status
eventTab(Tab t, EventObj ev)
{ Int X, Y;
  int x, y;

  get_xy_event(ev, t, OFF, &X, &Y);
  x = valInt(X), y = valInt(Y);

  if ( y < 0 )				/* tab-bar */
  { if ( isDownEvent(ev) &&
	 y > -valInt(t->label_size->h) &&
	 x > valInt(t->label_offset) &&
	 x < valInt(t->label_offset) + valInt(t->label_size->w) )
    { send(t->device, NAME_onTop, t, 0); /* TBD: use gesture? */
      succeed;
    }

    fail;				/* pass to next one */
  }

  if ( t->status == NAME_onTop )
  { eventDialogGroup((DialogGroup) t, ev);
    succeed;				/* don't continue */
  }

  fail;
}


static status
flashTab(Tab t, Area a, Int time)
{ if ( notDefault(a) )
    return flashDevice((Device)t, a, DEFAULT);

  if ( t->status == NAME_onTop )
  { a = answerObject(ClassArea,
		     ZERO, ZERO,
		     t->area->w,
		     sub(t->area->h, t->label_size->h), 0);
  } else
  { a = answerObject(ClassArea,
		     t->label_offset, neg(t->label_size->h),
		     t->label_size->w, t->label_size->h, 0);
  }

  flashDevice((Device)t, a, DEFAULT);
  doneObject(a);
  succeed;
}

		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declaractions */

static char *T_geometry[] =
        { "x=[int]", "y=[int]", "width=[int]", "height=[int]" };
static char *T_flash[] =
	{ "area=[area]", "time=[int]" };

/* Instance Variables */

static vardecl var_tab[] =
{ IV(NAME_labelSize, "size", IV_GET,
     NAME_layout, "Size of the label-box"),
  SV(NAME_labelOffset, "int", IV_GET|IV_STORE, labelOffsetTab,
     NAME_layout, "X-Offset of label-box"),
  SV(NAME_status, "{on_top,hidden}", IV_GET|IV_STORE, statusTab,
     NAME_appearance, "Currently displayed status"),
  SV(NAME_labelFormat, "{left,center,right}", IV_GET|IV_STORE|IV_REDEFINE,
     labelFormatDialogGroup,
     NAME_appearance, "Alignment of label in box")
};

/* Send Methods */

static senddecl send_tab[] =
{ SM(NAME_initialise, 1, "name=[name]", initialiseTab,
     DEFAULT, "Create a new tab-entry"),
  SM(NAME_geometry, 4, T_geometry, geometryTab,
     DEFAULT, "Move/resize tab"),
  SM(NAME_event, 1, "event", eventTab,
     NAME_event, "Process event"),
  SM(NAME_flash, 2, T_flash, flashTab,
     NAME_report, "Flash tab or area"),
  SM(NAME_position, 1, "point", positionGraphical,
     NAME_geometry, "Top-left corner of tab"),
  SM(NAME_x, 1, "int", xGraphical,
     NAME_geometry, "Left-side of tab"),
  SM(NAME_y, 1, "int", yGraphical,
     NAME_geometry, "Top-side of tab"),
  SM(NAME_compute, 0, NULL, computeTab,
     NAME_update, "Recompute area"),
  SM(NAME_ChangedLabel, 0, NULL, ChangedLabelTab,
     NAME_update, "Add label-area to the update")
};

/* Get Methods */

static getdecl get_tab[] =
{ GM(NAME_position, 0, "point", NULL, getPositionGraphical,
     NAME_geometry, "Top-left corner of tab"),
  GM(NAME_x, 0, "int", NULL, getXGraphical,
     NAME_geometry, "Left-side of tab"),
  GM(NAME_y, 0, "int", NULL, getYGraphical,
     NAME_geometry, "Top-side of tab")
};

/* Resources */

static resourcedecl rc_tab[] =
{ RC(NAME_elevation, "elevation",
     "when(@colour_display, " /* concat */
           "1, " /* concat */
     	   "elevation(tab, 2, relief := @grey50_image, shadow := black))",
     "Elevation above environment"),
  RC(NAME_gap, "size", "size(15,8)",
     "Distance between items in X and Y"),
  RC(NAME_labelFont, "font", "normal",
     "Font used to display the label"),
  RC(NAME_labelFormat, "{left,center,right}", "left",
     "Alignment of label in box"),
  RC(NAME_labelSize, "size", "size(80, 20)",
     "Size of box for label")
};

/* Class Declaration */

ClassDecl(tab_decls,
          var_tab, send_tab, get_tab, rc_tab,
          ARGC_INHERIT, NULL,
          "$Rev$");


status
makeClassTab(Class class)
{ declareClass(class, &tab_decls);

  setRedrawFunctionClass(class, RedrawAreaTab);
  setInEventAreaFunctionClass(class, inEventAreaTab);

  succeed;
}

