/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1994 University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>
#include <h/graphics.h>
#include "include.h"
#include "fshell.h"

#if O_MOTIF
#define XMSTRINGDEFINES
#include <Xm/Xm.h>
#endif

static void	xEventFrame(Widget, FrameObj, XEvent *);
static void	destroyFrame(Widget, FrameObj, XtPointer);
static status   updateAreaFrame(FrameObj fr, Int border);

#define MainWindow(fr)	     ( isNil(fr->members->head) ? (Any) fr : \
			       fr->members->head->value )


		 /*******************************
		 *	    REFERENCES		*
		 *******************************/

static void
setWidgetFrame(FrameObj fr, Widget w)
{ if ( !fr->ws_ref )
  { fr->ws_ref = alloc(sizeof(frame_ws_ref));
    ((frame_ws_ref *)fr->ws_ref)->busy_window = 0;
  }

  ((frame_ws_ref *)fr->ws_ref)->widget = w;
}


static void
setBusyWindowFrame(FrameObj fr, Window w)
{ if ( !fr->ws_ref )
  { fr->ws_ref = alloc(sizeof(frame_ws_ref));
    ((frame_ws_ref *)fr->ws_ref)->widget = NULL;
  }

  ((frame_ws_ref *)fr->ws_ref)->busy_window = w;
}


static Window
busyWindowFrame(FrameObj fr)
{ return fr->ws_ref ? ((frame_ws_ref *)fr->ws_ref)->busy_window : 0;
}

		 /*******************************
		 *	     (UN)CREATE		*
		 *******************************/

status
ws_created_frame(FrameObj fr)
{ if ( widgetFrame(fr) )
    succeed;
  
  fail;
}


void
ws_uncreate_frame(FrameObj fr)
{ Widget w;

  if ( (w = widgetFrame(fr)) )
  { assign(fr, status, NAME_unmapped);
    setWidgetFrame(fr, NULL);

    XtRemoveCallback(w, XtNdestroyCallback,
		     (XtCallbackProc)destroyFrame, fr);
    XtRemoveCallback(w, XtNeventCallback,
		     (XtCallbackProc)xEventFrame, fr);

    XtDestroyWidget(w);

    if ( fr->ws_ref )
    { unalloc(sizeof(frame_ws_ref), fr->ws_ref);
      fr->ws_ref = NULL;
    }
  }
}


status
ws_create_frame(FrameObj fr)
{ Arg args[25];
  Cardinal n = 0;
  Widget w;
  DisplayObj d = fr->display;
  DisplayWsXref r = d->ws_ref;

  XtSetArg(args[n], XtNtitle,		  strName(fr->label));    n++;
  XtSetArg(args[n], XtNmappedWhenManaged, False);                 n++;
  XtSetArg(args[n], XtNwidth,      	  valInt(fr->area->w));   n++;
  XtSetArg(args[n], XtNheight,      	  valInt(fr->area->h));   n++;
  XtSetArg(args[n], XtNinput,		  True);		  n++;
  if ( instanceOfObject(fr->background, ClassColour) )
  { XtSetArg(args[n], XtNbackground, getPixelColour(fr->background, d));
    n++;
  } else
  { Pixmap pm = (Pixmap) getXrefObject(fr->background, d);

    XtSetArg(args[n], XtNbackgroundPixmap, pm); n++;
  }		

  if ( notNil(fr->icon_label) )
  { XtSetArg(args[n], XtNiconName, strName(getIconLabelFrame(fr)));
    n++;
  }
  if ( fr->kind == NAME_popup )
  { XtSetArg(args[n], XtNsaveUnder, True);
    n++;
  }
  if ( notNil(fr->icon_image) )
  { XtSetArg(args[n], XtNiconPixmap,
	     getXrefObject(fr->icon_image, fr->display));
    n++;
  }
  if ( notNil(fr->icon_position) )
  { XtSetArg(args[n], XtNiconX, valInt(fr->icon_position->x)); n++;
    XtSetArg(args[n], XtNiconY, valInt(fr->icon_position->y)); n++;
  }
#if O_MOTIF
  XtSetArg(args[n], XmNdeleteResponse, XmDO_NOTHING); n++;
#endif

  w = XtCreatePopupShell(
		    strName(fr->label),
		    fr->kind == NAME_popup     ? overrideFrameWidgetClass  :
		    fr->kind == NAME_transient ? transientFrameWidgetClass :
					         topLevelFrameWidgetClass,
		    r->shell_xref,
		    args, n);

  if ( !w )
    return errorPce(fr, NAME_xOpen, fr->display);

  XtAddCallback(w, XtNeventCallback,
		(XtCallbackProc) xEventFrame, (caddr_t) fr);
  XtAddCallback(w, XtNdestroyCallback,
		(XtCallbackProc) destroyFrame, (caddr_t) fr);

  setWidgetFrame(fr, w);

  succeed;
}


