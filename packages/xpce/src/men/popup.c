/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1992 University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>
#include <h/dialog.h>

static status closePopup(PopupObj);

static status
initialisePopup(PopupObj p, Name label, Code msg)
{ if ( isDefault(label) )
    label = NAME_options;

  assign(p, update_message, NIL);
  assign(p, button,	    NAME_right);
  assign(p, default_item,   DEFAULT);	/* resource */
  assign(p, show_current,   OFF);
  initialiseMenu((Menu) p, label, NAME_popup, msg);

  succeed;
}


		/********************************
		*             WINDOW		*
		********************************/


static Chain windows = NIL;

static PceWindow
createPopupWindow(DisplayObj d)
{ Cell cell;
  PceWindow sw;
  Any frame;

  if ( isNil(windows) )
    windows = globalObject(NAME_PopupWindows, ClassChain, 0);

  for_cell(cell, windows)
  { sw = cell->value;

    if ( emptyChain(sw->graphicals) && sw->frame->display == d )
      return sw;
  }


  sw = newObject(ClassDialog, NAME_popup, DEFAULT, d, 0);

  send(sw, NAME_kind, NAME_popup, 0);
  send(sw, NAME_pen, ZERO, 0);
  send(sw, NAME_create, 0);
  frame = get(sw, NAME_frame, 0);
  send(frame, NAME_border, ONE, 0);
  send(getTileFrame(frame), NAME_border, ZERO, 0);

  appendChain(windows, sw);
  
  return sw;
}


		/********************************
		*            UPDATE		*
		********************************/

static Any updateContext;		/* HACK of pullright menus */

static status
updatePopup(PopupObj p, Any context)
{ updateContext = context;

  if ( notNil(p->update_message) )
    forwardReceiverCode(p->update_message, p, context, 0);

  return updateMenu((Menu) p, context);
}


static status
resetPopup(PopupObj p)
{ return closePopup(p);
}


static MenuItem
getDefaultMenuItemPopop(PopupObj p)
{ Cell cell;

  if ( isNil(p->default_item) ||
       equalName(p->default_item, NAME_first) )
  { for_cell(cell, p->members)
    { MenuItem mi = cell->value;

      if ( mi->active == ON )
	answer(mi);
    }

    fail;
  }

  if ( equalName(p->default_item, NAME_selection) )
  { for_cell(cell, p->members)
    { MenuItem mi = cell->value;

      if ( mi->selected == ON )
	answer(mi);
    }

    fail;
  }

  answer(findMenuItemMenu((Menu) p, (Any) p->default_item));
}

		/********************************
		*           OPEN/CLOSE		*
		********************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Show a popup on a graphical.  Pos is the position relative to the graphical
on which to display the popup.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static status
openPopup(PopupObj p, Graphical gr, Point pos,
	  Bool pos_is_pointer, Bool warp_pointer, Bool ensure_on_display)
{ int dw, dh;				/* Display width and height */
  PceWindow sw;
  int moved = FALSE;			/* Cursor needs be moved */
  int cx, cy;				/* mouse X-Y */
  int px, py;				/* Popup X-Y */
  int pw, ph;				/* Popup W-H */
  int dx, dy;				/* Popup-Pointer offset */
  Point offset;
  DisplayObj d = CurrentDisplay(gr);
  MenuItem mi;

  if ( emptyChain(p->members) )
    fail;

  if ( isDefault(pos_is_pointer) )	pos_is_pointer = ON;
  if ( isDefault(warp_pointer) )	warp_pointer = ON;
  if ( isDefault(ensure_on_display) )	ensure_on_display = ON;

  dw = valInt(getWidthDisplay(d));
  dh = valInt(getHeightDisplay(d));

  sw = createPopupWindow(d);
  send(sw, NAME_display, p, 0);

  if ( !(offset = getDisplayPositionGraphical(gr)) )
    return errorPce(p, NAME_graphicalNotDisplayed, gr);

  plusPoint(pos, offset);
  doneObject(offset);

					/* get sizes and coordinates */
  ComputeGraphical((Graphical) p);
  dy = valInt(p->area->y);
  dx = valInt(p->area->x);

  if ( (mi = getDefaultMenuItemPopop(p)) != FAIL )
  { int ix, iy, iw, ih;
    area_menu_item((Menu) p, mi, &ix, &iy, &iw, &ih);
    dy += iy +ih/2;
    dx += ix;
  } else
  { mi = NIL;
    dy += 10;
  }

  if ( notNil(p->default_item) )
  { dx += 2;
    previewMenu((Menu) p, mi);
  } else
  { dx = -4;
    previewMenu((Menu) p, NIL);
  }
  pw = valInt(p->area->w);
  ph = valInt(p->area->h);

  if ( pos_is_pointer == ON )
  { cx = valInt(pos->x);
    cy = valInt(pos->y);
    px = cx - dx;
    py = cy - dy;
  } else
  { px = valInt(pos->x);
    py = valInt(pos->y);
    cx = px + dx;
    cy = py + dy;
    moved = TRUE;
  }

  if ( ensure_on_display == ON )
  { if ( px < 0 )       moved = TRUE, px = 0;
    if ( py < 0 )       moved = TRUE, py = 0;
    if ( px + pw > dw ) moved = TRUE, px = dw - pw;
    if ( py + ph > dh ) moved = TRUE, py = dh - ph;
  }

  send(get(sw, NAME_frame, 0), NAME_set,
       toInt(px), toInt(py), toInt(pw), toInt(ph), 0);
  send(sw, NAME_show, ON, 0);
  if ( moved && warp_pointer == ON )
  { Point pos = tempObject(ClassPoint, toInt(dx), toInt(dy), 0);
    send(sw, NAME_pointer, pos, 0);
    considerPreserveObject(pos);
  }

  send(sw, NAME_sensitive, ON, 0);

  succeed;
}


