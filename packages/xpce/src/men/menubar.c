/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1992 University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>
#include <h/dialog.h>

static status	currentMenuBar(MenuBar mb, PopupObj p);

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
    assign(b->area, y, ZERO);
  }

  return RedrawAreaGraphical(mb, a);
}


static Button
getButtonMenuBar(MenuBar mb, PopupObj p)
{ Cell cell;

  for_cell(cell, mb->buttons)
  { Button b = cell->value;
    if ( b->popup == p )
      answer(b);
  }

  fail;
}


static status
changedMenuBarButton(MenuBar mb, Any obj)
{ Button b = DEFAULT;

  if ( instanceOfObject(obj, ClassPopup) )
    b = getButtonMenuBar(mb, obj);
  else
    b = obj;

  if ( isDefault(b) )
    changedDialogItem(mb);
  else if ( instanceOfObject(b, ClassButton) )
  { Area a = b->area;

    changedImageGraphical(mb, a->x, a->y, a->w, a->h);
  }

  succeed;
}


static status
computeMenuBar(MenuBar mb)
{ Cell cell;
  int x = 0;
  int gap;
  int h = 0;

  obtainResourcesObject(mb);
  gap = valInt(mb->gap);

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
	changedDialogItem(mb));

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


static status
showPopupMenuBar(MenuBar mb, PopupObj p)
{ Button b = getButtonMenuBar(mb, p);
  Point pos = tempObject(ClassPoint, b->area->x, mb->area->h, 0);
  
  if ( notNil(mb->current) && mb->current->displayed == ON )
    send(mb->current, NAME_close, 0);
  currentMenuBar(mb, p);
  send(mb->current, NAME_update, mb, 0);
  send(mb->current, NAME_open, mb, pos, OFF, OFF, ON, 0);
  considerPreserveObject(pos);

  succeed;
}


		/********************************
		*            EVENTS		*
		********************************/