void
ws_realise_frame(FrameObj fr)
{ LocalArray(Widget, children, valInt(getSizeChain(fr->members)));
  int n = 0;
  Widget w = widgetFrame(fr);
  Cell cell;
  DisplayWsXref r = fr->display->ws_ref;

  for_cell(cell, fr->members)
    children[n++] = widgetWindow(cell->value);

  XtManageChildren(children, n);
  XtRealizeWidget(w);

  for_cell(cell, fr->members)
    send(cell->value, NAME_geometry, 0); /* see note at ws_create_window */

  if ( notNil(fr->transient_for) )
  { XSetTransientForHint(r->display_xref,
			 XtWindow(w),
			 XtWindow(widgetFrame(fr->transient_for)));
  }
#if O_MOTIF
  else
  { XDeleteProperty(r->display_xref,
		    XtWindow(w),
		    XA_WM_TRANSIENT_FOR);
  }
#endif

  ws_frame_background(fr, fr->background); /* Why is this necessary? */
}


		 /*******************************
		 *   FIND WINDOW HOLDING POINT	*
		 *******************************/

PceWindow
ws_window_holding_point_frame(FrameObj fr)
{ Cell cell;

  for_cell(cell, fr->members)
  { PceWindow sw = cell->value;

    if ( instanceOfObject(sw, ClassWindowDecorator) )
    { WindowDecorator dw = (WindowDecorator) sw;
      sw = dw->window;
    }

    if ( sw->has_pointer == ON )
      answer(sw);
  }

  fail;
}

		 /*******************************
		 *	    HIDE/EXPOSE		*
		 *******************************/

void
ws_raise_frame(FrameObj fr)
{ Widget w = widgetFrame(fr);
  DisplayWsXref r = fr->display->ws_ref;

  if ( w )
  { XMapWindow(r->display_xref, XtWindow(w));
    XRaiseWindow(r->display_xref, XtWindow(w));
  }

  send(fr, NAME_exposed, 0);		/* doesn't appear to generate a */
					/* CirculateNotify event */
}


void
ws_lower_frame(FrameObj fr)
{ Widget w = widgetFrame(fr);
  DisplayWsXref r = fr->display->ws_ref;
  
  if ( w )
    XLowerWindow(r->display_xref, XtWindow(w));
}


		 /*******************************
		 *	   WM-PROTOCOL		*
		 *******************************/

status
ws_attach_wm_prototols_frame(FrameObj fr)
{ Atom *pr = (Atom *)alloca(valInt(getSizeChain(fr->wm_protocols->attributes))
			    * sizeof(Atom));
  Cell cell;
  int n = 0;
  DisplayWsXref r = fr->display->ws_ref;

  for_cell(cell, fr->wm_protocols->attributes)
  { Attribute a = cell->value;
    Name name = checkType(a->name, TypeName, fr);

    if ( name != FAIL )
      pr[n++] = FrameAtom(fr, name);
  }

  DEBUG(NAME_frame, Cprintf("Attaching WM_PROTOCOLS\n"));

  XSetWMProtocols(r->display_xref, XtWindow(widgetFrame(fr)), pr, n);

  assign(fr, wm_protocols_attached, ON);

  succeed;
}


		 /*******************************
		 *     XT-CALLBACK HANDLING	*
		 *******************************/