static status
closePopup(PopupObj p)
{ PceWindow sw;

  if ( notNil(p->pullright) )
  { send(p->pullright, NAME_close, 0);
    assign(p, pullright, NIL);
  }

  if ( notNil(sw = (PceWindow) p->device) )
  { send(sw, NAME_show, OFF, 0);
    send(sw, NAME_sensitive, OFF, 0);
    send(sw, NAME_clear, 0);
    assign(p, displayed, OFF);
  }

  succeed;
}


		/********************************
		*         EVENT HANDLING	*
		********************************/

status
keyPopup(PopupObj p, Name key)
{ Cell cell;

  for_cell(cell, p->members)
  { MenuItem mi = cell->value;
    
    if ( (mi->accelerator == key && mi->active == ON) ||
	 (notNil(mi->popup) && keyPopup(mi->popup, key)) )
    { assign(p, selected_item, mi);
      succeed;
    }
  }

  fail;
}


static status
execute_popup(PopupObj p, Any context)
{ if ( p->kind == NAME_cyclePopup )
  { Menu m = context;

    if ( instanceOfObject(m, ClassMenu) )
    { if ( notNil(p->selected_item) )
      { selectionMenu(m, p->selected_item);
	flushGraphical(m);
	forwardMenu(m, m->message, EVENT->value);
      }
    } else
      return errorPce(context, NAME_unexpectedType, ClassMenu);
  } else
  { Code def_msg = DEFAULT;

    for( ; instanceOfObject(p, ClassPopup); p = p->selected_item )
    { if ( notDefault(p->message) )
	def_msg = p->message;

      if ( instanceOfObject(p->selected_item, ClassMenuItem) )
      { MenuItem mi = p->selected_item;
    
	if ( p->multiple_selection == ON )
	{ toggleMenu((Menu) p, mi);
	  if ( isDefault(mi->message) )
	  { if ( notDefault(def_msg) && notNil(def_msg) )
	      forwardReceiverCode(def_msg, p,
				  mi->value, mi->selected, context, 0);
	  } else if ( notNil(mi->message) )
	    forwardReceiverCode(mi->message, p, mi->selected, context, 0);
	} else
	{ if ( isDefault(mi->message) )
	  { if ( notDefault(def_msg) && notNil(def_msg) )
	      forwardReceiverCode(def_msg, p, mi->value, context, 0);
	  } else if ( notNil(mi->message) )
	    forwardReceiverCode(mi->message, p, context, 0);
	}

	succeed;
      }
    }
  }

  succeed;
}


static status
executePopup(PopupObj p, Any context)
{ status rval;
  DisplayObj d = CurrentDisplay(context);

  busyCursorDisplay(d, DEFAULT, DEFAULT);
  rval = execute_popup(p, context);
  busyCursorDisplay(d, NIL, DEFAULT);

  return rval;
}


static int
pullright_x_offset(PopupObj p)
{ int ix = valInt(p->item_offset->x) +
	   valInt(p->item_size->w) -
	   valInt(p->border);

  if ( notNil(p->popup_image) )
    ix -= valInt(p->popup_image->size->w);
  else
    ix -= 8;

  return ix;
}


