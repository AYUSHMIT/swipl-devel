/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1992 University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>
#include <h/graphics.h>

static status	layoutTile(TileObj t, Int ax, Int ay, Int aw, Int ah);
static status	computeTile(TileObj t);

#define Max(a, b)	(valInt(a) > valInt(b) ? (a) : (b))
#define Min(a, b)	(valInt(a) < valInt(b) ? (a) : (b))
#define MAX_TILE_MEMBERS 200
#define INFINITE toInt(PCE_MAX_INT)


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
A tile manages a set of  non-overlapping areas.  It  is used to manage
the areas of the windows in a frame.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

		/********************************
		*         CREATE/DESTROY	*
		********************************/

static status
initialiseTile(TileObj t, Any object, Int w, Int h)
{ Int border = (Int) getResourceValueObject(t, NAME_border);

  if ( notNil(object) )
  { if ( isDefault(w) )
      w = get(object, NAME_width, 0);
    if ( isDefault(h) )
      h = get(object, NAME_height, 0);
  } 

  assign(t, enforced,	 OFF);		/* building stage */
  assign(t, border,      border);
  assign(t, idealWidth,  w);		/* ideal size */
  assign(t, idealHeight, h);
  assign(t, horStretch,  toInt(100));	/* stretchabilities */
  assign(t, horShrink,   toInt(100));
  assign(t, verStretch,  toInt(100));
  assign(t, verShrink,   toInt(100));
  assign(t, orientation, NAME_none);
  assign(t, members, NIL);		/* subtiles */
  assign(t, super,   NIL);		/* super-tile */
  assign(t, object,  object);		/* managed object */
					/* Actual area */
  assign(t, area, newObject(ClassArea, ZERO, ZERO, w, h, 0));
  
  succeed;
}


static status
unlinkTile(TileObj t)
{ if ( notNil(t->members) )
  { clearChain(t->members);
    assign(t, members, NIL);
  }

  succeed;
}

		/********************************
		*         UNRELATE TILE		*
		********************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
A member of this tile has been deleted.  If there is only one member
left, remove this tile for the hierarchy.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static status
cleanTile(TileObj t)
{ if ( t->members->size == ONE )
  { TileObj child = getHeadChain(t->members);
      
    if ( notNil(t->super) )
    { TileObj super = t->super;

      replaceChain(super->members, t, child);
      assign(child, super, super);
    } else
    { assign(child, super, NIL);
      freeObject(t);
    }

    computeTile(getRootTile(child));
  } else
    computeTile(t);

  succeed;
}


status
unrelateTile(TileObj t)
{ if ( notNil(t->super) )
  { TileObj super = t->super;

    deleteChain(t->super->members, t);
    assign(t, super, NIL);
    cleanTile(super);
  }

  succeed;
}

		/********************************
		*    CREATING TILE HIERARCHY	*
		********************************/

static TileObj
toTile(Any obj)
{ if ( instanceOfObject(obj, ClassTile) )
    return (TileObj) obj;

  return answerObject(ClassTile, obj, 0);
}


static status
computeTile(TileObj t)
{ if ( equalName(t->orientation, NAME_horizontal) )
  { Cell cell;
    Int w   = ZERO;
    Int h   = ZERO;
    Int hsh = ZERO;
    Int hst = ZERO;
    Int vsh = INFINITE;
    Int vst = INFINITE;
    
    for_cell(cell, t->members)
    { TileObj t2 = cell->value;
      
      w = add(w, t2->idealWidth);
      h = Max(h, t2->idealHeight);
      hsh = Max(hsh, t2->horShrink);
      hst = Max(hst, t2->horStretch);
      vsh = Min(vsh, t2->verShrink);
      vst = Min(vst, t2->verStretch);
      w = add(w, t->border);
    }
    
    assign(t, idealWidth,  w);
    assign(t, idealHeight, h);
    assign(t, horShrink,   hsh);
    assign(t, horStretch,  hst);
    assign(t, verShrink,   vsh);
    assign(t, verStretch,  vst);
  } else if ( equalName(t->orientation, NAME_vertical) )
  { Cell cell;
    Int w   = ZERO;
    Int h   = ZERO;
    Int vsh = ZERO;
    Int vst = ZERO;
    Int hsh = INFINITE;
    Int hst = INFINITE;
    
    for_cell(cell, t->members)
    { TileObj t2 = cell->value;
      
      w = Max(w, t2->idealWidth);
      h = add(h, t2->idealHeight);
      hsh = Min(hsh, t2->horShrink);
      hst = Min(hst, t2->horStretch);
      vsh = Max(vsh, t2->verShrink);
      vst = Max(vst, t2->verStretch);
      h = add(h, t->border);
    }
    
    assign(t, idealWidth,  w);
    assign(t, idealHeight, h);
    assign(t, horShrink,   hsh);
    assign(t, horStretch,  hst);
    assign(t, verShrink,   vsh);
    assign(t, verStretch,  vst);
  }
  
  succeed;
}