static int
service_frame(FrameObj fr)
{ Application app = fr->application;

  DEBUG(NAME_service, Cprintf("Event on %s, app %s, kind %s\n",
			      pp(fr), pp(app),
			      notNil(app) ? pp(app->kind) : "-"));

  return (notNil(app) && app->kind == NAME_service ? PCE_EXEC_SERVICE
						   : PCE_EXEC_USER);
}


static void
destroyFrame(Widget w, FrameObj fr, XtPointer data)
{ if ( fr->ws_ref )
  { unalloc(sizeof(frame_ws_ref), fr->ws_ref);
    fr->ws_ref = NULL;
  }

  ServiceMode(service_frame(fr),
	      freeObject(fr));
}


static void
x_event_frame(Widget w, FrameObj fr, XEvent *event)
{ switch( event->xany.type )
  { case ClientMessage:
    { DEBUG(NAME_frame, Cprintf("Received client message\n"));
					/* Window Manager Request */
      if ( event->xclient.message_type == WmProtocols(fr) )
      { Name name;
	Code msg;

	DEBUG(NAME_frame,
	      Cprintf("Protocol message %s\n",
		      FrameAtomToString(fr, event->xclient.data.l[0])));

	name = CtoName(FrameAtomToString(fr, event->xclient.data.l[0]));
	if ( (msg = checkType(getValueSheet(fr->wm_protocols, name),
			      TypeCode, fr)) )
	  forwardReceiverCode(msg, fr, MainWindow(fr), 0);
      }
      return;
    }
    case ConfigureNotify:
    { updateAreaFrame(fr, toInt(event->xconfigure.border_width));
      return;
    }
    case PropertyNotify:
      if ( fr->wm_protocols_attached == OFF &&
	   event->xproperty.atom == WmProtocols(fr) )
      { if ( fr->kind != NAME_popup )
	  ws_attach_wm_prototols_frame(fr);
      }
      return;
    case MapNotify:
    { Cell cell;

      for_cell(cell, fr->members)
	send(cell->value, NAME_displayed, ON, 0);
      updateAreaFrame(fr, DEFAULT);
      send(fr, NAME_mapped, ON, 0);
      assign(fr, status, NAME_window);
      return;
    }
    case UnmapNotify:
    { Cell cell;

      for_cell(cell, fr->members)
	send(cell->value, NAME_displayed, OFF, 0);
      if ( !isFreedObj(fr) || isFreeingObj(fr) )
	send(fr, NAME_mapped, OFF, 0);
      assign(fr, status, NAME_hidden);
      return;
    }
    case CirculateNotify:
      if ( event->xcirculate.place == PlaceOnTop )
      	send(fr, NAME_exposed, 0);
      else
	send(fr, NAME_hidden, 0);
      return;
    case FocusIn:
      send(fr, NAME_inputFocus, ON, 0);
      return;
    case FocusOut:
      send(fr, NAME_inputFocus, OFF, 0);
      return;
    case KeyPress:
    { EventObj ev;
      PceWindow sw = getKeyboardFocusFrame(fr);

      if ( sw && (ev = CtoEvent(sw, event)) )
      { addCodeReference(ev);
	postEvent(ev, (Graphical) sw, DEFAULT);
	delCodeReference(ev);
	freeableObj(ev);
      }

      return;
    }
    default:
    { EventObj ev;
      AnswerMark mark;
      markAnswerStack(mark);
  
      if ( (ev = CtoEvent(fr, event)) )
      { addCodeReference(ev);
	send(fr, NAME_event, ev, 0);
	delCodeReference(ev);
	freeableObj(ev);
      }

      rewindAnswerStack(mark, NIL);
      
      return;
    }
  }
}


