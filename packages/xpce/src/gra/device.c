/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1992 University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>
#include <h/graphics.h>

static status	updateConnectionsDevice(Device dev, Int level);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Class device is an abstract superclass used to define method common
to  Pictures and  Figures:  manipulating  a chain   of  graphicals and
dispatching events.

A device is a subclass of graphical and thus can be displayed on other
devices.

Devices  maintain the  graphical's  attribute  <->area  to reflect the
bounding box  of   all displayed  graphicals  (e.g.   graphicals  with
<->displayed equals @on).  To the X-Y  coordinate of this bounding box
the  <->offset is  added  to  ensure  smooth intergration  with  class
figure.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

		/********************************
		*        CREATE/DESTROY		*
		********************************/

status
initialiseDevice(Device dev)
{ initialiseGraphical(dev, ZERO, ZERO, ZERO, ZERO);

  assign(dev, level, ZERO);
  assign(dev, offset, newObject(ClassPoint, 0));
  assign(dev, graphicals, newObject(ClassChain, 0));
  assign(dev, badBoundingBox, OFF);
  assign(dev, badFormat, OFF);
  assign(dev, format, NIL);
  assign(dev, pointed, newObject(ClassChain, 0));
  assign(dev, clip_area, NIL);
  assign(dev, recompute, newObject(ClassChain, 0));

  succeed;
}


status
unlinkDevice(Device dev)
{ if ( notNil(dev->graphicals) )
  { Graphical gr;

    for_chain(dev->graphicals, gr, DeviceGraphical(gr, NIL));
  }
  
  return unlinkGraphical((Graphical) dev);
}

		/********************************
		*             CURSOR		*
		********************************/

CursorObj
getDisplayedCursorDevice(Device dev)
{ CursorObj c2;
  Cell cell;

  for_cell(cell, dev->pointed)
  { if ( notNil(c2 = qadGetv(cell->value, NAME_displayedCursor, 0, NULL)) )
      answer(c2);
  }

  answer(dev->cursor);			/* = getDisplayedCursorGraphical()! */
}


		/********************************
		*         EVENT HANDLING	*
		********************************/

static Chain
get_pointed_objects_device(Device dev, Int x, Int y, Chain ch)
{ Cell cell;

  if ( isDefault(ch) )
    ch = answerObject(ClassChain, 0);
  else
    clearChain(ch);

  for_cell(cell, dev->graphicals)
  { register Graphical gr = cell->value;

    if ( gr->displayed == ON &&
	 inEventAreaGraphical(gr, x, y) )
      prependChain(ch, gr);
  }

  if ( notDefault(ch) )
    answer(ch);

  fail;
}


Chain
getPointedObjectsDevice(Device dev, Any pos, Chain ch)
{ Int x, y;

  if ( instanceOfObject(pos, ClassPoint) )
  { Point pt = pos;

    x = pt->x;
    y = pt->y;
  } else /*if ( instanceOfObject(pos, ClassEvent) )*/
    get_xy_event(pos, dev, OFF, &x, &y);

  return get_pointed_objects_device(dev, x, y, ch);
}

#define MAX_ACTIVE 250			/* Objects under the mouse */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
updatePointedDevice() updates the <->pointed chain of the device.
The <->pointed chain is a  chain  holding all events that overlap with
the mouse  position and  are  editable  and  displayed.   It sends  an
area_enter event to all graphicals  that have  been added to the chain
and an area_exit event to all graphicals that have been deleted to the
chain.  Care is taken to prevent this function from creating  too many
intermediate objects as is it called very frequently.

The event area_cancel is ok, but area_resume should verify the buttons
are in the same  state as when  the area is  left and at  least one is
down.   This requires us  to store the status  of the button with  the
graphical object, costing us an additional 4 bytes on  each graphical.
To do or not to do?
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static status
updatePointedDevice(Device dev, EventObj ev)
{ Cell cell, c2;
  Graphical active[MAX_ACTIVE];
  int n, an = 0;
  Int x, y;
  Name enter, exit;

  if ( allButtonsUpEvent(ev) )
  { enter = NAME_areaEnter;
    exit  = NAME_areaExit;
  } else
  { enter = NAME_areaResume;
    exit  = NAME_areaCancel;
  }

					/* Exit event: leave all children */
  if ( isAEvent(ev, NAME_areaExit) )
  { for_cell(cell, dev->pointed)
      generateEventGraphical(cell->value, exit);

    clearChain(dev->pointed);
    succeed;
  }

  get_xy_event(ev, dev, OFF, &x, &y);

					/* See which graphicals are left */
  for_cell_save(cell, c2, dev->pointed)
  { register Graphical gr = cell->value;

    if ( gr->displayed == OFF || !inEventAreaGraphical(gr, x, y) )
    { DEBUG(NAME_event, Cprintf("Leaving %s\n", pp(gr)));
      deleteChain(dev->pointed, gr);
      generateEventGraphical(gr, exit);
    }
  }
  
					/* See which graphicals are entered */
  for_cell(cell, dev->graphicals)
  { register Graphical gr = cell->value;

    if ( gr->displayed == ON && inEventAreaGraphical(gr, x, y) )
    { active[an++] = gr;

      if ( memberChain(dev->pointed, gr) != SUCCEED )
      { DEBUG(NAME_event, Cprintf("Entering %s\n", pp(gr)));
        generateEventGraphical(gr, enter);
      }

      if ( an == MAX_ACTIVE )		/* Shift to keep top ones */
      { int n;
        for( n = 0; n < MAX_ACTIVE-1; n++ )
	  active[n] = active[n+1];
	an--;
      }
    }
  }
    
					/* Update the ->pointed chain */
  for( cell = dev->pointed->head, n = an-1; n >= 0; n--, cell = cell->next )
  { if ( isNil(cell) )			/* Chain is out; extend it */
    { for( ; n >=0; n-- )
        appendChain(dev->pointed, active[n]);
      break;
    }

    cellValueChain(dev->pointed, PointerToInt(cell), active[n]);
  }
  
  while( notNil(cell) )			/* Remove the tail of the chain */
  { c2 = cell->next;
    deleteChain(dev->pointed, cell->value);
    cell = c2;
  }

  succeed;
}


status
inspectDevice(Device dev, EventObj ev)
{ Cell cell;

  for_cell(cell, dev->pointed)
  { if ( instanceOfObject(cell->value, ClassDevice) )
    { if ( inspectDevice(cell->value, ev) )
    	succeed;
    } else
    { if ( inspectDisplay(CurrentDisplay(dev), cell->value, ev) )
	succeed;
    }
  }

  return inspectDisplay(CurrentDisplay(dev), (Graphical) dev, ev);
}