status
leftTile(TileObj t, Any obj)
{ TileObj t2 = toTile(obj);
  TileObj super;
					/* One already has a super tile */
  if ( notNil(t->super)  &&
       (t->super->orientation  == NAME_vertical ||
	notNil(t->super->super)) )
    return leftTile(t->super, t2);
  if ( notNil(t2->super) &&
       (t2->super->orientation == NAME_vertical ||
	notNil(t2->super->super)) )
    return leftTile(t, t2->super);
					/* both left-to-right */
  if ( notNil(t->super) && notNil(t2->super) )
    return leftTile(t->super, t2->super);

  if ( notNil(t->super) )
  { super = t->super;
    appendChain(super->members, t2);
  } else if ( notNil(t2->super) )
  { super = t2->super;
    prependChain(super->members, t);
  } else
  { super = newObject(ClassTile, NIL, ZERO, ZERO, 0);
    assign(super, orientation, NAME_horizontal);
    assign(super, members, newObject(ClassChain, t, t2, 0));
    assign(super->area, x, t->area->x);
    assign(super->area, y, t->area->y);
  }

  assign(t,  super, super);
  assign(t2, super, super);
  computeTile(super);

  succeed;
}


status
rightTile(TileObj t, Any obj)
{ return leftTile(toTile(obj), t);
}


status
aboveTile(TileObj t, Any obj)
{ TileObj t2 = toTile(obj);
  TileObj super;
					/* One already has a super tile */
  if ( notNil(t->super)  &&
       (t->super->orientation  == NAME_horizontal ||
	notNil(t->super->super)) )
    return aboveTile(t->super, t2);
  if ( notNil(t2->super) &&
       (t2->super->orientation == NAME_horizontal ||
	notNil(t2->super->super)) )
    return aboveTile(t, t2->super);

  if ( notNil(t->super) && notNil(t2->super) )
    return aboveTile(t->super, t2->super);

  if ( notNil(t->super) )
  { super = t->super;
    appendChain(super->members, t2);
  } else if ( notNil(t2->super) )
  { super = t2->super;
    prependChain(super->members, t);
  } else
  { super = newObject(ClassTile, NIL, ZERO, ZERO, 0);
    assign(super, orientation, NAME_vertical);
    assign(super, members, newObject(ClassChain, t, t2, 0));
    assign(super->area, x, t->area->x);
    assign(super->area, y, t->area->y);
  }

  assign(t,  super, super);
  assign(t2, super, super);

  computeTile(super);

  succeed;
}


status
belowTile(TileObj t, Any obj)
{ return aboveTile(toTile(obj), t);
}


		/********************************
		*        LAYOUT MANAGEMENT	*
		********************************/


TileObj
getRootTile(TileObj t)
{ while(notNil(t->super))
    t = t->super;

  answer(t);
}


typedef struct
{ int	ideal;				/* ideal size */
  int	stretch;			/* stretch handicap */
  int	shrink;				/* shrink handicap */
  int	size;				/* resulting size */
} stretch;


