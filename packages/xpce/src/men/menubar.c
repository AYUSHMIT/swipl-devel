/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1992 University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>
#include <h/dialog.h>

static status	currentMenuBar(MenuBar mb, PopupObj p);

#define X_GAP 15

static status
initialiseMenuBar(MenuBar mb, Name name)
{ createDialogItem(mb, name);

  assign(mb, members, newObject(ClassChain, 0));
  assign(mb, buttons, newObject(ClassChain, 0));
  assign(mb, label_font, DEFAULT);
  assign(mb, pen, DEFAULT);
  assign(mb, format, DEFAULT);
  assign(mb, gap, DEFAULT);
  assign(mb, radius, DEFAULT);

  obtainResourcesObject(mb);
  succeed;
}


static status
RedrawAreaMenuBar(MenuBar mb, Area a)
{ Cell cell;
  Int x = mb->area->x;

  for_cell(cell, mb->buttons)
  { Button b = cell->value;

    assign(b->area, x, add(b->area->x, x));
    assign(b->area, y, mb->area->y);
    if ( overlapArea(b->area, a) )
    { assign(b, device, mb->device);
      assign(b, active, b->popup->active);
      assign(b, status, b->popup == mb->current ? NAME_preview
	     					: NAME_inactive);
      RedrawAreaButton(b, a);
      assign(b, device, NIL);
    }
    assign(b->area, x, sub(b->area->x, x));
  }

  return RedrawAreaGraphical(mb, a);
}


static status
computeMenuBar(MenuBar mb)
{ Cell cell;
  int x = 0;
  int gap = valInt(mb->gap);
  int h = 0;

  for_cell(cell, mb->buttons)
  { Button b = cell->value;

    ComputeGraphical(b);
    assign(b->area, x, toInt(x));

    x += valInt(b->area->w) + gap;
    if ( valInt(b->area->h) > h )
      h = valInt(b->area->h);
  }

  CHANGING_GRAPHICAL(mb,
	assign(mb->area, w, x > 0 ? toInt(x-gap) : ZERO);
	assign(mb->area, h, toInt(h));
	changedEntireImageGraphical(mb));

  succeed;
}


static Point
getReferenceMenuBar(MenuBar mb)
{ Button b = getHeadChain(mb->buttons);
  Point ref;

  if ( b && (ref = getReferenceButton(b)) )
    answer(ref);

  return getReferenceDialogItem(mb);
}


		/********************************
		*            EVENTS		*
		********************************/

static PopupObj
find_popup_menu_bar(MenuBar mb, EventObj ev, int *X, int *Y)
{ int ex, ey;
  Int EX, EY;
  Cell cell;

  get_xy_event(ev, mb, ON, &EX, &EY);
  ex = valInt(EX); ey = valInt(EY);

  if ( ey < 0 || ey >= valInt(mb->area->h) )
    fail;

  for_cell(cell, mb->buttons)
  { Button b = cell->value;

    if ( ex >= valInt(b->area->x) &&
	 ex <= valInt(b->area->x) + valInt(b->area->w) )
    { *X = valInt(b->area->x);
      *Y = valInt(b->area->h);

      return b->popup;
    }
  }

  fail;
}


static status
keyMenuBar(MenuBar mb, Name key)
{ Cell cell;

  if ( mb->active == OFF )
    fail;

  for_cell(cell, mb->members)
  { PopupObj p = cell->value;

    if ( keyPopup(p, key) )
    { currentMenuBar(mb, p);
      flushGraphical(mb);
      send(p, NAME_execute, mb, 0);
      if ( !isFreedObj(mb) )
	currentMenuBar(mb, NIL);

      succeed;
    }
  }

  fail;
}