static Graphical
get_find_device(Device dev, Int x, Int y, Code cond)
{ LocalArray(Graphical, grv, valInt(dev->graphicals->size));
  int i, grn;
  Cell cell;

  grn=0;
  for_cell(cell, dev->graphicals)
    grv[grn++] = cell->value;

  for(i=grn-1; i >= 0; i--)
  { Graphical gr = grv[i];

    if ( notDefault(x) && !inEventAreaGraphical(gr, x, y) )
      continue;

    if ( instanceOfObject(gr, ClassDevice) )
    { Device dev2 = (Device) gr;
      Any rval;

      if ( (rval=get_find_device(dev2,
				 isDefault(x) ? x : sub(x, dev2->offset->x),
				 isDefault(y) ? y : sub(y, dev2->offset->y),
				 cond)) )
	answer(rval);
    } else
    { if ( isDefault(cond) ||
	   forwardCodev(cond, 1, (Any *)&gr) )
	answer(gr);
    }
  }

  if ( isDefault(cond) ||
       forwardCodev(cond, 1, (Any *)&dev) )
    answer((Graphical) dev);

  fail;
}


static Graphical
getFindDevice(Device dev, Any location, Code cond)
{ Int x, y;

  if ( instanceOfObject(location, ClassEvent) )
    get_xy_event(location, dev, OFF, &x, &y);
  else if ( isDefault(location) )
  { x = y = (Int) DEFAULT;
  } else
  { Point p = location;

    x = p->x;
    y = p->y;
  }

  return get_find_device(dev, x, y, cond);
}


status
eventDevice(Any obj, EventObj ev)
{ Device dev = obj;

  if ( dev->active != OFF )
  { Cell cell;

    updatePointedDevice(dev, ev);
  
    for_cell(cell, dev->pointed)
    { if ( postEvent(ev, cell->value, DEFAULT) )
	succeed;
    }

    return eventGraphical(dev, ev);
  }

  fail;
}


static status
typedDevice(Device dev, EventId id, Bool delegate)
{ Any key = characterName(id);
  Graphical gr;

  for_chain(dev->graphicals, gr,
	    if ( sendv(gr, NAME_key, 1, &key) )
	      succeed);

  if ( delegate == ON && notNil(dev->device) )
    return send(dev->device, NAME_typed, id, delegate, 0);

  fail;
}


status
advanceDevice(Device dev, Graphical gr)
{ Cell cell;
  int skip = TRUE;
  Graphical first = NIL;
  PceWindow sw;

  if ( isDefault(gr) )
    gr = NIL;

  TRY( sw = getWindowGraphical((Graphical) dev) );

  for_cell(cell, dev->graphicals)
  { if ( skip )
    { if ( isNil(first) &&
	   send(cell->value, NAME_WantsKeyboardFocus, 0) )
	first = cell->value;
      if ( cell->value == gr )
        skip = FALSE;

      continue;
    }
      
    if ( send(cell->value, NAME_WantsKeyboardFocus, 0) )
    { keyboardFocusWindow(sw, cell->value);
      succeed;
    }
  }
  
  if ( ((Device) sw != dev) && !(isNil(gr) && notNil(first)) )
  { send(dev->device, NAME_advance, dev, 0);
  } else
  { if ( notNil(first) )
      keyboardFocusWindow(sw, first);
    else
      keyboardFocusWindow(sw, NIL);
  }

  succeed;
}


		/********************************
		*       REPAINT MANAGEMENT	*
		********************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
The device's repaint manager is responsible for  keeping  track of the
devices area (the bounding box of its displayed graphicals) and of the
area that  needs repainting if  a ->RedrawArea is received.  It should
issue to appropriate ->RedrawArea request on its associated graphicals
if it receives an ->RedrawArea from its parent.

A number of changes are recognised:

   *) A graphical is added to the device or its displayed attribute has
      changed to @on.
   *) A graphical is erased from the device or its displayed attribute
      has changed to @off.
   *) The image of a graphical has changed.
   *) The area of a graphical has changed.

Graphicals indicate changes through the following call:

      CHANGING_GRAPHICAL(gr,
	  <code>
	  [changedImageGraphical(gr, x, y, w, h)]
	  [changedEntireImageGraphical(gr)]
      )
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

status
requestComputeDevice(Device dev, Any val)
{ DEBUG(NAME_compute, Cprintf("requestComputeDevice(%s)\n", pp(dev)));
  assign(dev, badBoundingBox, ON);
  assign(dev, badFormat, ON);

  return requestComputeGraphical(dev, val);
}


status
computeGraphicalsDevice(Device dev)
{ Chain ch = dev->recompute;

  while( !emptyChain(ch) )		/* tricky! */
  { Cell cell;
    int i, size = valInt(ch->size);
    ArgVector(array, size);

    for(i=0, cell = ch->head; notNil(cell); cell = cell->next)
      array[i++] = cell->value;

    clearChain(ch);
    for(i=0; i<size; i++)
    { Graphical gr = array[i];

      if ( !isFreedObj(gr) && notNil(gr->request_compute) )
      { qadSendv(gr, NAME_compute, 0, NULL);
	assign(gr, request_compute, NIL);
      }
    }
  }

  succeed;
}
   

status
computeDevice(Any obj)
{ Device dev = obj;

  if ( notNil(dev->request_compute) )
  { computeGraphicalsDevice(dev);
    computeFormatDevice(dev);
    computeBoundingBoxDevice(dev);

    assign(dev, request_compute, NIL);
  }

  succeed;
}


status
computeBoundingBoxDevice(Device dev)
{ if ( dev->badBoundingBox == ON )
  { Cell cell;
    Area a = dev->area;
    Int od[4];				/* ax, ay, aw, ah */
    od[0] = a->x; od[1] = a->y; od[2] = a->w; od[3] = a->h;

    clearArea(a);

    DEBUG(NAME_compute,
	  Cprintf("computeBoundingBoxDevice(%s) %ld %ld %ld %ld\n",
		  pp(dev),
		  valInt(od[0]), valInt(od[1]), valInt(od[3]), valInt(od[4])));


    for_cell(cell, dev->graphicals)
    { Graphical gr = cell->value;

      if ( gr->displayed == ON )
	unionNormalisedArea(a, gr->area);
    }

    relativeMoveArea(a, dev->offset);

    if ( od[0] != a->x || od[1] != a->y || od[3] != a->w || od[4] != a->h )
    { DEBUG(NAME_compute,
	    Cprintf("                          --> %ld %ld %ld %ld\n",
		    valInt(a->x), valInt(a->y), valInt(a->w), valInt(a->h)));

      if ( notNil(dev->device) )
      { requestComputeDevice(dev->device, DEFAULT);
      	updateConnectionsGraphical((Graphical) dev, sub(dev->level, ONE));
      }

      qadSendv(dev, NAME_changedUnion, 4, od);
    }

    if ( notNil(dev->clip_area) )
    { relativeMoveBackArea(a, dev->offset);
      if ( intersectionArea(dev->area, dev->clip_area) == FAIL )
      { assign(dev->area, w, ZERO);
        assign(dev->area, h, ZERO);
      }
      relativeMoveArea(a, dev->offset);
    }

    assign(dev, badBoundingBox, OFF);
  }

  succeed;
}