static status
distribute_stretches(stretch *s, int n, int w)
{ int ok = FALSE;
  int i;
  int maxloop = n;
  
  if ( w <= 0 )
  { for(i=0; i<n; i++)
      s[i].size = 0;

    succeed;
  }

  while( !ok && maxloop-- > 0 )
  { int total_ideal = 0, total_stretch = 0, total_shrink = 0;
    int grow, growed;
    int is_pos;
    
    for(i = 0; i < n; i++)
    { total_ideal   += s[i].ideal;
      total_stretch += s[i].stretch;
      total_shrink  += s[i].shrink;
      DEBUG(NAME_tile, Cprintf("%-2d %-3d <- %-3d -> %-3d\n",
			       i, s[i].shrink, s[i].ideal, s[i].stretch));
    }

    grow = w - total_ideal;

    if ( grow < 0 && total_shrink == 0 )
    { for(is_pos = 0, i = 0; i < n; i++)
        if ( s[i].ideal > 0 || s[i].shrink > 0 )
	  is_pos++;
    } else
      is_pos = n;

    DEBUG(NAME_tile, Cprintf("grow = %d, is_pos = %d\n", grow, is_pos));

    for(growed = 0, i = 0; i < n; i++)
    { int grow_this;

      if ( grow >= 0 )
      { grow_this = (total_stretch==0 ? grow / n
				      : (grow * s[i].stretch) / total_stretch);
      } else
      {	if ( s[i].ideal == 0 && s[i].shrink == 0 )
	  grow_this = 0;
	else
	  grow_this = (total_shrink ==0 ? grow / is_pos
				        : (grow * s[i].shrink) / total_shrink);
      }
      s[i].size = s[i].ideal + grow_this;
      growed += grow_this;
    }

    if ( grow != growed )
    { int do_grow = (grow > 0);
      int stretchables;
      int per_stretchable;
      int stretchall;
      int m;

      DEBUG(NAME_tile, Cprintf("Show grow %d, done %d\n", grow, growed));

      if ( grow < 0 )			/* normalise */
      { grow = -grow;
	growed = -growed;
      }

      for(i=0, stretchables=0; i < n; i++)
      { if ( (do_grow ? s[i].stretch : s[i].shrink) > 0)
	  stretchables++;
      }
					/* No one wants, so all have to */
      if ( stretchables == 0 )
      { stretchables = is_pos;
	stretchall = FALSE;
      } else
      { stretchall = TRUE;
      }
					/* distribute outside --> inside */
      per_stretchable = (grow - growed + stretchables - 1) / stretchables;

      for( i=0, m=n; growed < grow && m-- > 0; i++ )
      { int j = (i%2 ? i : n - i - 1);

	if ( stretchall ||
	     (do_grow ? s[j].stretch : s[i].shrink) > 0 )
	{ int to_grow = (grow - growed < per_stretchable ? grow - growed
							 : per_stretchable);
	  
					/* don't make negative */
	  if ( !do_grow	&& to_grow > s[j].size )
	    to_grow = s[j].size;

	  s[j].size += (do_grow ? to_grow : -to_grow);
	  growed += to_grow;
	}
      }
    }
  
    ok = TRUE;
    for(i = 0; i < n; i++)		/* Do not make sizes negative */
    { if ( s[i].size < 0 )
      { s[i].ideal = 0;
        s[i].shrink = 0;
	DEBUG(NAME_tile, Cprintf("%d is negative; setting to 0\n", i));
	ok = FALSE;
      }
    }
  }

  succeed;
}


status
setTile(TileObj t, Int x, Int y, Int w, Int h)
{ TileObj super;

  if ( notDefault(w) && valInt(w) < 0 ) w = ZERO;
  if ( notDefault(h) && valInt(h) < 0 ) h = ZERO;

  if ( notDefault(w) )
  { if ( t->enforced == ON )
    { assign(t, horStretch, ZERO);
      assign(t, horShrink,  ZERO);
    }
    assign(t, idealWidth, w);
  }
  if ( notDefault(h) )
  { if ( t->enforced == ON )
    { assign(t, verStretch, ZERO);
      assign(t, verShrink,  ZERO);
    }
    assign(t, idealHeight, h);
  }

  for(super = t->super; notNil(super); super = super->super)
    computeTile(super);

  if ( t->enforced == ON )
  { if ( notNil(t->super) )
      layoutTile(getRootTile(t), DEFAULT, DEFAULT, DEFAULT, DEFAULT);
    else
      layoutTile(t, x, y, w, h);
  }

  succeed;
}


status
enforceTile(TileObj t)
{ if ( t->enforced == OFF )
  { assign(t, enforced, ON);
    
    layoutTile(t, DEFAULT, DEFAULT, t->idealWidth, t->idealHeight);
  }

  succeed;
}