static status
eventMenuBar(MenuBar mb, EventObj ev)
{ int ix, iy;
  PopupObj p;
  Point pos;

  if ( mb->active == OFF )
    fail;

  if ( notNil(mb->current) )
  { postEvent(ev, (Graphical) mb->current, DEFAULT);

    if ( isDragEvent(ev) )
    { if ( (p = find_popup_menu_bar(mb, ev, &ix, &iy)) &&
	   p != mb->current )
      { send(mb->current, NAME_close, 0);
	assign(mb, current, p);
	pos = tempObject(ClassPoint, toInt(ix), toInt(iy), 0);
	assign(mb->current, button, mb->button);
	changedDialogItem(mb);
	send(mb->current, NAME_update, mb, 0);
	send(mb->current, NAME_open, mb, pos, OFF, OFF, ON, 0);
	considerPreserveObject(pos);
	postEvent(ev, (Graphical) mb->current, DEFAULT);
      }
    } else if ( isUpEvent(ev) && mb->button == getButtonEvent(ev) )
    { send(mb->current, NAME_execute, mb, 0);
      assign(mb, current, NIL);
      if ( !isFreedObj(mb) )
	changedDialogItem(mb);
    }
  } else if ( isDownEvent(ev) )
  { assign(mb, button, getButtonEvent(ev));

    if ( (p = find_popup_menu_bar(mb, ev, &ix, &iy)) )
    { assign(mb, current, p);
      pos = tempObject(ClassPoint, toInt(ix), toInt(iy), 0);
      changedDialogItem(mb);
      send(p, NAME_update, mb, 0);
      send(p, NAME_open, mb, pos, OFF, OFF, ON, 0);
      considerPreserveObject(pos);
      postEvent(ev, (Graphical) mb->current, DEFAULT);
      focusCursorGraphical((Graphical)mb,
			   getResourceValueObject(p, NAME_cursor));

      succeed;
    }
  }

  return eventDialogItem(mb, ev);
}

		/********************************
		*          ATTRIBUTES		*
		********************************/

static status
labelFontMenuBar(MenuBar mb, FontObj font)
{ if ( mb->label_font != font )
  { assign(mb, label_font, font);
    requestComputeGraphical(mb, DEFAULT);
  }

  succeed;
}


static status
clearMenuBar(MenuBar mb)
{ clearChain(mb->members);
  clearChain(mb->buttons);

  return requestComputeGraphical(mb, DEFAULT);
}


static status
appendMenuBar(MenuBar mb, PopupObj p)
{ if ( !memberChain(mb->members, p) )
  { Button b;

    appendChain(mb->members, p);
    appendChain(mb->buttons, b=newObject(ClassButton, p->label, NIL, 0));
    assign(b, popup, p);
    if ( mb->look != NAME_openLook )
    { assign(b, font, mb->label_font);
      assign(b, pen, mb->pen);
      assign(b, radius, mb->radius);
    }
    send(p, NAME_format, getSlotObject(mb, NAME_format), 0);
    requestComputeGraphical(mb, DEFAULT);
  }
  
  succeed;
}


static PopupObj
getMemberMenuBar(MenuBar mb, Any obj)
{ if ( isName(obj) )
  { Cell cell;

    for_cell(cell, mb->members)
    { PopupObj p = cell->value;
      if ( p->name == obj )
      	answer(p);
    }
    fail;
  } else if ( memberChain(mb->members, obj) )
    answer(obj);
  else
    fail;
}


static status
deleteMenuBar(MenuBar mb, PopupObj p)
{ deleteChain(mb->members, p);
  requestComputeGraphical(mb, DEFAULT);

  succeed;
}


static status
activeMemberMenuBar(MenuBar mb, PopupObj p, Bool val)
{ if ( p->active != val )
  { CHANGING_GRAPHICAL(mb,
	assign(p, active, val);
	changedDialogItem(mb));
  }
  
  succeed;
}


static status
allActiveMenuBar(MenuBar mb, Bool val)
{ Cell cell;

  CHANGING_GRAPHICAL(mb,
	for_cell(cell, mb->members)
	{ PopupObj p = cell->value;
	  assign(p, active, val);
	}
	changedDialogItem(mb));

  succeed;
}


static status
onMenuBar(MenuBar mb, Any obj)
{ Cell cell;

  for_cell(cell, mb->members)
    send(cell->value, NAME_on, obj, 0);

  succeed;
}


static status
offMenuBar(MenuBar mb, Any obj)
{ Cell cell;

  for_cell(cell, mb->members)
    send(cell->value, NAME_off, obj, 0);

  succeed;
}


static status
allOnMenuBar(MenuBar mb, PopupObj p)
{ return send(p, NAME_allOn, 0);
}


static status
allOffMenuBar(MenuBar mb, PopupObj p)
{ return send(p, NAME_allOff, 0);
}


static status
currentMenuBar(MenuBar mb, PopupObj p)
{ if ( mb->current != p )
  { assign(mb, current, p);
    changedDialogItem(mb);		/* TBD: optimize */
  }

  succeed;
}


		/********************************
		*             VISUAL		*
		********************************/

static Chain
getContainsMenuBar(MenuBar mb)
{ answer(mb->members);
}