static void
xEventFrame(Widget w, FrameObj fr, XEvent *event)
{ ServiceMode(service_frame(fr),
	      x_event_frame(w, fr, event));
}


		 /*******************************
		 *	      CURSOR		*
		 *******************************/

void
ws_frame_cursor(FrameObj fr, CursorObj cursor)
{ Widget w = widgetFrame(fr);

  if ( w )
  { DisplayWsXref r = fr->display->ws_ref;
    Display *d = r->display_xref;

    XDefineCursor(d,
		  XtWindow(w),
		  !instanceOfObject(cursor, ClassCursor)
		      ? None
		      : (Cursor) getXrefObject(cursor, fr->display));
  }
}


void
ws_grab_frame_pointer(FrameObj fr, Bool grab, CursorObj cursor)
{ Widget w = widgetFrame(fr);

  if ( w )
  { if ( grab == ON )
    { Cursor c = (!instanceOfObject(cursor, ClassCursor)
		      ? None
		      : (Cursor) getXrefObject(cursor, fr->display));

      XtGrabPointer(w,
		    False,
		    ButtonPressMask|ButtonReleaseMask|
		    EnterWindowMask|LeaveWindowMask|
		    PointerMotionMask|ButtonMotionMask,
		    GrabModeAsync, GrabModeAsync,
		    None,
		    c,
		    CurrentTime);
    } else
    { XtUngrabPointer(w, CurrentTime);
    }
  }
}


		 /*******************************
		 *     GEOMETRY MANAGEMENT	*
		 *******************************/

static status
updateAreaFrame(FrameObj fr, Int border)
{ Widget wdg;

  if ( (wdg = widgetFrame(fr)) )
  { DisplayWsXref r = fr->display->ws_ref;
    Display *d = r->display_xref;
    Window me, root, child;
    int x, y;
    unsigned int w, h, bw, depth;
    
    if ( (me = XtWindow(wdg)) )
    { Area a = fr->area;
      Int ow = a->w, oh = a->h;
    
      XGetGeometry(d, me, &root, &x, &y, &w, &h, &bw, &depth);
      XTranslateCoordinates(d, me, root, 0, 0, &x, &y, &child);

      assign(a, x, toInt(x));
      assign(a, y, toInt(y));
      assign(a, w, toInt(w));
      assign(a, h, toInt(h));
      if ( notDefault(border) )
	assign(fr, border, border);

      if ( a->w != ow || a->h != oh )
	send(fr, NAME_resize, 0);
    }

    succeed;
  }

  fail;
}


status
ws_frame_bb(FrameObj fr, int *x, int *y, int *w, int *h)
{ Window win;

  if ( (win = getWMFrameFrame(fr)) )
  { DisplayWsXref r = fr->display->ws_ref;
    XWindowAttributes atts;
    int bw = isDefault(fr->border) ? 1 : valInt(fr->border);

    XGetWindowAttributes(r->display_xref, win, &atts);
    *x = atts.x - bw;
    *y = atts.y - bw;
    *w = atts.width + 2*bw;
    *h = atts.height + 2*bw;
    
    succeed;
  }

  fail;
}


