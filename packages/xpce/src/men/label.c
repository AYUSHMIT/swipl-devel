/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1992 University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>
#include <h/dialog.h>

static status	statusLabel(Label lb, Name stat);
static status	selectionLabel(Label lb, Any selection);

static status
initialiseLabel(Label lb, Name name, Any selection, FontObj font)
{ if ( isDefault(name) )
    name = NAME_reporter;
  if ( isDefault(selection) )	
    selection = CtoName("");

  createDialogItem(lb, name);

  assign(lb, font,      font);
  assign(lb, length,    DEFAULT);
  assign(lb, border,    DEFAULT);
  selectionLabel(lb, selection);
  
  return requestComputeGraphical(lb, DEFAULT);
}


static status
RedrawAreaLabel(Label lb, Area a)
{ int x, y, w, h;
  int z = valInt(getResourceValueObject(lb, NAME_elevation));
  int preview = (lb->status == NAME_preview && notNil(lb->message));

  initialiseDeviceGraphical(lb, &x, &y, &w, &h);

  if ( z )
  { if ( preview )
      z = -z;
    r_3d_box(x, y, w, h, abs(z), lb->background, z > 0);
  }

  x += valInt(lb->border);
  y += valInt(lb->border);

  if ( instanceOfObject(lb->selection, ClassCharArray) )
  { CharArray s = lb->selection;
    int ex = valInt(getExFont(lb->font));

    str_string(&s->data, lb->font, x+ex/2, y, w, h, NAME_left, NAME_top);
  } else /*if ( instanceOfObject(lb->selection, ClassImage) )*/
  { Image image = (Image) lb->selection;

    r_image(image, 0, 0, x, y, w, h, ON);
  }

  if ( preview && !z )
    r_complement(x, y, w, h);    

  return RedrawAreaGraphical(lb, a);
}


static status
eventLabel(Label lb, EventObj ev)
{ if ( eventDialogItem(lb, ev) )
    succeed;

  if ( notNil(lb->message) && lb->active == ON )
  { makeButtonGesture();

    return eventGesture(GESTURE_button, ev);
  }

  fail;
}


static status
executeLabel(Label lb)
{ if ( notNil(lb->message) && notDefault(lb->message) )
  { statusLabel(lb, NAME_execute);
    flushGraphical(lb);
    forwardReceiverCode(lb->message, lb, 0);
    if ( !isFreedObj(lb) )
    { statusLabel(lb, NAME_inactive);
      flushGraphical(lb);
    }
  }

  succeed;
}



static status
statusLabel(Label lb, Name stat)
{ if ( stat != lb->status )
  { Name oldstat = lb->status;

    assign(lb, status, stat);

    if ( oldstat == NAME_preview || stat == NAME_preview )
      changedDialogItem(lb);
  }

  succeed;
}


static status
computeLabel(Label lb)
{ if ( notNil(lb->request_compute) )
  { int w, h;

    TRY(obtainResourcesObject(lb));

    if ( instanceOfObject(lb->selection, ClassCharArray) )
    { CharArray s = (CharArray) lb->selection;
      int ex = valInt(getExFont(lb->font));

      str_size(&s->data, lb->font, &w, &h);
      w = max(w+ex, valInt(lb->length) * ex);
    } else /*if ( instanceOfObject(lb->selection, ClassImage) )*/
    { Image image = (Image) lb->selection;

      w = valInt(image->size->w);
      h = valInt(image->size->h);
    }

    w += 2*valInt(lb->border);
    h += 2*valInt(lb->border);

    CHANGING_GRAPHICAL(lb,
	assign(lb->area, w, toInt(w));
	assign(lb->area, h, toInt(h));
	changedEntireImageGraphical(lb));

    assign(lb, request_compute, NIL);
  }

  succeed;
}


static Point
getReferenceLabel(Label lb)
{ Point ref;

  if ( !(ref = getReferenceDialogItem(lb)) )
  { if ( instanceOfObject(lb->selection, ClassCharArray) )
      ref = answerObject(ClassPoint,
			 ZERO, getAscentFont(lb->font), 0);
    else
      ref = answerObject(ClassPoint,
			 ZERO, ((Image) lb->selection)->size->h, 0);
  }

  answer(ref);
}



		/********************************
		*          ATTRIBUTES		*
		********************************/

static status
selectionLabel(Label lb, Any selection)
{ if ( lb->selection != selection )
  { assign(lb, selection, selection);
    requestComputeGraphical(lb, DEFAULT);
  }

  succeed;
}


static status
clearLabel(Label lb)
{ return selectionLabel(lb, CtoName(""));
}


static status
formatLabel(Label lb, CharArray fm, int argc, Any *argv)
{ ArgVector(av, argc+1);
  int ac;
  StringObj str;
  
  av[0] = fm;
  for(ac=1; ac <= argc; ac++)
    av[ac] = argv[ac-1];

  TRY(str = newObjectv(ClassString, ac, av));
  return selectionLabel(lb, str);
}


static status
fontLabel(Label lb, FontObj font)
{ if ( lb->font != font )
  { assign(lb, font, font);
    requestComputeGraphical(lb, DEFAULT);
  }

  succeed;
}