static status
showPullrightMenuPopup(PopupObj p, MenuItem mi, EventObj ev, Any context)
{ if ( isDefault(context) && validPceDatum(updateContext) )
    context = updateContext;

  send(mi->popup, NAME_update, context, 0);

  if ( !emptyChain(mi->popup->members) )
  { Point pos;		/* Create PULLRIGHT */
    int ih = valInt(p->item_size->h);
    int ic = valInt(getCenterYMenuItemMenu((Menu)p, mi));
    int ix = pullright_x_offset(p);
	    
    previewMenu((Menu) p, mi);
    pos = tempObject(ClassPoint, toInt(ix), toInt(ic - ih/2), 0);
	    
    assign(p, pullright, mi->popup);
    send(p->pullright, NAME_open, p, pos, OFF, OFF, ON, 0);
    considerPreserveObject(pos);
    assign(p->pullright, button, p->button);
    if ( notDefault(ev) )
      postEvent(ev, (Graphical) p->pullright, DEFAULT);

    succeed;
  }

  fail;
}


static status
dragPopup(PopupObj p, EventObj ev, Bool check_pullright)
{ MenuItem mi;

  if ( !(mi = getItemFromEventMenu((Menu) p, ev)) )
    previewMenu((Menu) p, NIL);
  else
  { if ( mi->active == ON )
    { previewMenu((Menu) p, mi);

      if ( notNil(mi->popup) && check_pullright != OFF )
      { Int ex, ey;
	int ix = pullright_x_offset(p);

	get_xy_event(ev, p, ON, &ex, &ey);
	if ( valInt(ex) > ix )
	  send(p, NAME_showPullrightMenu, mi, ev, 0);
      }
    } else
      previewMenu((Menu) p, NIL);
  }

  succeed;
}


static status
eventPopup(PopupObj p, EventObj ev)
{ 					/* Showing PULLRIGHT menu */
  if ( notNil(p->pullright) )
  { postEvent(ev, (Graphical) p->pullright, DEFAULT);

    if ( isDragEvent(ev) )
    { if ( isNil(p->pullright->preview) )
      { MenuItem mi;

	if ( (mi = getItemFromEventMenu((Menu) p, ev)) )
	{ send(p->pullright, NAME_close, 0);
	  assign(p, pullright, NIL);
	  return send(p, NAME_drag, ev, 0);
	}
      }
    } else if ( isUpEvent(ev) &&
		getButtonEvent(ev) == p->pullright->button &&
		isNil(p->pullright->pullright) )
    { if ( notNil(p->pullright->selected_item) )
    	assign(p, selected_item, p->pullright);
      else
      	assign(p, selected_item, NIL);
      assign(p, pullright, NIL);
      send(p, NAME_close, 0);
    }

    succeed;
  }

					/* UP: execute */
  if ( isUpEvent(ev) )
  { if ( notNil(p->preview) &&
	 notNil(p->preview->popup) &&
	 valInt(getClickTimeEvent(ev)) < 400 &&
	 valInt(getClickDisplacementEvent(ev)) < 10 )
      send(p, NAME_showPullrightMenu, p->preview, 0);
    else if ( getButtonEvent(ev) == p->button )
    { assign(p, selected_item, p->preview);
      send(p, NAME_close, 0);
      succeed;
    }
  } else if ( isDownEvent(ev) )		/* DOWN: set button */
  { assign(p, selected_item, NIL);
    assign(p, button, getButtonEvent(ev));
    send(p, NAME_drag, ev, OFF, 0);
    succeed;
  } else if ( isDragEvent(ev) )		/* DRAG: highlight entry */
  { send(p, NAME_drag, ev, 0);
    succeed;
  } else if ( isAEvent(ev, NAME_locMove) )
  { MenuItem mi = getItemFromEventMenu((Menu) p, ev);

    previewMenu((Menu) p, mi && mi->active == ON ? mi : NIL);
  }

  fail;
}


		/********************************
		*            MENU ITEM		*
		********************************/


static status
endGroupPopup(PopupObj p, Bool val)
{ if ( notNil(p->context) )
    return send(p->context, NAME_endGroup, val, 0);

  fail;
}


status
defaultPopupImages(PopupObj p)
{ if ( p->show_current == ON )
  { if ( p->multiple_selection == ON && p->look == NAME_win )
      assign(p, on_image, MS_MARK_IMAGE);
    else
      assign(p, on_image, NAME_marked);
  } else
    assign(p, on_image, NIL);

  assign(p, off_image, NIL);

  succeed;
}


static status
showCurrentPopup(PopupObj p, Bool show)
{ assign(p, show_current, show);

  return defaultPopupImages(p);
}


		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */

static const char *T_drag[] =
        { "event", "check_pullright=[bool]" };
static const char *T_showPullrightMenu[] =
        { "item=menu_item", "event=[event]", "context=[any]" };
static const char *T_initialise[] =
        { "name=[name]", "message=[code]*" };
static const char *T_open[] =
        { "on=graphical", "offset=point", "offset_is_pointer=[bool]", "warp=[bool]", "ensure_on_display=[bool]" };

/* Instance Variables */

static const vardecl var_popup[] =
{ IV(NAME_context, "any*", IV_BOTH,
     NAME_context, "Invoking context"),
  IV(NAME_updateMessage, "code*", IV_BOTH,
     NAME_active, "Ran just before popup is displayed"),
  IV(NAME_pullright, "popup*", IV_NONE,
     NAME_part, "Currently shown pullright menu"),
  IV(NAME_selectedItem, "menu_item|popup*", IV_GET,
     NAME_selection, "Selected menu-item/sub-popup"),
  IV(NAME_button, "button_name", IV_GET,
     NAME_event, "Name of invoking button"),
  IV(NAME_defaultItem, "{first,selection}|any*", IV_BOTH,
     NAME_appearance, "Initial previewed item"),
  SV(NAME_showCurrent, "bool", IV_GET|IV_STORE, showCurrentPopup,
     NAME_appearance, "If @on, show the currently selected value")
};

/* Send Methods */

static const senddecl send_popup[] =
{ SM(NAME_event, 1, "event", eventPopup,
     DEFAULT, "Handle an event"),
  SM(NAME_initialise, 2, T_initialise, initialisePopup,
     DEFAULT, "Create from name and message"),
  SM(NAME_key, 1, "key=name", keyPopup,
     NAME_accelerator, "Set <-selected_item according to accelerator"),
  SM(NAME_update, 1, "context=any", updatePopup,
     NAME_active, "Update entries using context object)"),
  SM(NAME_endGroup, 1, "bool", endGroupPopup,
     NAME_appearance, "Pullright: separation line below item in super"),
  SM(NAME_drag, 2, T_drag, dragPopup,
     NAME_event, "Handle a drag event"),
  SM(NAME_showPullrightMenu, 3, T_showPullrightMenu, showPullrightMenuPopup,
     NAME_event, "Show pullright for this item"),
  SM(NAME_execute, 1, "context=[object]*", executePopup,
     NAME_execute, "Execute selected message of item"),
  SM(NAME_close, 0, NULL, closePopup,
     NAME_open, "Finish after ->open"),
  SM(NAME_open, 5, T_open, openPopup,
     NAME_open, "Open on point relative to graphical"),
  SM(NAME_reset, 0, NULL, resetPopup,
     NAME_reset, "Close popup after an abort")
};

/* Get Methods */

static const getdecl get_popup[] =
{ 
};

/* Resources */

static const resourcedecl rc_popup[] =
{ RC(NAME_acceleratorFont, "font*", "small",
     "Show the accelerators"),
  RC(NAME_border, "int", "2",
     "Default border around items"),
  RC(NAME_cursor, "cursor", "right_ptr",
     "Cursor when popup is active"),
  RC(NAME_defaultItem, "name*", "first",
     "Item to select as default"),
  RC(NAME_feedback, "name", "image",
     "Feedback style"),
  RC(NAME_kind, "name", "popup",
     "Menu kind"),
  RC(NAME_labelSuffix, "name", "",
     "Ensured suffix of label"),
  RC(NAME_layout, "name", "vertical",
     "Put items below each other"),
  RC(NAME_multipleSelection, "bool", "@off",
     "Can have multiple selection"),
  RC(NAME_offImage, "image*", "@nil",
     "Marker for items not in selection"),
  RC(NAME_onImage, "image*", "@nil",
     "Marker for items in selection"),
  RC(NAME_pen, "int", "0",
     "Thickness of the drawing-pen"),
  RC(NAME_popupImage, "image*", "@pull_right_image",
     "Marker for items with popup"),
  RC(NAME_previewFeedback, "name", "invert",
     "Feedback on `preview' item"),
  RC(NAME_showLabel, "bool", "@off",
     "Label is visible"),
  RC(NAME_valueWidth, "int", "80",
     "Minimum width in pixels")
};

/* Class Declaration */

static Name popup_termnames[] = { NAME_name, NAME_message };

ClassDecl(popup_decls,
          var_popup, send_popup, get_popup, rc_popup,
          2, popup_termnames,
          "$Rev$");

status
makeClassPopup(Class class)
{ return declareClass(class, &popup_decls);
}