static status
resetMenuBar(MenuBar mb)
{ if ( notNil(mb->current) )
  { send(mb->current, NAME_close, 0);
    assign(mb, current, NIL);
    changedDialogItem(mb);
  }

  succeed;
}

status
makeClassMenuBar(Class class)
{ sourceClass(class, makeClassMenuBar, __FILE__, "$Revision$");

  localClass(class, NAME_members, NAME_organisation, "chain", NAME_get,
	     "The pulldown menus");
  localClass(class, NAME_format, NAME_appearance,
	     "{left,center,right}", NAME_get,
	     "Format of labels in their box");
  localClass(class, NAME_labelFont, NAME_appearance, "font", NAME_get,
	     "Font used to display the labels");
  localClass(class, NAME_current, NAME_internal, "popup*", NAME_none,
	     "Currently visible popup");
  localClass(class, NAME_button, NAME_event, "button_name*", NAME_none,
	     "Button that caused me to start");
  localClass(class, NAME_buttons, NAME_repaint, "chain", NAME_get,
	     "Chain holding the buttons");
  localClass(class, NAME_gap, NAME_appearance, "int", NAME_get,
	     "Distance between buttons");
  localClass(class, NAME_radius, NAME_appearance, "int", NAME_get,
	     "Radius for the buttons");

  termClass(class, "menu_bar", 1, NAME_label);
  setRedrawFunctionClass(class, RedrawAreaMenuBar);

  storeMethod(class, NAME_labelFont, labelFontMenuBar);

  sendMethod(class, NAME_initialise, DEFAULT, 1, "name=[name]",
	     "Create from a label",
	     initialiseMenuBar);
  sendMethod(class, NAME_reset, NAME_abort, 0,
	     "Reset menubar after an abort",
	     resetMenuBar);
  sendMethod(class, NAME_compute, DEFAULT, 0,
	     "Recompute the menu-bar",
	     computeMenuBar);
  sendMethod(class, NAME_event, DEFAULT, 1, "event",
	     "Process an event",
	     eventMenuBar);
  sendMethod(class, NAME_delete, NAME_organisation, 1, "member:popup",
	     "Delete popup or name",
	     deleteMenuBar);
  sendMethod(class, NAME_on, NAME_active, 1, "name|menu_item",
	     "Activate menu_item or name",
	     onMenuBar);
  sendMethod(class, NAME_off, NAME_active, 1, "name|menu_item",
	     "Deactivate menu_item or name",
	     offMenuBar);
  sendMethod(class, NAME_allOn, NAME_active, 1, "member:popup",
	     "Activate all items of popup or name",
	     allOnMenuBar);
  sendMethod(class, NAME_allOff, NAME_active, 1, "member:popup",
	     "Deactivate all items of popup or name",
	     allOffMenuBar);
  sendMethod(class, NAME_append, NAME_organisation, 1, "popup",
	     "Append a popup to the menubar",
	     appendMenuBar);
  sendMethod(class, NAME_clear, NAME_organisation, 0,
	     "Remove all menus from the menu_bar",
	     clearMenuBar);
  sendMethod(class, NAME_allActive, NAME_active, 1, "bool",
	     "(De)activate all popups",
	     allActiveMenuBar);
  sendMethod(class, NAME_activeMember, NAME_active, 2,
	     "popup=member:popup", "active=bool",
	     "(De)activate popup or name",
	     activeMemberMenuBar);
  sendMethod(class, NAME_key, NAME_accelerator, 1, "key=name",
	     "Execute (first) menu_item with accelerator",
	     keyMenuBar);

  getMethod(class, NAME_member, NAME_organisation, "popup", 1, "name|popup",
	    "Find popup from name",
	    getMemberMenuBar);
  getMethod(class, NAME_contains, DEFAULT, "any", 0,
	    "Chain with popup menus contained",
	    getContainsMenuBar);
  getMethod(class, NAME_reference, DEFAULT, "point", 0,
	    "Reference of first button",
	    getReferenceMenuBar);


  attach_resource(class, "label_font", "font",    "@helvetica_bold_14",
		  "Default font for labels");
  attach_resource(class, "size",       "size",    "size(80,20)",
		  "Mimimum size for labels");
  attach_resource(class, "pen",	       "int", "1",
		  "Thickness of line around labels");
  attach_resource(class, "format",     "name", "center",
		  "Format items {left,center,right}");
  attach_resource(class, "gap", "int", "-1",
		  "Distance between buttons");
  attach_resource(class, "radius", "int", "0",
		  "Rounding radius of the buttons");

  succeed;
}