static status
lengthLabel(Label lb, Int length)
{ if ( lb->length != length )
  { assign(lb, length, length);
    requestComputeGraphical(lb, DEFAULT);
  }

  succeed;
}


static status
borderLabel(Label lb, Int border)
{ return assignGraphical(lb, NAME_border, border);
}


static status
catchAllLabelv(Label lb, Name selector, int argc, Any *argv)
{ if ( hasSendMethodObject(lb->selection, selector) )
  { assign(PCE, last_error, NIL);

    if ( sendv(lb->selection, selector, argc, argv) )
      return requestComputeGraphical(lb, DEFAULT);
  } else if ( instanceOfObject(lb->selection, ClassCharArray) &&
	      getSendMethodClass(ClassString, selector) )
  { assign(lb, selection, newObject(ClassString,
				    name_procent_s, lb->selection, 0));
    assign(PCE, last_error, NIL);
    if ( sendv(lb->selection, selector, argc, argv) )
      return requestComputeGraphical(lb, DEFAULT);
  }

  fail;
}


static status
reportLabel(Label lb, Name kind, CharArray fmt, int argc, Any *argv)
{ if ( isDefault(fmt) )
    fmt = (CharArray) (kind == NAME_done ? NAME_done : CtoName(""));

  if ( kind == NAME_done )
  { if ( instanceOfObject(lb->selection, ClassCharArray) )
    { CharArray t1 = getEnsureSuffixCharArray(lb->selection,
					      (CharArray) CtoName(" "));
      StringObj str;
      ArgVector(av, argc+1);
      int ac;
  
      av[0] = fmt;
      for(ac=1; ac <= argc; ac++)
	av[ac] = argv[ac-1];

      TRY(str = newObjectv(ClassString, ac, av));
      t1 = getAppendCharArray(t1, (CharArray) str);
      doneObject(str);

      selectionLabel(lb, t1);
      doneObject(t1);
    } else
      TRY(formatLabel(lb, fmt, argc, argv));

    flushGraphical(lb);
  } else
  { TRY(formatLabel(lb, fmt, argc, argv));

    if ( kind == NAME_error || kind == NAME_warning )
    { send(lb, NAME_flash, 0); 
      alertReporteeVisual((VisualObj) lb);
    } else if ( kind == NAME_progress )
      flushGraphical(lb);
  }

  succeed;
}




status
makeClassLabel(Class class)
{ sourceClass(class, makeClassLabel, __FILE__, "$Revision$");

  localClass(class, NAME_font, NAME_appearance, "font", NAME_get,
	     "Font for selection");
  localClass(class, NAME_length, NAME_area, "int", NAME_get,
	     "Length in characters (with text)");
  localClass(class, NAME_selection, NAME_selection, "char_array|image",
	     NAME_get,
	     "Text or image displayed");
  localClass(class, NAME_border, NAME_appearance, "0..", NAME_get,
	     "Space around the image");

  termClass(class, "label", 3, NAME_name, NAME_selection, NAME_font);
  setRedrawFunctionClass(class, RedrawAreaLabel);

  storeMethod(class, NAME_status,    statusLabel);
  storeMethod(class, NAME_font,      fontLabel);
  storeMethod(class, NAME_length,    lengthLabel);
  storeMethod(class, NAME_selection, selectionLabel);
  storeMethod(class, NAME_border,    borderLabel);

  sendMethod(class, NAME_initialise, DEFAULT,
	     3, "name=[name]", "selection=[string|image]", "font=[font]",
	     "Create from name, selection and font",
	     initialiseLabel);
  sendMethod(class, NAME_catchAll, NAME_delegate, 2, "name", "unchecked ...",
	     "Delegate to <->selection",
	     catchAllLabelv);
  sendMethod(class, NAME_compute, DEFAULT, 0,
	     "Recompute layout",
	     computeLabel);
  sendMethod(class, NAME_event, DEFAULT, 1, "event",
	     "Act as button if <-message not @nil",
	     eventLabel);
  sendMethod(class, NAME_format, NAME_format, 2, "name", "any ...",
	     "Create string from format and make it the selection",
	     formatLabel);
  sendMethod(class, NAME_execute, NAME_execute, 0,
	     "Execute associated message",
	     executeLabel);
  sendMethod(class, NAME_clear, NAME_selection, 0,
	     "Equivalent to ->selection: ''",
	     clearLabel);
  sendMethod(class, NAME_report, NAME_report, 3,
	     "kind={status,inform,progress,done,warning,error}",
	     "format=[char_array]", "argument=any ...",
	     "Report message",
	     reportLabel);

  getMethod(class, NAME_reference, DEFAULT, "point", 0,
	    "Baseline or bottom (image)",
	    getReferenceLabel);

  attach_resource(class, "font", "font", "@helvetica_roman_14",
		  "Default font for selection");
  attach_resource(class, "length", "int", "25",
		  "Default length in characters");
  attach_resource(class, "length", "int", "25",
		  "Default length in characters");
  attach_resource(class, "border", "0..", "0",
		  "Space around image/string");

  succeed;
}