static PopupObj
getPopupFromEventMenuBar(MenuBar mb, EventObj ev)
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
      return b->popup;
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
{ PopupObj p;

  if ( mb->active == OFF )
    fail;

  if ( isDownEvent(ev) )
    assign(mb, button, getButtonEvent(ev));

  if ( notNil(mb->current) )
  { if ( isDragEvent(ev) )
    { if ( (p = getPopupFromEventMenuBar(mb, ev)) && p != mb->current )
	showPopupMenuBar(mb, p);
      postEvent(ev, (Graphical) mb->current, DEFAULT);
    } else if ( isUpEvent(ev) )
    { PceWindow sw = ev->window;
      PopupObj p;

      if ( valInt(getClickTimeEvent(ev)) < 400 && /* CLICK: stay-up */
	   valInt(getClickDisplacementEvent(ev)) < 10 &&
	   getAttributeObject(mb, NAME_Stayup) != ON )
      { attributeObject(mb, NAME_Stayup, ON);
	grabPointerWindow(sw, ON);
	focusWindow(sw, (Graphical) mb, DEFAULT, DEFAULT, NIL);
      } else if ( (p = getPopupFromEventMenuBar(mb, ev)) &&
		  mb->current != p &&
		  getAttributeObject(mb, NAME_Stayup) == ON )
      { showPopupMenuBar(mb, p);	/* stayup: open next */
	generateEventGraphical((Graphical)mb, NAME_msLeftDrag);
      } else
      { int grabbed = (getAttributeObject(mb, NAME_Stayup) == ON);

	if ( grabbed )
	  grabPointerWindow(sw, OFF);

	postEvent(ev, (Graphical) mb->current, DEFAULT);

	if ( mb->current->displayed == OFF ) /* popup has closed */
	{ PopupObj current = mb->current;

	  assign(mb, current, NIL);
	  send(current, NAME_execute, mb, 0);
	  if ( !onFlag(mb, F_FREED|F_FREEING) )
	    changedMenuBarButton(mb, current);
	}

	if ( !onFlag(mb, F_FREED|F_FREEING) &&
	     !onFlag(sw, F_FREED|F_FREEING) )
	{ if ( isNil(mb->current) )
	  { focusWindow(sw, NIL, NIL, NIL, NIL);
	    deleteAttributeObject(mb, NAME_Stayup);
	  } else if ( grabbed )
	  { grabPointerWindow(sw, ON);
	  }
	}
      }
    } else
      postEvent(ev, (Graphical) mb->current, DEFAULT);

    succeed;
  } else
  { if ( isDownEvent(ev) )
    { if ( (p = getPopupFromEventMenuBar(mb, ev)) )
      { showPopupMenuBar(mb, p);
	postEvent(ev, (Graphical) mb->current, DEFAULT);
	focusCursorGraphical((Graphical)mb,
			     getResourceValueObject(p, NAME_cursor));
	succeed;
      }
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
    obtainResourcesObject(mb);
    if ( mb->look != NAME_openLook )
    { assign(b, label_font, mb->label_font);
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
        changedMenuBarButton(mb, p));
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
  { changedMenuBarButton(mb, mb->current);
    assign(mb, current, p);
    if ( notNil(p) && notNil(mb->button) )
      assign(mb->current, button, mb->button);
    changedMenuBarButton(mb, mb->current);
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
    currentMenuBar(mb, NIL);
  }

  succeed;
}

		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */

static const char *T_activeMember[] =
        { "popup=member:popup", "active=bool" };

/* Instance Variables */

static const vardecl var_menuBar[] =
{ IV(NAME_members, "chain", IV_GET,
     NAME_organisation, "The pulldown menus"),
  IV(NAME_format, "{left,center,right}", IV_GET,
     NAME_appearance, "Format of labels in their box"),
  SV(NAME_labelFont, "font", IV_GET|IV_STORE, labelFontMenuBar,
     NAME_appearance, "Font used to display the labels"),
  IV(NAME_current, "popup*", IV_GET,
     NAME_event, "Currently visible popup"),
  IV(NAME_button, "button_name*", IV_NONE,
     NAME_event, "Button that caused me to start"),
  IV(NAME_buttons, "chain", IV_GET,
     NAME_repaint, "Chain holding the buttons"),
  IV(NAME_gap, "int", IV_GET,
     NAME_appearance, "Distance between buttons"),
  IV(NAME_radius, "int", IV_GET,
     NAME_appearance, "Radius for the buttons")
};

/* Send Methods */

static const senddecl send_menuBar[] =
{ SM(NAME_compute, 0, NULL, computeMenuBar,
     DEFAULT, "Recompute the menu-bar"),
  SM(NAME_event, 1, "event", eventMenuBar,
     DEFAULT, "Process an event"),
  SM(NAME_initialise, 1, "name=[name]", initialiseMenuBar,
     DEFAULT, "Create from a label"),
  SM(NAME_reset, 0, NULL, resetMenuBar,
     NAME_abort, "Reset menubar after an abort"),
  SM(NAME_key, 1, "key=name", keyMenuBar,
     NAME_accelerator, "Execute (first) menu_item with accelerator"),
  SM(NAME_activeMember, 2, T_activeMember, activeMemberMenuBar,
     NAME_active, "(De)activate popup or name"),
  SM(NAME_allActive, 1, "bool", allActiveMenuBar,
     NAME_active, "(De)activate all popups"),
  SM(NAME_allOff, 1, "member:popup", allOffMenuBar,
     NAME_active, "Deactivate all items of popup or name"),
  SM(NAME_allOn, 1, "member:popup", allOnMenuBar,
     NAME_active, "Activate all items of popup or name"),
  SM(NAME_off, 1, "name|menu_item", offMenuBar,
     NAME_active, "Deactivate menu_item or name"),
  SM(NAME_on, 1, "name|menu_item", onMenuBar,
     NAME_active, "Activate menu_item or name"),
  SM(NAME_showPopup, 1, "popup", showPopupMenuBar,
     NAME_event, "Make popup <-current and ->open it"),
  SM(NAME_append, 1, "popup", appendMenuBar,
     NAME_organisation, "Append a popup to the menubar"),
  SM(NAME_clear, 0, NULL, clearMenuBar,
     NAME_organisation, "Remove all menus from the menu_bar"),
  SM(NAME_delete, 1, "member:popup", deleteMenuBar,
     NAME_organisation, "Delete popup or name")
};

/* Get Methods */

static const getdecl get_menuBar[] =
{ GM(NAME_contains, 0, "any", NULL, getContainsMenuBar,
     DEFAULT, "Chain with popup menus contained"),
  GM(NAME_reference, 0, "point", NULL, getReferenceMenuBar,
     DEFAULT, "Reference of first button"),
  GM(NAME_popupFromEvent, 1, "popup", "event", getPopupFromEventMenuBar,
     NAME_event, "Find popup to open from event on menu_bar"),
  GM(NAME_member, 1, "popup", "name|popup", getMemberMenuBar,
     NAME_organisation, "Find popup from name")
};

/* Resources */

static const resourcedecl rc_menuBar[] =
{ RC(NAME_format, "name", "center",
     "Format items {left,center,right}"),
  RC(NAME_gap, "int", "-1",
     "Distance between buttons"),
  RC(NAME_labelFont, "font", "normal",
     "Default font for labels"),
  RC(NAME_pen, "int", "1",
     "Thickness of line around labels"),
  RC(NAME_radius, "int", "0",
     "Rounding radius of the buttons"),
  RC(NAME_size, "size", "size(80,20)",
     "Minimum size for labels")
};

/* Class Declaration */

static Name menuBar_termnames[] = { NAME_label };

ClassDecl(menuBar_decls,
          var_menuBar, send_menuBar, get_menuBar, rc_menuBar,
          1, menuBar_termnames,
          "$Rev$");


status
makeClassMenuBar(Class class)
{ declareClass(class, &menuBar_decls);
  setRedrawFunctionClass(class, RedrawAreaMenuBar);

  succeed;
}