void
ws_x_geometry_frame(FrameObj fr, Name spec)
{ Widget wdg = widgetFrame(fr);
  
  if ( wdg )
  { int x, y, w, h, mask;
    Int X, Y, W, H;
    char def[50];
    Area a = fr->area;
    DisplayWsXref r = fr->display->ws_ref;
    Display *d = r->display_xref;

    sprintf(def,
	    "%ldx%ld+%ld+%ld",
	    valInt(a->w), valInt(a->h), valInt(a->x), valInt(a->y));

    mask = XGeometry(d, DefaultScreen(d),
		     strName(spec),
		     def,
		     isDefault(fr->border) ? 1 : valInt(fr->border),
		     1, 1,
		     0, 0,
		     &x, &y, &w, &h);
  
    if ( fr->status == NAME_open )
    { Window me = XtWindow(wdg);
      Window wm = getWMFrameFrame(fr);
      Window root;
      int mex, mey, wmx, wmy;
      unsigned int mew, meh, wmw, wmh, mebw, wmbw;
      unsigned depth;

      XGetGeometry(d, me, &root, &mex, &mey, &mew, &meh, &mebw, &depth);
      XGetGeometry(d, wm, &root, &wmx, &wmy, &wmw, &wmh, &wmbw, &depth);

      DEBUG(NAME_frame, Cprintf("wmbw %d; wmw %d; wmh %d; mex %d; mey %d\n",
				wmbw, wmw, wmh, mex, mey));

      if ( mask & XValue )
      { if ( mask & XNegative )
	  x -= mex;
      }
      if ( mask & YValue )
      { if ( mask & YNegative )
	  y -= mey;
      }
    }

    X = ( mask & XValue      ? toInt(x) : (Int) DEFAULT );
    Y = ( mask & YValue      ? toInt(y) : (Int) DEFAULT );
    W = ( mask & WidthValue  ? toInt(w) : (Int) DEFAULT );
    H = ( mask & HeightValue ? toInt(h) : (Int) DEFAULT );

    send(fr, NAME_set, X, Y, W, H, 0);
  }
}


void
ws_geometry_frame(FrameObj fr, Int x, Int y, Int w, Int h)
{ Widget wdg = widgetFrame(fr);

  if ( wdg )
  { XtWidgetGeometry in, out;
    DisplayWsXref r = fr->display->ws_ref;

    in.request_mode = 0;
    if ( notDefault(x) ) in.request_mode |= CWX;
    if ( notDefault(y) ) in.request_mode |= CWY;
    if ( notDefault(w) ) in.request_mode |= CWWidth;
    if ( notDefault(h) ) in.request_mode |= CWHeight;

    in.x      = valInt(fr->area->x);
    in.y      = valInt(fr->area->y);
    in.width  = valInt(fr->area->w);
    in.height = valInt(fr->area->h);

    XtMakeGeometryRequest(wdg, &in, &out);

    if ( fr->kind != NAME_popup )
    { XSizeHints hints;
      
      hints.flags  = 0;
      if ( notDefault(x) || notDefault(y) ) hints.flags |= USPosition;
      if ( notDefault(w) || notDefault(h) ) hints.flags |= USSize;

      hints.x      = valInt(fr->area->x);
      hints.y      = valInt(fr->area->y);
      hints.width  = valInt(fr->area->w);
      hints.height = valInt(fr->area->h);
      
      XSetNormalHints(r->display_xref, XtWindow(wdg), &hints);
    }
  }
}


void
ws_border_frame(FrameObj fr, int b)
{ Widget w = widgetFrame(fr);

  if ( w )
  { XtWidgetGeometry in, out;
    in.request_mode = CWBorderWidth;
    in.border_width = b;

    XtMakeGeometryRequest(w, &in, &out);
  }
}

		 /*******************************
		 *	     CURSOR		*
		 *******************************/