static status
layoutTile(TileObj t, Int ax, Int ay, Int aw, Int ah)
{ int border = valInt(t->border);
  int borders = (isNil(t->members) ? 0 : valInt(getSizeChain(t->members))-1);
  int x, y, w, h;

  assign(t, enforced, ON);

  if ( notDefault(aw) && valInt(aw) < 0 ) aw = ZERO;
  if ( notDefault(ah) && valInt(ah) < 0 ) ah = ZERO;
 
  setArea(t->area, ax, ay, aw, ah);
  x = valInt(t->area->x);
  y = valInt(t->area->y);
  w = valInt(t->area->w);
  h = valInt(t->area->h);

  if ( isNil(t->super) )
  { x += border;
    y += border;
    w -= border*2;
    h -= border*2;
  }

  if ( t->orientation == NAME_none )
    return send(t->object, NAME_doSet,
		toInt(x), toInt(y), toInt(w), toInt(h), 0);

  if ( t->orientation == NAME_horizontal )
  { stretch s[MAX_TILE_MEMBERS];
    int n = 0;
    Cell cell;
    
    for_cell(cell, t->members)
    { TileObj t2 = cell->value;
      
      s[n].ideal   = valInt(t2->idealWidth);
      s[n].stretch = valInt(t2->horStretch);
      s[n].shrink  = valInt(t2->horShrink);
      n++;
    }
    
    distribute_stretches(s, n, w - border*borders);
    
    n = 0;
    for_cell(cell, t->members)
    { TileObj t2 = cell->value;
      
      layoutTile(t2, toInt(x), toInt(y), toInt(s[n].size), toInt(h));
      x += s[n].size + border;
      n++;
    }
    
    succeed;
  } else /*if ( t->orientation == NAME_vertical )*/
  { stretch s[MAX_TILE_MEMBERS];
    int n = 0;
    Cell cell;
    
    for_cell(cell, t->members)
    { TileObj t2 = cell->value;
      
      s[n].ideal   = valInt(t2->idealHeight);
      s[n].stretch = valInt(t2->verStretch);
      s[n].shrink  = valInt(t2->verShrink);
      n++;
    }
    
    distribute_stretches(s, n, h - border*borders);
    
    n = 0;
    for_cell(cell, t->members)
    { TileObj t2 = cell->value;
      
      layoutTile(t2, toInt(x), toInt(y), toInt(w), toInt(s[n].size));
      y += s[n].size + border;
      n++;
    }
    
    succeed;
  }
}


static status
areaTile(TileObj t, Area a)
{ return setTile(t, a->x, a->y, a->w, a->h);
}


static status
positionTile(TileObj t, Point p)
{ return setTile(t, p->x, p->y, DEFAULT, DEFAULT);
}


static status
sizeTile(TileObj t, Size s)
{ return setTile(t, DEFAULT, DEFAULT, s->w, s->h);
}


static status
xTile(TileObj t, Int x)
{ return setTile(t, x, DEFAULT, DEFAULT, DEFAULT);
}


static status
yTile(TileObj t, Int y)
{ return setTile(t, DEFAULT, y, DEFAULT, DEFAULT);
}


static status
widthTile(TileObj t, Int w)
{ return setTile(t, DEFAULT, DEFAULT, w, DEFAULT);
}


static status
heightTile(TileObj t, Int h)
{ return setTile(t, DEFAULT, DEFAULT, DEFAULT, h);
}


static status
cornerTile(TileObj t, Point pos)
{ return setTile(t, DEFAULT, DEFAULT, sub(pos->x, t->area->x),
		      		      sub(pos->y, t->area->y));
}


static status
centerTile(TileObj t, Point pos)
{ return setTile(t, dif(pos->x, t->area->w),
		    dif(pos->y, t->area->h),
		    DEFAULT, DEFAULT);
}


		 /*******************************
		 *	  SET OPERATIONS	*
		 *******************************/

static status
forAllTile(TileObj t, Code msg)
{ if ( notNil(t->object) )
    TRY(forwardCodev(msg, 1, &t->object));

  if ( notNil(t->members) )
  { TileObj st;

    for_chain(t->members, st, TRY(forAllTile(st, msg)));
  }

  succeed;
}


		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */

static char *T_initialise[] =
        { "object=object*", "width=[int]", "height=[int]" };
static char *T_xADintD_yADintD_widthADintD_heightADintD[] =
        { "x=[int]", "y=[int]", "width=[int]", "height=[int]" };

/* Instance Variables */