status
changedUnionDevice(Device dev, Int ox, Int oy, Int ow, Int oh)
{ succeed;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Our  parent requests  us to  repaint an area.    This  area is in  the
coordinate system of  the device we  are  displayed on.  The requested
repaint area may be larger than the area of myself.

This algorithm can be made more clever on  a number of  points.  First
of all we could be  more  clever  with none-square graphicals, notably
lines.  Next,  we could determine that  objects  are obscured by other
objects and thus  do not need to be  repainted.  We  will  leave these
optimisations for later.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

status
RedrawAreaDevice(Device dev, Area a)
{ Cell cell;
  Int ax = a->x, ay = a->y, aw = a->w, ah = a->h;
  Point offset = dev->offset;
  int ox = valInt(offset->x);
  int oy = valInt(offset->y);

  if ( aw == ZERO || ah == ZERO )
    succeed;

  DEBUG(NAME_redraw, Cprintf("RedrawAreaDevice(%s, %ld %ld %ld %ld)\n",
			     pp(dev), 
			     valInt(a->x), valInt(a->y),
			     valInt(a->w), valInt(a->h)));

  assign(a, x, toInt(valInt(a->x) - ox));
  assign(a, y, toInt(valInt(a->y) - oy));
  r_offset(ox, oy);

  if ( notNil(dev->clip_area) )
  { if ( !intersectionArea(a, dev->clip_area) )
      succeed;

    clipGraphical((Graphical)dev, a);
  }

  for_cell(cell, dev->graphicals)
  { Graphical gr = cell->value;

    if ( gr->displayed == ON && overlapArea(a, gr->area) )
      RedrawArea(gr, a);
  }

  if ( notNil(dev->clip_area) )
    unclipGraphical((Graphical)dev);

  r_offset(-ox, -oy);
  assign(a, x, ax);
  assign(a, y, ay);
  assign(a, w, aw);			/* why? should not change! */
  assign(a, h, ah);

  return RedrawAreaGraphical(dev, a);
}



		/********************************
		*         DISPLAY/ERASE		*
		********************************/


static status
clearDevice(Device dev)
{ Chain ch = dev->graphicals;

  while( !emptyChain(ch) )
    eraseDevice(dev, getHeadChain(ch));

  succeed;
}


status
displayDevice(Any Dev, Any Gr, Point pos)
{ Device dev = Dev;
  Graphical gr = Gr;
  
  if ( notDefault(pos) )
  { Variable var;

    if ( (var = getInstanceVariableClass(classOfObject(gr), NAME_autoAlign)) )
    { Any off = OFF;

      sendVariable(var, gr, 1, &off);
    }

    setGraphical(gr, pos->x, pos->y, DEFAULT, DEFAULT);
  }

  DeviceGraphical(gr, dev);
  DisplayedGraphical(gr, ON);

  succeed;
}


status
appendDevice(Device dev, Graphical gr)
{ appendChain(dev->graphicals, gr);
  assign(gr, device, dev);
  if ( notNil(gr->request_compute) )
    appendChain(dev->recompute, gr);

  if ( gr->displayed == ON )
  { assign(gr, displayed, OFF);
    displayedGraphicalDevice(dev, gr, ON);
  }

  qadSendv(gr, NAME_reparent, 0, NULL);

  succeed;
}


status
eraseDevice(Device dev, Graphical gr)
{ if ( gr->device == dev )
  { PceWindow sw = getWindowGraphical((Graphical) dev);

    if ( sw != FAIL )
    { if ( sw->keyboard_focus == gr )
	keyboardFocusWindow(sw, NIL);
      if ( sw->focus == gr )
	focusWindow(sw, NIL, NIL, NIL, NIL);
    }

    if ( gr->displayed == ON )
      displayedGraphicalDevice(dev, gr, OFF);

    deleteChain(dev->recompute, gr);
    deleteChain(dev->pointed, gr);
    assign(gr, device, NIL);
    GcProtect(dev, deleteChain(dev->graphicals, gr));
    if ( !isFreedObj(gr) )
      qadSendv(gr, NAME_reparent, 0, NULL);
  }

  succeed;
}


status
displayedGraphicalDevice(Device dev, Graphical gr, Bool val)
{ if ( gr->displayed != val )
  { int solid = onFlag(gr, F_SOLID);

    if ( solid )
      clearFlag(gr, F_SOLID);

    if ( val == ON )
    { assign(gr, displayed, val);
      changedEntireImageGraphical(gr);
    } else
    { changedEntireImageGraphical(gr);
      assign(gr, displayed, val);
    }

    if ( solid )
      setFlag(gr, F_SOLID);

    if ( instanceOfObject(gr, ClassDevice) )
      updateConnectionsDevice((Device) gr, dev->level);
    else
      updateConnectionsGraphical(gr, dev->level);

    requestComputeDevice(dev, DEFAULT);		/* TBD: ON: just union */
  }

  succeed;
}

		/********************************
		*           EXPOSURE		*
		********************************/

status
exposeDevice(Device dev, Graphical gr, Graphical gr2)
{ if ( gr->device != dev || (notDefault(gr2) && gr2->device != dev) )
    fail;

  if ( isDefault(gr2) )
  { addCodeReference(gr);
    deleteChain(dev->graphicals, gr);
    appendChain(dev->graphicals, gr);
    delCodeReference(gr);
  } else
  { moveAfterChain(dev->graphicals, gr, gr2);
    changedEntireImageGraphical(gr2);
  }
  requestComputeDevice(dev, DEFAULT);	/* Actually only needs format */

  changedEntireImageGraphical(gr);

  succeed;
}


status
hideDevice(Device dev, Graphical gr, Graphical gr2)
{ if ( gr->device != dev || (notDefault(gr2) && gr2->device != dev) )
    fail;

  if ( isDefault(gr2) )
  { addCodeReference(gr);
    deleteChain(dev->graphicals, gr);
    prependChain(dev->graphicals, gr);
    delCodeReference(gr);
  } else
  { moveBeforeChain(dev->graphicals, gr, gr2);
    changedEntireImageGraphical(gr2);
  }
  requestComputeDevice(dev, DEFAULT);	/* Actually only needs format */

  changedEntireImageGraphical(gr);

  succeed;
}


status
swapGraphicalsDevice(Device dev, Graphical gr, Graphical gr2)
{ if ( gr->device != dev || (notDefault(gr2) && gr2->device != dev) )
    fail;

  swapChain(dev->graphicals, gr, gr2);

  changedEntireImageGraphical(gr);
  changedEntireImageGraphical(gr2);
  requestComputeDevice(dev, DEFAULT);		/* Actually only needs format */

  succeed;
}


		/********************************
		*          SELECTION		*
		********************************/


static status
selectionDevice(Device dev, Any obj)
{ Cell cell;

  if ( instanceOfObject(obj, ClassChain) )
  { for_cell(cell, dev->graphicals)
    { if ( memberChain(obj, cell->value) )
        send(cell->value, NAME_selected, ON, 0);
      else
	send(cell->value, NAME_selected, OFF, 0);
    }

    succeed;
  }

  for_cell(cell, dev->graphicals)
    send(cell->value, NAME_selected, cell->value == obj ? ON : OFF, 0);

  succeed;
}


static Chain
getSelectionDevice(Device dev)
{ Chain ch = answerObject(ClassChain, 0);
  Cell cell;

  for_cell(cell, dev->graphicals)
  { if ( ((Graphical)cell->value)->selected == ON )
      appendChain(ch, cell->value);
  }

  answer(ch);
}


		/********************************
		*           FORMATTING          *
		*********************************/

static status
formatDevice(Device dev, Any obj, Any arg)
{ status rval = SUCCEED;

  if ( isNil(obj) || instanceOfObject(obj, ClassFormat) )
  { assign(dev, format, obj);
  } else 
  { if ( isNil(dev->format) )
      assign(dev, format, newObject(ClassFormat, 0));

    rval = send(dev->format, (Name)obj, arg, 0);
  }
  requestComputeDevice(dev, DEFAULT);

  return rval;
}

static void
move_graphical(Graphical gr, int x, int y)
{ Int X = toInt(x);
  Int Y = toInt(y);

  if ( X != gr->area->x || Y != gr->area->y )
    setGraphical(gr, X, Y, DEFAULT, DEFAULT);
}

#define MAXCOLS 1024
#define MAXROWS 1024

status
computeFormatDevice(Device dev)
{ Format l;

  if ( dev->badFormat == OFF || isNil(l=dev->format) )
    succeed;

#define HV(h, v) (l->direction == NAME_horizontal ? (h) : (v))
#define MUSTBEVISIBLE(dev, gr) { if (gr->displayed == OFF) continue; }

  if ( l->columns == ON )
  { int cw[MAXCOLS];			/* column widths */
    int rh[MAXROWS];			/* row heights */
    char cf[MAXCOLS];			/* column format */
    int cs = valInt(l->column_sep);	/* column separator size */
    int rs = valInt(l->row_sep);	/* row separator size */
    Cell cell;
    int c, r = 0;
    int cols = valInt(l->width);
    int x = 0;
    int y = 0;

    for(c=0; c < cols; c++)
    { cw[c] = 0;
      cf[c] = 'l';
    }
    
    if ( notNil(l->adjustment) )
    { for(c=0; c < cols; c++)
      { Name format = (Name) getElementVector(l->adjustment, toInt(c+1));

	if ( equalName(format, NAME_center) )
	  cf[c] = 'c';
	else if ( equalName(format, NAME_right) )
	  cf[c] = 'r';
	else
	  cf[c] = 'l';
      }
    }

    rh[r] = c = 0;
    for_cell(cell, dev->graphicals)
    { Graphical gr = cell->value;
      int gw, gh;

      MUSTBEVISIBLE(dev, gr);
      gw = valInt(HV(gr->area->w, gr->area->h));
      gh = valInt(HV(gr->area->h, gr->area->w));

      cw[c] = max(cw[c], gw);
      rh[r] = max(rh[r], gh);

      if ( ++c >= cols )
      { c = 0;
        rh[++r] = 0;
      }
    }

    c = r = 0;

    for_cell(cell, dev->graphicals)
    { Graphical gr = cell->value;
      MUSTBEVISIBLE(dev, gr);

      if ( l->direction == NAME_horizontal )
      { switch( cf[c] )
        { case 'l':	move_graphical(gr, x, y);
			break;
          case 'r':	move_graphical(gr, x+cw[c]-valInt(gr->area->w), y);
			break;
	  case 'c':	move_graphical(gr, x+(cw[c]-valInt(gr->area->w))/2, y);
	  		break;
	}
      } else
      { switch( cf[c] )
        { case 'l':	move_graphical(gr, y, x);
			break;
          case 'r':	move_graphical(gr, y, x+cw[c]-valInt(gr->area->h));
			break;
	  case 'c':	move_graphical(gr, y, x+(cw[c]-valInt(gr->area->h))/2);
	  		break;
	}
      }

      if ( c+1 >= cols )
      { y += rh[r++] + rs;
        c = 0;
	x = 0;
      } else
      { x += cw[c++] + cs;
      }
    }
  } else				/* non-column device */
  { int x = 0;
    int y = 0;
    int w = valInt(l->width);
    int cs = valInt(l->column_sep);
    int rs = valInt(l->row_sep);
    int rh = 0;
    int first = TRUE;
    Cell cell;

    for_cell(cell, dev->graphicals)
    { Graphical gr = cell->value;
      int gw, gh;

      MUSTBEVISIBLE(dev, gr);
      gw = valInt(HV(gr->area->w, gr->area->h));
      gh = valInt(HV(gr->area->h, gr->area->w));

      if ( !first && x + gw > w )	/* start next column */
      { y += rh + rs;
        rh = 0;
        x = 0;
        first = TRUE;
      }
      move_graphical(gr, HV(x, y), HV(y, x));
      x += gw + cs;
      rh = max(rh, gh);
      first = FALSE;
    }
  }
#undef HV

  assign(dev, badFormat, OFF);

  succeed;
}


		/********************************
		*            LAYOUT		*
		********************************/

static HashTable PlacedTable = NULL;	/* placed objects */

#define MAX_L_ROWS	100
#define MAXCOLLUMNS	100

typedef struct _unit			/* can't use cell! */
{ Graphical item;			/* Item displayed here */
  short height;				/* Height above reference */
  short	depth;				/* Depth below reference */
  short right;				/* Right of reference point */
  short left;				/* Left of reference point */
  short	hstretch;			/* Strechable horizontal */
  short vstretch;			/* Strechable vertical */
  Name  alignment;			/* alignment of the item */
} unit, *Unit;

static unit empty_unit = {(Graphical) NIL, 0, 0, 0, 0, 0, 0, NAME_column};

typedef struct _matrix
{ unit *units[MAXCOLLUMNS];
} matrix, *Matrix;


#define IsPlaced(gr)  (getMemberHashTable(PlacedTable, gr) == ON)
#define SetPlaced(gr) (appendHashTable(PlacedTable, gr, ON))

static void
shift_x_matrix(Matrix m, int *max_x, int *max_y)
{ int x, y;

  m->units[*max_x] = alloc(sizeof(unit) * MAX_L_ROWS);
  for(y=0; y < *max_y; y++)
  { for(x = *max_x; x > 0; x--)
      m->units[x][y] = m->units[x-1][y];

    m->units[0][y] = empty_unit;
  }

  (*max_x)++;
}


static void
shift_y_matrix(Matrix m, int *max_x, int *max_y)
{ int x, y;

  for(x=0; x < *max_x; x++)
  { for(y = *max_y; y > 0; y--)
      m->units[x][y] = m->units[x][y-1];

    m->units[x][0] = empty_unit;
  }

  (*max_y)++;
}


static void
expand_x_matrix(Matrix m, int *max_x, int *max_y)
{ int y;

  m->units[*max_x] = alloc(sizeof(unit) * MAX_L_ROWS);
  for(y=0; y < *max_y; y++)
    m->units[*max_x][y] = empty_unit;

  (*max_x)++;
}


static void
expand_y_matrix(Matrix m, int *max_x, int *max_y)
{ int x;

  for(x=0; x < *max_x; x++)
    m->units[x][*max_y] = empty_unit;

  (*max_y)++;
}


static void
free_matrix_columns(Matrix m, int max_x)
{ int x;

  for(x=0; x<max_x; x++)
    unalloc(sizeof(unit) * MAX_L_ROWS, m->units[x]);
}


static status
placeDialogItem(Device d, Matrix m, Graphical i,
		int x, int y, int *max_x, int *max_y)
{ Graphical gr;

  if ( IsPlaced(i) || get(i, NAME_autoAlign, 0) != ON )
    succeed;
  SetPlaced(i);

  DEBUG(NAME_layout, Cprintf("placing %s\n", pp(i)));

  while( x < 0 ) { shift_x_matrix(m, max_x, max_y); x++; }
  while( y < 0 ) { shift_y_matrix(m, max_x, max_y); y++; }
  while( x >= *max_x ) expand_x_matrix(m, max_x, max_y); 
  while( y >= *max_y ) expand_y_matrix(m, max_x, max_y); 

  if ( isNil(i->device) )
    displayDevice(d, i, DEFAULT);

  m->units[x][y].item = i;
  m->units[x][y].alignment = get(i, NAME_alignment, 0);
  if ( !m->units[x][y].alignment )
    m->units[x][y].alignment = NAME_left;

  if ( instanceOfObject((gr = get(i, NAME_above, 0)), ClassGraphical) )
    placeDialogItem(d, m, gr, x, y-1, max_x, max_y);
  if ( instanceOfObject((gr = get(i, NAME_below, 0)), ClassGraphical) )
    placeDialogItem(d, m, gr, x, y+1, max_x, max_y);
  if ( instanceOfObject((gr = get(i, NAME_right, 0)), ClassGraphical) )
    placeDialogItem(d, m, gr, x+1, y, max_x, max_y);
  if ( instanceOfObject((gr = get(i, NAME_left, 0)), ClassGraphical)  )
    placeDialogItem(d, m, gr,  x-1, y, max_x, max_y);

  succeed;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Distribute `total' pixels over n buckets.  Remaining pixels are distributed
outward-in.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void
distribute(int total, int n, int *buckets)
{ int l, i, m = total/n;

  for(i=0; i<n; i++)
    buckets[i] = m;
  total -= m * n;

  for(i=0, l=TRUE; total > 0; l = !l, total--, i++ )
    buckets[l ? i : n-i]++;
}


status
layoutDialogDevice(Device d, Size gap, Size bb)
{ matrix m;
  int x, y, max_x = 0, max_y = 0;
  int px, py;
  Graphical gr;
  Cell cell;

  if ( isDefault(gap) )			/* TBD: proper integration */
    gap = answerObject(ClassSize, toInt(15), toInt(8), 0);

  if ( notNil(d->request_compute) )
  { Cell cell;

    for_cell(cell, d->graphicals)
      send(cell->value, NAME_layoutDialog, 0, NULL);
  }

  if ( !PlacedTable )
    PlacedTable = createHashTable(toInt(32), OFF);
  else
    clearHashTable(PlacedTable);	

  for(;;)
  { int found = 0;

    for_cell(cell, d->graphicals)
    { if ( !IsPlaced(cell->value) &&
	   get(cell->value, NAME_autoAlign, 0) == ON )
      { placeDialogItem(d, &m, cell->value, 0, 0, &max_x, &max_y);
	found++;
  	break;
      }
    }

    if ( !found )
      succeed;				/* finished */

    for(x=0; x<max_x; x++)		/* Align labels and values */
    { int lw = -1;
      int vw = -1;
      int align_flags[MAXCOLLUMNS];

#define AUTO_ALIGN_LABEL 1
#define AUTO_ALIGN_VALUE 2

      for(y=0; y<max_y; y++)
      { align_flags[y] = 0;

	if ( notNil(gr = m.units[x][y].item) &&
	     gr->displayed == ON &&
	     m.units[x][y].alignment == NAME_column )
	{ int w;

	  if ( get(gr, NAME_autoLabelAlign, 0) == ON )
	  { if ( (w = valInt(get(gr, NAME_labelWidth, 0))) > lw )
	      lw = w;
	    align_flags[y] |= AUTO_ALIGN_LABEL;
	  }

	  if ( get(gr, NAME_autoValueAlign, 0) == ON )
	  { if ( (w = valInt(get(gr, NAME_valueWidth, 0))) > vw )
	      vw = w;
	    align_flags[y] |= AUTO_ALIGN_VALUE;
	  }
	}
      }
      if ( lw >= 0 )
      { for(y=0; y<max_y; y++)
	  if ( (align_flags[y] & AUTO_ALIGN_LABEL) &&
	       m.units[x][y].alignment == NAME_column )
	    send(m.units[x][y].item, NAME_labelWidth, toInt(lw), 0);
      }
      if ( vw >= 0 )
      { for(y=0; y<max_y; y++)
	  if ( (align_flags[y] & AUTO_ALIGN_VALUE) &&
	       m.units[x][y].alignment == NAME_column )
	    send(m.units[x][y].item, NAME_valueWidth, toInt(vw), 0);
      }
    }

    ComputeGraphical(d);		/* recompute for possible changes */

    for(x=0; x<max_x; x++)		/* Get sizes */
    { for(y=0; y<max_y; y++)
      { Unit u = &m.units[x][y];

	if ( notNil(gr = u->item) )
	{ if ( gr->displayed == ON )
	  { Point reference = get(gr, NAME_reference, 0);
	    int rx = (reference ? valInt(reference->x) : 0);
	    int ry = (reference ? valInt(reference->y) : 0);
	    Int hs = get(gr, NAME_horStretch, 0);
	    Int vs = get(gr, NAME_verStretch, 0);
	  
	    if ( !hs ) hs = ZERO;
	    if ( !vs ) vs = ZERO;

	    u->left     = rx;
	    u->height   = ry;
	    u->depth    = valInt(gr->area->h) - ry;
	    u->right    = valInt(gr->area->w) - rx;
	    u->hstretch = valInt(hs);
	    u->vstretch = valInt(vs);
	  } else
	  { u->left     = 0;
	    u->height   = 0;
	    u->depth    = 0;
	    u->right    = 0;
	  }
	}
      }
    }


    for(x=0; x<max_x; x++)		/* Determine unit width */
    { int r = 0, l = 0;
  
      for(y=0; y<max_y; y++)
      { if ( m.units[x][y].alignment == NAME_column )
	{ if ( m.units[x][y].right > r ) r = m.units[x][y].right;
	  if ( m.units[x][y].left  > l ) l = m.units[x][y].left;
	}
      }

      for(y=0; y<max_y; y++)
      { if ( m.units[x][y].alignment == NAME_column )
	{ m.units[x][y].right = r;
	  m.units[x][y].left = l;
	}
      }
    }


  { int gaph = valInt(gap->h);
    int ideal_y = gaph * 2;
    int ngaps = -1;

    for(y=0; y<max_y; y++)		/* Determine unit height */
    { int h = -1000, d = -1000;

      for(x=0; x<max_x; x++)
      { if ( m.units[x][y].height > h ) h = m.units[x][y].height;
	if ( m.units[x][y].depth  > d ) d = m.units[x][y].depth;
      }
      for(x=0; x<max_x; x++)
      { m.units[x][y].height = h;
	m.units[x][y].depth = d;
      }

      ideal_y += (h+d);
      ngaps++;
    }
    ideal_y += ngaps * gaph;

    if ( notDefault(bb) && ngaps > 0 )	/* distribute in Y-direction */
    { int extra_y = valInt(bb->h) - ideal_y;

      if ( extra_y > 0 )
      { int *distributed = alloca(sizeof(int) * ngaps);
	int ngap = 0;

	distribute(extra_y, ngaps, distributed);

	for(y=0; y<max_y; y++)
	{ for(x=0; x<max_x; x++)
	  { m.units[x][y].depth += distributed[ngap];
	  }
	      
	  ngap++;
	}
      }
    }

					  /* Place the items */
    for(py = gaph, y=0; y<max_y; y++)
    { int px = valInt(gap->w);
      int lx = px;			/* x for left aligned items */
      int gapw = valInt(gap->w);
      
      for(x = 0; x < max_x; x++)
      { if ( notNil(gr = m.units[x][y].item) &&
	     gr->displayed == ON )
	{ Point reference = get(gr, NAME_reference, 0);
	  int rx = (reference ? valInt(reference->x) : 0);
	  int ry = (reference ? valInt(reference->y) : 0);
	  int iy = py + m.units[x][y].height;
	  int ix = (m.units[x][y].alignment == NAME_column ? px : lx) +
		    					m.units[x][y].left;
	  Int iw = DEFAULT;
	  
					/* hor_stretch handling */
	  if ( m.units[x][y].hstretch )
	  { int nx=0;			/* make compiler happy */

	    if ( x+1 < max_x && notNil(m.units[x+1][y].item) )
	    { nx = m.units[x][y].left + m.units[x][y].right + gapw;

	      if ( m.units[x+1][y].alignment == NAME_column )
		nx += px;
	      else
		nx += ix - rx;
	    } else if ( notDefault(bb) )
	    { nx = valInt(bb->w);
	    } else
	    { iw = toInt(m.units[x][y].left + m.units[x][y].right);
	    }

	    if ( isDefault(iw) )
	      iw = toInt(nx - gapw - (ix - rx));
	  }

	  setGraphical(gr, toInt(ix - rx), toInt(iy - ry), iw, DEFAULT);
	  lx = valInt(gr->area->x) + valInt(gr->area->w) + gapw;
	}
	px += m.units[x][y].left + m.units[x][y].right + gapw;
      }

      py += m.units[0][y].depth + m.units[0][y].height + gaph; 
    }
  }

    ComputeGraphical(d);		/* recompute bounding-box */
    
    for(y = 0; y < max_y; y++)
    { if ( notDefault(bb) )
      { px = valInt(bb->w);
      } else
      { if ( instanceOfObject(d, ClassWindow) )
	{ PceWindow sw = (PceWindow) d;

	  px = valInt(sw->bounding_box->x) +
	       valInt(sw->bounding_box->w) +
	       valInt(gap->w);
	} else
	{ px = valInt(d->area->x) - valInt(d->offset->x) +
	       valInt(d->area->w) + valInt(gap->w);
	} 
      }

      for(x = max_x-1; x >= 0; x--)
      { if ( notNil(gr = m.units[x][y].item) &&
	     gr->displayed == ON )
	{ if ( m.units[x][y].alignment == NAME_right ||
	       m.units[x][y].alignment == NAME_center )
	  { Name algnmt = m.units[x][y].alignment;
	    int x2;
	    Graphical gr2 = NULL, grl = gr;
	    int tw, dx;

	    DEBUG(NAME_layout, Cprintf("%s is aligned %s\n",
				       pp(gr), pp(algnmt)));

	    for(x2 = x-1; x2 >= 0; x2--)
	    { if ( notNil(gr2 = m.units[x2][y].item) &&
		   gr2->displayed == ON )
	      { if ( m.units[x2][y].alignment != algnmt )
		  break;
		else
		{ DEBUG(NAME_layout, Cprintf("\tadding %s\n",
					     pp(m.units[x2][y].item)));
		  grl = gr2;
		}
	      }
	    }
	    
	    tw = valInt(getRightSideGraphical(gr)) - valInt(grl->area->x);

	    if ( m.units[x][y].alignment == NAME_right )
	      dx = px - tw - valInt(gap->w);
	    else
	    { int sx = (x2 < 0 ? 0 : valInt(getRightSideGraphical(gr2)));
	      DEBUG(NAME_layout, Cprintf("sx = %d; ", sx));
	      dx = (px - sx - tw)/2 + sx;
	    }
	    dx -= valInt(getLeftSideGraphical(grl));

	    DEBUG(NAME_layout,
		  Cprintf("R = %d; Total width = %d, shift = %d\n",
			  px, tw, dx));

	    for(; ; x--)
	    { if ( notNil(gr = m.units[x][y].item) &&
		   gr->displayed == ON )
	      { setGraphical(gr, toInt(valInt(gr->area->x) + dx),
			     DEFAULT, DEFAULT, DEFAULT);
		DEBUG(NAME_layout, Cprintf("\t moved %s\n", pp(gr)));
		if ( gr == grl )
		  break;
	      }
	    }
	  }

	  px = valInt(gr->area->x);
	}
      }
    }
  }
  
  free_matrix_columns(&m, max_x);

  { PceWindow sw;

    if ( (sw = getWindowGraphical((Graphical) d)) &&
	 isNil(sw->keyboard_focus) )
      advanceDevice(d, NIL);
  }
      
  succeed;
}

status
appendDialogItemDevice(Device d, Graphical item, Name where)
{ Graphical di;
  Name algmnt;

  if ( emptyChain(d->graphicals) )
    return displayDevice(d, item, DEFAULT);

  di = getTailChain(d->graphicals);
  if ( isDefault(where) )
  { if ( instanceOfObject(di, ClassButton) &&
	 instanceOfObject(item, ClassButton) )
      where = NAME_right;
    else
      where = NAME_nextRow;
  } else if ( where == NAME_right &&
	      (algmnt = get(di, NAME_alignment, 0)) != NAME_column )
    send(item, NAME_alignment, algmnt, 0);

  if ( where == NAME_nextRow )
  { Graphical left;

    while ( (left = get(di, NAME_left, 0)) && notNil(left) )
      di = left;
    where = NAME_below;
  }

  return send(item, where, di, 0);
}


		/********************************
		*         MISCELENEOUS		*
		********************************/


static status
convertLoadedObjectDevice(Device dev, Int ov, Int cv)
{ if ( isNil(dev->recompute) )
    assign(dev, recompute, newObject(ClassChain, 0));
    
  succeed;
}


static status
reparentDevice(Device dev)
{ Cell cell;

  if ( isNil(dev->device) )
    assign(dev, level, ZERO);
  else
    assign(dev, level, add(dev->device->level, ONE));

  for_cell(cell, dev->graphicals)
    qadSendv(cell->value, NAME_reparent, 0, NULL);

  return reparentGraphical((Graphical) dev);
}


static status
roomDevice(Device dev, Area area)
{ register Cell cell;

  ComputeGraphical(dev);
  for_cell(cell, dev->graphicals)
  { Graphical gr = cell->value;

    if ( gr->displayed == ON &&
         overlapArea(gr->area, area) != FAIL )
      fail;
  }

  succeed;
}


static Chain
getInsideDevice(Device dev, Area a)
{ register Cell cell;
  Chain ch;

  ch = answerObject(ClassChain, 0);

  ComputeGraphical(dev);
  for_cell(cell, dev->graphicals)
  { if (insideArea(a, ((Graphical) cell->value)->area) == SUCCEED)
      appendChain(ch, cell->value);
  }

  answer(ch);
}


static status
resizeDevice(Device dev, Real xfactor, Real yfactor, Point origin)
{ float xf, yf;
  int ox = valInt(dev->offset->x);
  int oy = valInt(dev->offset->y);
  Point p;
  Cell cell;

  init_resize_graphical(dev, xfactor, yfactor, origin, &xf, &yf, &ox, &oy);
  if ( xf == 1.0 && yf == 1.0 )
    succeed;

  p = tempObject(ClassPoint, toInt(ox - valInt(dev->offset->x)),
		 	     toInt(oy - valInt(dev->offset->y)), 0);

  for_cell(cell, dev->graphicals)
    send(cell->value, NAME_resize, xfactor, yfactor, p, 0);
  considerPreserveObject(p);

  succeed;
}


		/********************************
		*         NAMED MEMBERS		*
		********************************/

Graphical
getMemberDevice(Device dev, Name name)
{ if ( notNil(dev->graphicals) )
  { Cell cell;

    for_cell(cell, dev->graphicals)
    { if (((Graphical)cell->value)->name == name)
	answer(cell->value);
    }
  }

  fail;
}


static status
forSomeDevice(Device dev, Name name, Code msg)
{ Cell cell, c2;

  for_cell_save(cell, c2, dev->graphicals)
  { Graphical gr = cell->value;

    if ( isDefault(name) || gr->name == name )
      forwardReceiverCode(msg, dev, gr, 0);
  }
  
  succeed;
}


static status
forAllDevice(Device dev, Name name, Code msg)
{ Cell cell, c2;

  for_cell_save(cell, c2, dev->graphicals)
  { Graphical gr = cell->value;

    if ( isDefault(name) || gr->name == name )
      TRY(forwardReceiverCode(msg, dev, gr, 0));
  }
  
  succeed;
}

		/********************************
		*            MOVING		*
		********************************/


static status
updateConnectionsDevice(Device dev, Int level)
{ Cell cell;

  for_cell(cell, dev->graphicals)
  { if ( instanceOfObject(cell->value, ClassDevice) )
      updateConnectionsDevice(cell->value, level);
    else
      updateConnectionsGraphical(cell->value, level);
  }

  return updateConnectionsGraphical((Graphical) dev, level);
}


status
geometryDevice(Device dev, Int x, Int y, Int w, Int h)
{ ComputeGraphical(dev);

  if ( isDefault(x) ) x = dev->area->x;
  if ( isDefault(y) ) y = dev->area->y;

  if ( x != dev->area->x || y != dev->area->y )
  { Int dx = sub(x, dev->area->x);
    Int dy = sub(y, dev->area->y);

    CHANGING_GRAPHICAL(dev,
	assign(dev->offset, x, add(dev->offset->x, dx));
	assign(dev->offset, y, add(dev->offset->y, dy));
	if ( notNil(dev->clip_area) )
	{ assign(dev, badBoundingBox, ON); /* TBD: ??? */
	  computeBoundingBoxDevice(dev);
	} else
	{ assign(dev->area, x, x);
	  assign(dev->area, y, y);
	});

    updateConnectionsDevice(dev, sub(dev->level, ONE));
  }

  succeed;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Quick and dirty	move as used by class text_image to move devices for
event-handling.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

status
textMoveDevice(Device dev, Int x, Int y)
{ ComputeGraphical(dev);

  if ( x != dev->area->x || y != dev->area->y )
  { Int dx = sub(x, dev->area->x);
    Int dy = sub(y, dev->area->y);

    assign(dev->offset, x, add(dev->offset->x, dx));
    assign(dev->offset, y, add(dev->offset->y, dy));

    if ( notNil(dev->clip_area) )
    { assign(dev, badBoundingBox, ON); /* TBD: ??? */
      computeBoundingBoxDevice(dev);
    } else
    { assign(dev->area, x, x);
      assign(dev->area, y, y);
    }
  }

  succeed;
}

		/********************************
		*           REFERENCE		*
		********************************/


static status
referenceDevice(Device dev, Point pos)
{ Int x, y;

  if ( isDefault(pos) )
  { ComputeGraphical(dev);
    x = sub(dev->area->x, dev->offset->x);
    y = sub(dev->area->y, dev->offset->y);
  } else
  { x = pos->x;
    y = pos->y;
  }

  if ( x != ZERO || y != ZERO )
  { Cell cell;
    Point move = tempObject(ClassPoint, sub(ZERO, x), sub(ZERO, y), 0);

    offsetPoint(dev->offset, x, y);
    for_cell(cell, dev->graphicals)
      relativeMoveGraphical(cell->value, move);

    considerPreserveObject(move);
  }

  succeed;
}


static status
set_position_device(Device dev, Int x, Int y)
{ ComputeGraphical(dev);

  if ( isDefault(x) ) x = dev->offset->x;
  if ( isDefault(y) ) y = dev->offset->y;

  x = add(dev->area->x, sub(x, dev->offset->x));
  y = add(dev->area->y, sub(y, dev->offset->y));

  return setGraphical(dev, x, y, DEFAULT, DEFAULT);
}


static status
positionDevice(Device dev, Point pos)
{ return set_position_device(dev, pos->x, pos->y);
}


static status
xDevice(Device dev, Int x)
{ return set_position_device(dev, x, DEFAULT);
}


static status
yDevice(Device dev, Int y)
{ return set_position_device(dev, DEFAULT, y);
}


static Point
getPositionDevice(Device dev)
{ ComputeGraphical(dev);
  answer(dev->offset);
}


static Int
getXDevice(Device dev)
{ answer(getPositionDevice(dev)->x);
}


static Int
getYDevice(Device dev)
{ answer(getPositionDevice(dev)->y);
}


static Point
getOffsetDevice(Device dev)
{ ComputeGraphical(dev);
  answer(dev->offset);
}


		/********************************
		*           CATCH ALL		*
		********************************/

static Any
getCatchAllDevice(Device dev, Name name)
{ Name base;

  if ( (base = getDeleteSuffixName(name, NAME_Member)) )
    answer(getMemberDevice(dev, base));

  fail;
}

		/********************************
		*             VISUAL		*
		********************************/

static Chain
getContainsDevice(Device dev)
{ answer(dev->graphicals);
}

		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */

static char *T_DnameD_code[] =
        { "[name]", "code" };
static char *T_find[] =
        { "at=[point|event]", "condition=[code]" };
static char *T_pointedObjects[] =
        { "at=point|event", "append_to=[chain]" };
static char *T_typed[] =
        { "event_id", "[bool]" };
static char *T_format[] =
        { "format*|name", "[any]" };
static char *T_layout[] =
        { "gap=[size]", "size=[size]" };
static char *T_modifiedItem[] =
        { "graphical", "bool" };
static char *T_display[] =
        { "graphical", "position=[point]" };
static char *T_appendDialogItem[] =
        { "item=graphical", "relative_to_last=[{below,right,next_row}]" };
static char *T_convertLoadedObject[] =
        { "old_version=int", "new_version=int" };
static char *T_changedUnion[] =
        { "ox=int", "oy=int", "ow=int", "oh=int" };
static char *T_geometry[] =
        { "x=[int]", "y=[int]", "width=[int]", "height=[int]" };
static char *T_resize[] =
        { "x_factor=real", "y_factor=[real]", "origin=[point]" };

/* Instance Variables */

static vardecl var_device[] =
{ IV(NAME_level, "int", IV_GET,
     NAME_organisation, "Nesting depth to topmost device"),
  IV(NAME_offset, "point", IV_NONE,
     NAME_area, "Offset of origin"),
  IV(NAME_clipArea, "area*", IV_NONE,
     NAME_scroll, "Clip all graphicals to this area"),
  IV(NAME_graphicals, "chain", IV_GET,
     NAME_organisation, "Displayed graphicals (members)"),
  IV(NAME_pointed, "chain", IV_GET,
     NAME_event, "Graphicals pointed-to by the mouse"),
  IV(NAME_format, "format*", IV_GET,
     NAME_layout, "Use tabular layout"),
  IV(NAME_badFormat, "bool", IV_NONE,
     NAME_update, "Format needs to be recomputed"),
  IV(NAME_badBoundingBox, "bool", IV_NONE,
     NAME_update, "Indicate bounding box is out-of-date"),
  IV(NAME_recompute, "chain", IV_NONE,
     NAME_update, "Graphicals that requested a recompute")
};

/* Send Methods */

static senddecl send_device[] =
{ SM(NAME_geometry, 4, T_geometry, geometryDevice,
     DEFAULT, "Move device"),
  SM(NAME_initialise, 0, NULL, initialiseDevice,
     DEFAULT, "Create an empty device"),
  SM(NAME_unlink, 0, NULL, unlinkDevice,
     DEFAULT, "Clear device and unlink from super-device"),
  SM(NAME_typed, 2, T_typed, typedDevice,
     NAME_accelerator, "Handle accelerators"),
  SM(NAME_foreground, 1, "colour", colourGraphical,
     NAME_appearance, "Default colour for all members"),
  SM(NAME_move, 1, "point", positionDevice,
     NAME_area, "Set origin"),
  SM(NAME_position, 1, "point", positionDevice,
     NAME_area, "Set origin"),
  SM(NAME_reference, 1, "[point]", referenceDevice,
     NAME_area, "Move origin, while moving members opposite"),
  SM(NAME_resize, 3, T_resize, resizeDevice,
     NAME_area, "Resize device with specified factor"),
  SM(NAME_x, 1, "int", xDevice,
     NAME_area, "Set X of origin"),
  SM(NAME_y, 1, "int", yDevice,
     NAME_area, "Set Y of origin"),
  SM(NAME_convertLoadedObject, 2, T_convertLoadedObject,
     convertLoadedObjectDevice,
     NAME_compatibility, "Initialise recompute and request_compute"),
  SM(NAME_event, 1, "event", eventDevice,
     NAME_event, "Process an event"),
  SM(NAME_updatePointed, 1, "event", updatePointedDevice,
     NAME_event, "Update <-pointed, sending area_enter and area_exit events"),
  SM(NAME_advance, 1, "[graphical]*", advanceDevice,
     NAME_focus, "Advance keyboard focus to next item"),
  SM(NAME_forAll, 2, T_DnameD_code, forAllDevice,
     NAME_iterate, "Run code on graphicals; demand acceptance"),
  SM(NAME_forSome, 2, T_DnameD_code, forSomeDevice,
     NAME_iterate, "Run code on all graphicals"),
  SM(NAME_format, 2, T_format, formatDevice,
     NAME_layout, "Use tabular layout"),
  SM(NAME_layoutDialog, 2, T_layout, layoutDialogDevice,
     NAME_layout, "(Re)compute layout of dialog_items"),
  SM(NAME_room, 1, "area", roomDevice,
     NAME_layout, "Test if no graphicals are in area"),
  SM(NAME_appendDialogItem, 2, T_appendDialogItem, appendDialogItemDevice,
     NAME_organisation, "Append dialog_item {below,right,next_row} last"),
  SM(NAME_clear, 0, NULL, clearDevice,
     NAME_organisation, "Erase all graphicals"),
  SM(NAME_display, 2, T_display, displayDevice,
     NAME_organisation, "Display graphical at point"),
  SM(NAME_erase, 1, "graphical", eraseDevice,
     NAME_organisation, "Erase a graphical"),
  SM(NAME_reparent, 0, NULL, reparentDevice,
     NAME_organisation, "Device's parent-chain has changed"),
  SM(NAME_DrawPostScript, 0, NULL, drawPostScriptDevice,
     NAME_postscript, "Create PostScript"),
  SM(NAME_changedUnion, 4, T_changedUnion, changedUnionDevice,
     NAME_resize, "Trap changes to the union of all graphicals"),
  SM(NAME_selection, 1, "graphical|chain*", selectionDevice,
     NAME_selection, "Set selection to graphical or chain"),
  SM(NAME_compute, 0, NULL, computeDevice,
     NAME_update, "Recompute device"),
  SM(NAME_modifiedItem, 2, T_modifiedItem, failObject,
     NAME_virtual, "Trap modification of item (fail)")
};

/* Get Methods */

static getdecl get_device[] =
{ GM(NAME_contains, 0, "chain", NULL, getContainsDevice,
     DEFAULT, "Chain with visuals contained"),
  GM(NAME_offset, 0, "point", NULL, getOffsetDevice,
     NAME_area, "Get origin (also <-position)"),
  GM(NAME_position, 0, "point", NULL, getPositionDevice,
     NAME_area, "Get origin"),
  GM(NAME_x, 0, "int", NULL, getXDevice,
     NAME_area, "Get X of origin"),
  GM(NAME_y, 0, "int", NULL, getYDevice,
     NAME_area, "Get Y of origin"),
  GM(NAME_pointedObjects, 2, "chain", T_pointedObjects,
     getPointedObjectsDevice,
     NAME_event, "New chain holding graphicals at point|event"),
  GM(NAME_catchAll, 1, "graphical", "name", getCatchAllDevice,
     NAME_organisation, "Find named graphicals"),
  GM(NAME_member, 1, "graphical", "graphical_name=name", getMemberDevice,
     NAME_organisation, "Find named graphical"),
  GM(NAME_find, 2, "graphical", T_find, getFindDevice,
     NAME_search, "Find most local graphical"),
  GM(NAME_displayedCursor, 0, "cursor*", NULL, getDisplayedCursorDevice,
     NAME_cursor, "Currently displayed cursor"),
  GM(NAME_inside, 1, "chain", "area", getInsideDevice,
     NAME_selection, "New chain with graphicals inside area"),
  GM(NAME_selection, 0, "chain", NULL, getSelectionDevice,
     NAME_selection, "New chain of selected graphicals")
};

/* Resources */

#define rc_device NULL
/*
static resourcedecl rc_device[] =
{ 
};
*/

/* Class Declaration */

ClassDecl(device_decls,
          var_device, send_device, get_device, rc_device,
          0, NULL,
          "$Rev$");


status
makeClassDevice(Class class)
{ declareClass(class, &device_decls);
  setRedrawFunctionClass(class, RedrawAreaDevice);

  succeed;
}