void
ws_busy_cursor_frame(FrameObj fr, CursorObj c)
{ Widget w = widgetFrame(fr);
  DisplayWsXref r = fr->display->ws_ref;

  if ( !w )
    return;

#define BlockAllMask (KeyPressMask | KeyReleaseMask | \
		      ButtonPressMask | ButtonReleaseMask | \
		      PointerMotionMask)

  if ( !busyWindowFrame(fr) && notNil(c) )
  { unsigned long valuemask = CWCursor;
    XSetWindowAttributes attributes;
    Size size = getSizeDisplay(fr->display);
    Window bw;

    if ( isDefault(c) )
    { if ( !(c = getResourceValueObject(fr, NAME_busyCursor)) )
	return;				/* TBD: default? */
      if ( isNil(c) )
	goto out;
    }
    attributes.cursor = (Cursor) getXrefObject(c, fr->display);

    bw = XCreateWindow(r->display_xref,
		       XtWindow(widgetFrame(fr)), 0, 0,
		       valInt(size->w), valInt(size->h),
		       (unsigned int) 0, 0, InputOnly,
		       CopyFromParent, valuemask, &attributes);

    if ( bw )
      setBusyWindowFrame(fr, bw);
    else
      errorPce(fr, NAME_failedToCreate, 0); /* TBD */
  } else if ( busyWindowFrame(fr) && instanceOfObject(c, ClassCursor) )
  { unsigned long valuemask = 0L;
    XSetWindowAttributes attributes;

    if ( notDefault(c) )
    { valuemask |= CWCursor;
      attributes.cursor = (Cursor) getXrefObject(c, fr->display);
    }

    XChangeWindowAttributes(r->display_xref,
			    busyWindowFrame(fr),
			    valuemask, &attributes);
  }

out:
  if ( notNil(c) )
    XMapRaised(r->display_xref, busyWindowFrame(fr));
  else if ( busyWindowFrame(fr) )
    XUnmapWindow(r->display_xref, busyWindowFrame(fr));
}

		 /*******************************
		 *	      COLOUR		*
		 *******************************/

void
ws_frame_background(FrameObj fr, Any c)
{ Widget w = widgetFrame(fr);

  if ( w )
  { Arg args[2];
    DisplayObj d = fr->display;
    int i=0;

    if ( instanceOfObject(c, ClassColour) )
    { XtSetArg(args[i], XtNbackground, getPixelColour(c, d));		i++;
      XtSetArg(args[i], XtNbackgroundPixmap, XtUnspecifiedPixmap);	i++;
    } else
    { Pixmap pm = (Pixmap) getXrefObject(c, d);

      XtSetArg(args[i], XtNbackgroundPixmap, pm);			i++;
    }
      
    XtSetValues(w, args, i);
  }
}


		 /*******************************
		 *	      ICONS		*
		 *******************************/

void
ws_set_icon_frame(FrameObj fr)
{ Widget w = widgetFrame(fr);

  if ( w )
  { Arg args[2];

    XtSetArg(args[0], XtNiconPixmap,
	     getXrefObject(fr->icon_image, fr->display));
    XtSetArg(args[1], XtNiconName,
	     strName(getIconLabelFrame(fr)));

    XtSetValues(w, args, 2);
  }
}


void
ws_set_icon_label_frame(FrameObj fr)
{ Widget w = widgetFrame(fr);

  if ( w )
  { Arg args[1];

    XtSetArg(args[0], XtNiconName,
	     strName(getIconLabelFrame(fr)));

    XtSetValues(w, args, 1);
  }
}


void
ws_set_icon_position_frame(FrameObj fr, int x, int y)
{ Widget w = widgetFrame(fr);
  DisplayWsXref r = fr->display->ws_ref;

  if ( w )
  { XWMHints hints;

    hints.flags = IconPositionHint;
    hints.icon_x = x;
    hints.icon_y = y;

    XSetWMHints(r->display_xref, XtWindow(w), &hints);
  }
}


status
ws_get_icon_position_frame(FrameObj fr, int *x, int *y)
{ Widget w = widgetFrame(fr);
  DisplayWsXref r = fr->display->ws_ref;

  if ( w )
  { XWMHints *hints = XGetWMHints(r->display_xref, XtWindow(w));

    if ( hints )
    { *x = hints->icon_x;
      *y = hints->icon_y;
      XFree((void *)hints);

      succeed;
    }
  }

  fail;
}


void
ws_status_frame(FrameObj fr, Name status)
{ Widget w = widgetFrame(fr);

  if ( status == NAME_window || status == NAME_fullScreen )
  { if ( w )
      XtPopup(w, XtGrabNone);
  } else if ( status == NAME_iconic )
  { if ( w )
    { Arg args[1];
      XtSetArg(args[0], XtNiconic, True);
      XtSetValues(w, args, 1);
    }
  } else if ( status == NAME_hidden )
  { if ( w )
      XtPopdown(w);
  }
}