static vardecl var_tile[] =
{ IV(NAME_idealWidth, "int", IV_BOTH,
     NAME_dimension, "Desired width of the tile"),
  IV(NAME_idealHeight, "int", IV_BOTH,
     NAME_dimension, "Desired height of the tile"),
  IV(NAME_horStretch, "int", IV_BOTH,
     NAME_resize, "Encouragement to get wider"),
  IV(NAME_horShrink, "int", IV_BOTH,
     NAME_resize, "Encouragement to get smaller"),
  IV(NAME_verStretch, "int", IV_BOTH,
     NAME_resize, "Encouragement to get higher"),
  IV(NAME_verShrink, "int", IV_BOTH,
     NAME_resize, "Encouragement to get lower"),
  IV(NAME_border, "int", IV_BOTH,
     NAME_appearance, "Distance between areas"),
  IV(NAME_orientation, "{none,horizontal,vertical}", IV_GET,
     NAME_layout, "Direction of adjacent sub-tiles"),
  IV(NAME_members, "chain*", IV_GET,
     NAME_organisation, "Managed tiles (subtiles)"),
  IV(NAME_super, "tile*", IV_GET,
     NAME_organisation, "Tile that manages me"),
  IV(NAME_object, "object*", IV_GET,
     NAME_client, "Object managed"),
  SV(NAME_area, "area", IV_GET|IV_STORE, areaTile,
     NAME_dimension, "Area of the object"),
  IV(NAME_enforced, "bool", IV_GET,
     NAME_layout, "If @on, the tile's layout will be enforced")
};

/* Send Methods */

static senddecl send_tile[] =
{ SM(NAME_initialise, 3, T_initialise, initialiseTile,
     DEFAULT, "Create from object, width and height"),
  SM(NAME_unlink, 0, NULL, unlinkTile,
     DEFAULT, "Unlink sub and super-tiles"),
  SM(NAME_center, 1, "point", centerTile,
     NAME_dimension, "Set center by moving tile"),
  SM(NAME_corner, 1, "point", cornerTile,
     NAME_dimension, "Set point opposite to origin"),
  SM(NAME_height, 1, "int", heightTile,
     NAME_dimension, "Set H of tile"),
  SM(NAME_position, 1, "point", positionTile,
     NAME_dimension, "Set XY of the tile"),
  SM(NAME_set, 4, T_xADintD_yADintD_widthADintD_heightADintD, setTile,
     NAME_dimension, "Set XYWH of entire tile"),
  SM(NAME_size, 1, "size", sizeTile,
     NAME_dimension, "Set WH of the tile"),
  SM(NAME_width, 1, "int", widthTile,
     NAME_dimension, "Set W of tile"),
  SM(NAME_x, 1, "int", xTile,
     NAME_dimension, "Set X of the tile"),
  SM(NAME_y, 1, "int", yTile,
     NAME_dimension, "Set Y of the tile"),
  SM(NAME_forAll, 1, "code", forAllTile,
     NAME_iterate, "Iterate over all <-object's"),
  SM(NAME_above, 1, "object", aboveTile,
     NAME_layout, "Place a tile above me"),
  SM(NAME_below, 1, "object", belowTile,
     NAME_layout, "Place a tile below me"),
  SM(NAME_enforce, 0, NULL, enforceTile,
     NAME_layout, "Enforce the tile layout"),
  SM(NAME_layout, 4, T_xADintD_yADintD_widthADintD_heightADintD, layoutTile,
     NAME_layout, "Compute subtile layout and adjust objects"),
  SM(NAME_left, 1, "object", leftTile,
     NAME_layout, "Place a tile to my left"),
  SM(NAME_right, 1, "object", rightTile,
     NAME_layout, "Place a tile to my right"),
  SM(NAME_compute, 0, NULL, computeTile,
     NAME_update, "Compute ideal sizes from sub-tiles")
};

/* Get Methods */

static getdecl get_tile[] =
{ GM(NAME_root, 0, "tile", NULL, getRootTile,
     NAME_organisation, "Root of the tile-hierarchy")
};

/* Resources */

static resourcedecl rc_tile[] =
{ RC(NAME_border, "int", "3",
     "Border between subtiles")
};

/* Class Declaration */

static Name tile_termnames[] =
	{ NAME_object, NAME_idealWidth, NAME_idealHeight };

ClassDecl(tile_decls,
          var_tile, send_tile, get_tile, rc_tile,
          1, tile_termnames,
          "$Rev$");


status
makeClassTile(Class class)
{ return declareClass(class, &tile_decls);
}

