/*  $Id$

    Part of XPCE

    Author:  Jan Wielemaker and Anjo Anjewierden
    E-mail:  jan@swi.psy.uva.nl
    WWW:     http://www.swi.psy.uva.nl/projects/xpce/
    Copying: GPL-2.  See the file COPYING or http://www.gnu.org

    Copyright (C) 1990-2001 SWI, University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>
#include <h/graphics.h>
#include <h/dialog.h>

static status
initialiseDialog(Dialog d, Name name, Size size, DisplayObj display)
{ TileObj t;

  initialiseWindow((PceWindow) d, name, size, display);

  assign(d, gap, newObject(ClassSize, 0));
  copySize(d->gap, getClassVariableValueObject(d, NAME_gap));
  assign(d, size_given, NAME_none);

  t = getTileWindow((PceWindow) d);
  assign(t, horShrink,  ZERO);
  assign(t, verShrink,  ZERO);
  assign(t, horStretch, ZERO);
  assign(t, verStretch, ZERO);

  succeed;
}


static status
displayDialog(Dialog d, Graphical item, Point pos)
{ if ( displayDevice(d, item, pos) )
  { if ( instanceOfObject(item, ClassDialogItem) )
      d->graphicals->current = d->graphicals->tail;
    if ( isNil(d->keyboard_focus) && send(item, NAME_WantsKeyboardFocus, 0) )
      keyboardFocusWindow((PceWindow) d, item);

    succeed;
  }
  
  fail;
}


		/********************************
		*            TYPING		*
		********************************/

static status
typedDialog(Dialog d, EventId id, Bool delegate)
{ Name key = characterName(id);
  Graphical gr;

  for_chain(d->graphicals, gr,
	    if ( send(gr, NAME_key, key, 0) )
	      succeed);

  if ( delegate == ON && notNil(d->frame) )
    return send(d->frame, NAME_typed, id, 0);

  fail;
}


static status
appendDialog(Dialog d, Graphical item, Name where)
{ return appendDialogItemDevice((Device) d, item, where);
}


static status
layoutDialog(Dialog d, Size size)
{ return layoutDialogDevice((Device) d, d->gap, size, DEFAULT);
}


static status
ComputeDesiredSizeDialog(Dialog d)
{ Name given;

  TRY(send(d, NAME_layout, 0));

  if ( isNil(d->keyboard_focus) )
    send(d, NAME_advance, NIL, 0);	/* select first text item */

  ComputeGraphical(d);

  if ( (given=d->size_given) != NAME_both )
  { Int w, h;
    int empty;

    if ( (empty=emptyChain(d->graphicals)) )
    { Size sz = getClassVariableValueObject(d, NAME_size);

      w = sz->w;
      h = sz->h;
    } else
    { Area a = d->bounding_box;

      w = toInt(valInt(a->x) + valInt(a->w) + valInt(d->gap->w));
      h = toInt(valInt(a->y) + valInt(a->h) + valInt(d->gap->h));
    }

    if ( given == NAME_width )
      w = DEFAULT;
    else if ( given == NAME_height )
      h = DEFAULT;

    send(d, NAME_set, DEFAULT, DEFAULT, w, h, 0);
  }

  succeed;
}


#define WIDTH_GIVEN  1
#define HEIGTH_GIVEN 2

static status
setDialog(Dialog d, Int x, Int y, Int w, Int h)
{ static Name given_names[] = { NAME_none,
  				NAME_width,
				NAME_height,
				NAME_both
			      };
  int given;
  
  for(given = 0; given < 4; given++)
  { if ( given_names[given] == d->size_given )
      break;
  }
  if ( given == 4 )			/* just in case */
    given = 0;

  if ( notDefault(w) )
    given |= WIDTH_GIVEN;
  if ( notDefault(h) )
    given |= HEIGTH_GIVEN;

  assign(d, size_given, given_names[given]);

  return setGraphical((Graphical) d, x, y, w, h);
}


static status
widthDialog(Dialog d, Int w)
{ return setDialog(d, DEFAULT, DEFAULT, w, DEFAULT);
}