void
ws_topmost_frame(FrameObj fr, Bool topmost)
{ 
}


		 /*******************************
		 *	       LABEL		*
		 *******************************/

void
ws_set_label_frame(FrameObj fr)
{ Widget w = widgetFrame(fr);
  DisplayWsXref r = fr->display->ws_ref;

  if ( w )
    XStoreName(r->display_xref, XtWindow(w), strName(fr->label));
}



		 /*******************************
		 *	   MISCELLANEOUS	*
		 *******************************/

Image
ws_image_of_frame(FrameObj fr)
{ Window win;

  if ( (win = getWMFrameFrame(fr)) )
  { Window root, child;
    DisplayWsXref r = fr->display->ws_ref;
    Display *d = r->display_xref;
    int x, y;
    unsigned int w, h, bw, depth;
    Image im;
    XImage *ix;

    XGetGeometry(d, win, &root, &x, &y, &w, &h, &bw, &depth);
    XTranslateCoordinates(d, win, root, 0, 0, &x, &y, &child);
    if ( notDefault(fr->border) )
      bw = valInt(fr->border);
    
    TRY(im = answerObject(ClassImage, NIL,
			  toInt(w+2*bw), toInt(h+2*bw), NAME_pixmap, 0));
    
    ix = XGetImage(d, root,
		   x-bw, y-bw, w+2*bw, h+2*bw, AllPlanes, ZPixmap);
    setXImageImage(im, ix);
    assign(im, depth, toInt(ix->depth));
    answer(im);
  }
  
  fail;
}


void
ws_transient_frame(FrameObj fr, FrameObj fr2)
{ Widget w1 = widgetFrame(fr);
  Widget w2 = widgetFrame(fr2);
  

  if ( w1 && w2 )
  { DisplayWsXref r = fr->display->ws_ref;

    XSetTransientForHint(r->display_xref,
			 XtWindow(w1),
			 XtWindow(w2));
  }
}

static int
psdepthXImage(XImage *im)
{ if ( im->depth < 3 )			/* 1, 2 */
    return im->depth;
  if ( im->depth < 8 )
    return 4;
  return 8;
}


  
status
ws_postscript_frame(FrameObj fr)
{ Window win;

  if ( (win = getWMFrameFrame(fr)) )
  { Window root, child;
    DisplayWsXref r = fr->display->ws_ref;
    Display *d = r->display_xref;
    int x, y;
    unsigned int w, h, bw, depth;
    XImage *im;
    int iw, ih;
    XWindowAttributes atts;

    XGetGeometry(d, win, &root, &x, &y, &w, &h, &bw, &depth);
    XTranslateCoordinates(d, win, root, 0, 0, &x, &y, &child);
    XGetWindowAttributes(d, root, &atts);
    
    if ( notDefault(fr->border) )
      bw = valInt(fr->border);
    
    iw = w+2*bw; ih = h+2*bw;		/* include the frame-border */
    x -= bw; y -= bw;
    if ( x < 0 ) { iw += x; x = 0; }	/* clip to the display */
    if ( y < 0 ) { ih += y; y = 0; }
    if ( x + iw > atts.width )  iw = atts.width - x;
    if ( y + ih > atts.height ) ih = atts.height - y;

    DEBUG(NAME_postscript, Cprintf("frame at %d %d %d %d\n", x, y, iw, ih));

    im = XGetImage(d, root, x, y, iw, ih, AllPlanes, ZPixmap);
    
    ps_output("0 0 ~D ~D ~D greymap\n", iw, ih, psdepthXImage(im));
    postscriptXImage(im, 0, 0, iw, ih,
		     r->display_xref, r->colour_map, 0);
    ps_output("\n");

    XDestroyImage(im);
    succeed;
  }
  
  return errorPce(fr, NAME_mustBeOpenBeforePostscript);
}