static status
heightDialog(Dialog d, Int h)
{ return setDialog(d, DEFAULT, DEFAULT, DEFAULT, h);
}


static status
sizeDialog(Dialog d, Size size)
{ return setDialog(d, DEFAULT, DEFAULT, size->w, size->h);
}



		/********************************
		*         MISCELLANEAUS		*
		********************************/

static Graphical
getMemberDialog(Dialog d, Any obj)
{ if ( isName(obj) )
    return getMemberDevice((Device) d, (Name) obj);
  
  if ( ((Graphical)obj)->device == (Device) d )
    answer(obj);

  fail;
}


static status
memberDialog(Dialog d, Any obj)
{ return getMemberDialog(d, obj) != FAIL ? SUCCEED : FAIL;
}


static Chain
getMembersDialog(Dialog d)
{ answer(d->graphicals);
}


static status
deleteDialog(Dialog d, Graphical gr)
{ return freeObject(gr);
}


static status
caretDialog(Dialog d, Graphical gr)
{ return keyboardFocusWindow((PceWindow) d, gr);
}


static status
activeDialog(Dialog d, Bool val)
{ assign(d, sensitive, val);

  succeed;
}


static Bool
getActiveDialog(Dialog d)
{ answer(d->sensitive);
}


		/********************************
		*         COMMUNICATION		*
		********************************/

static status
defaultButtonDialog(Dialog d, Button b)
{ Cell cell;

  for_cell(cell, d->graphicals)
  { Button b2 = cell->value;

    if ( instanceOfObject(b2, ClassButton) )
      assign(b2, default_button, b == b2 ? ON : OFF);
  }

  succeed;
}


static Button
getDefaultButtonDialog(Dialog d)
{ Cell cell;

  for_cell(cell, d->graphicals)
  { Button b = cell->value;

    if ( instanceOfObject(b, ClassButton) &&
	 b->default_button == ON )
      answer(b);
  }

  fail;
}


static status
applyDialog(Dialog d, Bool always)
{ DialogItem di;
  Graphical defb;
  
  for_chain(d->graphicals, di, send(di, NAME_apply, always, 0));
  if ( (defb = get(d, NAME_defaultButton, 0)) )
    send(defb, NAME_active, OFF, 0);

  succeed;
}


static status
restoreDialog(Dialog d)
{ DialogItem di;
  Graphical defb;
  
  for_chain(d->graphicals, di, send(di, NAME_restore, 0));
  if ( (defb = get(d, NAME_defaultButton, 0)) )
    send(defb, NAME_active, OFF, 0);

  succeed;
}


static status
modifiedItemDialog(Dialog d, Graphical gr, Bool m)
{ Button b;

  if ( (b = getDefaultButtonDialog(d)) )
  { send(b, NAME_active, ON, 0);
    if ( send(b, NAME_isApply, 0) )
      succeed;
  }
    
  fail;
}

		/********************************
		*             REPORT		*
		********************************/

static Any
getReportToDialog(Dialog d)
{ Any reporter;

  if ( (reporter = get(d, NAME_member, NAME_reporter, 0)) )
    answer(reporter);

  answer(getReportToVisual((VisualObj) d));
}


		 /*******************************
		 *	   ACCELERATORS		*
		 *******************************/

static status
assignAcceletatorsDialog(Dialog d)
{ return assignAccelerators(d->graphicals, CtoName("\\e"), NAME_label);
}


		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */

static char *T_typed[] =
        { "event_id", "[bool]" };
static char *T_display[] =
        { "graphical", "at=[point]" };
static char *T_modifiedItem[] =
        { "item=graphical", "modified=bool" };
static char *T_append[] =
        { "item=graphical", "relative_to_last=[{below,right,next_row}]" };
static char *T_initialise[] =
        { "label=[name]", "size=[size]", "display=[display]" };

/* Instance Variables */

static vardecl var_dialog[] =
{ IV(NAME_gap, "size", IV_BOTH,
     NAME_layout, "Distance in X and Y direction between items"),
  IV(NAME_sizeGiven, "{none,width,height,both}", IV_NONE,
     NAME_layout, "User specified explicit width/height")
};

/* Send Methods */

static senddecl send_dialog[] =
{ SM(NAME_initialise, 3, T_initialise, initialiseDialog,
     DEFAULT, "Create from label, size and display"),
  SM(NAME_defaultButton, 1, "member:button*", defaultButtonDialog,
     NAME_accelerator, "Button connected to `RET'"),
  SM(NAME_assignAccelerators, 0, NULL, assignAcceletatorsDialog,
     NAME_accelerator, "Assign accelerators for the dialog_item objects"),
  SM(NAME_typed, 2, T_typed, typedDialog,
     NAME_accelerator, "Handle accelerators"),
  SM(NAME_active, 1, "bool", activeDialog,
     NAME_active, "(DE)activate the entire window"),
  SM(NAME_apply, 1, "always=[bool]", applyDialog,
     NAME_apply, "->apply all changed items"),
  SM(NAME_modifiedItem, 2, T_modifiedItem, modifiedItemDialog,
     NAME_apply, "Indicates item has changed state"),
  SM(NAME_restore, 0, NULL, restoreDialog,
     NAME_apply, "->restore all items to their <-default"),
  SM(NAME_size, 1, "size", sizeDialog,
     NAME_area, "Give dialog an explicit size"),
  SM(NAME_width, 1, "0..", widthDialog,
     NAME_area, "Give dialog an explicit width"),
  SM(NAME_height, 1, "0..", heightDialog,
     NAME_area, "Give dialog an explicit height"),
  SM(NAME_caret, 1, "member:graphical", caretDialog,
     NAME_focus, "Assign the caret to an input object"),
  SM(NAME_ComputeDesiredSize, 0, NULL, ComputeDesiredSizeDialog,
     NAME_layout, "Compute the desired size"),
  SM(NAME_layout, 1, "size=[size]", layoutDialog,
     NAME_layout, "(Re)compute layout of dialog_items"),
  SM(NAME_append, 2, T_append, appendDialog,
     NAME_organisation, "Append dialog_item {below,right,next_row} last"),
  SM(NAME_delete, 1, "member:graphical", deleteDialog,
     NAME_organisation, "Delete (named) dialog item"),
  SM(NAME_display, 2, T_display, displayDialog,
     NAME_organisation, "Display a graphical (or item) at point"),
  SM(NAME_member, 1, "name|dialog_item", memberDialog,
     NAME_organisation, "Test if dialog_item or name is a member")
};

/* Get Methods */

static getdecl get_dialog[] =
{ GM(NAME_defaultButton, 0, "button", NULL, getDefaultButtonDialog,
     NAME_accelerator, "Current Button connected to `RET'"),
  GM(NAME_active, 0, "bool", NULL, getActiveDialog,
     NAME_active, "Equivalent to Window <-sensitive"),
  GM(NAME_member, 1, "graphical", "name|graphical", getMemberDialog,
     NAME_organisation, "Find named dialog_item"),
  GM(NAME_members, 0, "chain", NULL, getMembersDialog,
     NAME_organisation, "Equivalent to <-graphicals"),
  GM(NAME_reportTo, 0, "graphical|frame", NULL, getReportToDialog,
     NAME_report, "<-member: reporter or <-contained_in")
};

/* Resources */

static classvardecl rc_dialog[] =
{ RC(NAME_gap, "size", "size(15,8)",
     "Distance between items in X and Y"),
  RC(NAME_background, RC_REFINE, "@_dialog_bg", NULL)
};

/* Class Declaration */

static Name dialog_termnames[] =
	{ NAME_label, NAME_displaySize, NAME_display };

ClassDecl(dialog_decls,
          var_dialog, send_dialog, get_dialog, rc_dialog,
          1, dialog_termnames,
          "$Rev$");

status
makeClassDialog(Class class)
{ return declareClass(class, &dialog_decls);
}

