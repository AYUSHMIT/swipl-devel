/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1994 University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>
#include <h/graphics.h>
#include "include.h"
#include <X11/keysym.h>

XtAppContext	ThePceXtAppContext;	/* X toolkit application context */

		 /*******************************
		 *       X11 APP CONTEXT	*
		 *******************************/

static int
x_error_handler(Display *display, XErrorEvent *error)
{ char msg[1024];
  char request[100];
  char buf[100];

  XGetErrorText(display, error->error_code, msg, 1024);
  sprintf(buf, "%d", error->request_code);
  XGetErrorDatabaseText(display, "XRequest", buf,
			"Unknown request", request, 100);
  fprintf(stderr, "X error of failed request: %s\n", msg);
  fprintf(stderr, "Major opcode of failed request: %d (%s)\n",
	  error->request_code, request);
  fprintf(stderr, "Minor opcode of failed request: %d\n", error->minor_code);
  fprintf(stderr, "Resource id in failed request:  0x%x\n",
	  (unsigned int) error->resourceid);
  fprintf(stderr, "Serial number of failed request: %ld\n", error->serial);
  
  errorPce(NIL, NAME_xError);

  return 0;				/* what to return here? */
}


XtAppContext
pceXtAppContext(XtAppContext ctx)
{ if ( ThePceXtAppContext == NULL )
  { if ( ctx != NULL )
    { ThePceXtAppContext = ctx;
      XSetErrorHandler(x_error_handler);
    } else
    { XtToolkitInitialize();
      XSetErrorHandler(x_error_handler);

      if ( (ThePceXtAppContext = XtCreateApplicationContext()) == NULL )
      { errorPce(TheDisplayManager(), NAME_noApplicationContext);
	fail;
      }
    }
  }

  return ThePceXtAppContext;
}

		 /*******************************
		 *	 WIDGET REFERENCE	*
		 *******************************/

Widget
widgetWindow(PceWindow sw)
{ return (Widget) sw->ws_ref;
}


Widget
widgetFrame(FrameObj fr)
{ return fr->ws_ref ? ((frame_ws_ref *)fr->ws_ref)->widget : NULL;
}

void
setXImageImage(Image image, XImage *i)
{ image->ws_ref = i;
}


		 /*******************************
		 *	     X11 ATOMS		*
		 *******************************/

#if O_MOTIF
#include <Xm/Xm.h>
#include <Xm/AtomMgr.h>			/* was X11? */
#include <Xm/Protocols.h>		/* idem */

#define XInternAtom(d, nm, v) XmInternAtom(d, nm, v)
#define XGetAtomName(d, atom) XmAtomToName(d, atom)
#endif

Atom
DisplayAtom(DisplayObj d, Name name)
{ DisplayWsXref r = d->ws_ref;

  return XInternAtom(r->display_xref, strName(name), False);
}


char *
DisplayAtomToString(DisplayObj d, Atom atom)
{ DisplayWsXref r = d->ws_ref;

  return XGetAtomName(r->display_xref, atom);
}


Atom
FrameAtom(FrameObj fr, Name name)
{ return DisplayAtom(fr->display, name);
}


char *
FrameAtomToString(FrameObj fr, Atom a)
{ return DisplayAtomToString(fr->display, a);
}


Atom
WmProtocols(FrameObj fr)
{ return FrameAtom(fr, CtoName("WM_PROTOCOLS"));
}

		 /*******************************
		 *	  FRAME THINGS		*
		 *******************************/

Window
getWMFrameFrame(FrameObj fr)
{ Widget wdg;

  if ( (wdg = widgetFrame(fr)) )
  { Window w = XtWindow(wdg);
    DisplayWsXref r = fr->display->ws_ref;

    if ( equalName(fr->kind, NAME_popup) )
      return w;
    else
    { Window root, parent, *children;
      unsigned int nchildren;
      int m = 5;

      while(--m >= 0)			/* avoid a loop */
      { if ( !XQueryTree(r->display_xref, w, &root, &parent,
			 &children, &nchildren) )
	  return w;
	XFree((char *) children);	/* declared char * ???? */
	DEBUG(NAME_frame, printf("w = %ld; root = %ld; parent = %ld\n",
				 w, root, parent));
	if ( parent == root )
	  return w;

	w = parent;
      }
    }
  }

  return 0;
}

		 /*******************************
		 *	     POSTSCRIPT		*
		 *******************************/

#define putByte(b) { ps_put_char(print[(b >> 4) & 0xf]); \
		     ps_put_char(print[b & 0xf]); \
 		     if ( (++bytes % 32) == 0 ) ps_put_char('\n'); \
		     bits = 8; c = 0; \
		   }

void
postscriptXImage(XImage *im, int w, int h, ulong bg)
{ static char print[] = {'0', '1', '2', '3', '4', '5', '6', '7',
			 '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  int x, y;
  int bits, bytes;
  int c;

  for(bytes = y = c = 0, bits = 8; y < h; y++)
  { for(x=0; x < w; x++)
    { bits--;
      if ( XGetPixel(im, x, y) != bg )
	c |= 1 << bits;
      if ( bits == 0 )
        putByte(c);
    }
    if ( bits != 8 )
      putByte(c);
  }
}


		 /*******************************
		 *       COLOUR HANDLING	*
		 *******************************/

ulong
getPixelColour(Colour c, DisplayObj d)
{ XColor *color = (XColor *) getXrefObject(c, d);

  return color ? color->pixel : 0L;
}


void
x11_set_gc_foreground(DisplayObj d, Any fg, int gcs, GC *gc)
{ XGCValues values;
  ulong mask;
  DisplayWsXref r = d->ws_ref;

  if ( instanceOfObject(fg, ClassColour) )
  { ulong pixel = getPixelColour(fg, d);
	
    values.foreground = pixel;
    values.fill_style = FillSolid;
    mask	      = (GCForeground|GCFillStyle);
  } else
  { Pixmap pm   = (Pixmap) getXrefObject(fg, d);
    
    values.tile       = pm;
    values.fill_style = FillTiled;
    mask	      = (GCTile|GCFillStyle);
  }

  for(; gcs > 0; gcs--, gc++)
    XChangeGC(r->display_xref, *gc, mask, &values);
}


		/********************************
		*      X-EVENT TRANSLATION	*
		********************************/

static Any
keycode_to_name(XEvent *event)
{ char buf[256];
  int bytes;
  KeySym sym;

  bytes = XLookupString((XKeyEvent *) event, buf, 256, &sym, NULL);
  if ( bytes == 1 )
  { int c = buf[0];

    if ( event->xkey.state & Mod1Mask )	/* meta depressed */
      c += META_OFFSET;

    return toInt(c);
  }

  switch(sym)
  { case XK_F1:		return NAME_keyTop_1;
    case XK_F2:		return NAME_keyTop_2;
    case XK_F3:		return NAME_keyTop_3;
    case XK_F4:		return NAME_keyTop_4;
    case XK_F5:		return NAME_keyTop_5;
    case XK_F6:		return NAME_keyTop_6;
    case XK_F7:		return NAME_keyTop_7;
    case XK_F8:		return NAME_keyTop_8;
    case XK_F9:		return NAME_keyTop_9;
    case XK_F10:	return NAME_keyTop_10;

    case XK_L1:		return NAME_keyLeft_1;
    case XK_L2:		return NAME_keyLeft_2;
    case XK_L3:		return NAME_keyLeft_3;
    case XK_L4:		return NAME_keyLeft_4;
    case XK_L5:		return NAME_keyLeft_5;
    case XK_L6:		return NAME_keyLeft_6;
    case XK_L7:		return NAME_keyLeft_7;
    case XK_L8:		return NAME_keyLeft_8;
    case XK_L9:		return NAME_keyLeft_9;
    case XK_L10:	return NAME_keyLeft_10;

    case XK_R1:		return NAME_keyRight_1;
    case XK_R2:		return NAME_keyRight_2;
    case XK_R3:		return NAME_keyRight_3;
    case XK_R4:		return NAME_keyRight_4;
    case XK_R5:		return NAME_keyRight_5;
    case XK_R6:		return NAME_keyRight_6;
    case XK_R7:		return NAME_keyRight_7;
    case XK_R8:		return NAME_keyRight_8;
    case XK_R9:		return NAME_keyRight_9;
    case XK_R10:	return NAME_keyRight_10;
    case XK_R11:	return NAME_keyRight_11;
    case XK_R12:	return NAME_keyRight_12;
    case XK_R13:	return NAME_keyRight_13;
    case XK_R14:	return NAME_keyRight_14;
    case XK_R15:	return NAME_keyRight_14;

    case XK_Home:	return NAME_cursorHome;
    case XK_Left:	return NAME_cursorLeft;
    case XK_Up:		return NAME_cursorUp;
    case XK_Right:	return NAME_cursorRight;
    case XK_Down:	return NAME_cursorDown;
  }

  DEBUG(NAME_keysym, printf("sym = 0x%X\n", (unsigned int)sym));

  fail;
}


static Int
state_to_buttons(unsigned int state, Name name)
{ int r = 0;

  if ( state & Button1Mask )	r |= BUTTON_ms_left;
  if ( state & Button2Mask )	r |= BUTTON_ms_middle;
  if ( state & Button3Mask )	r |= BUTTON_ms_right;
  if ( state & ShiftMask )	r |= BUTTON_shift;
  if ( state & ControlMask )	r |= BUTTON_control;
  if ( state & Mod1Mask )	r |= BUTTON_meta;

  if ( equalName(name, NAME_msLeftDown) )
    r |= BUTTON_ms_left;
  else if ( equalName(name, NAME_msMiddleDown) )
    r |= BUTTON_ms_middle;
  else if ( equalName(name, NAME_msRightDown) )
    r |= BUTTON_ms_right;
  else if ( equalName(name, NAME_msLeftUp) )
    r &= ~BUTTON_ms_left;
  else if ( equalName(name, NAME_msMiddleUp) )
    r &= ~BUTTON_ms_middle;
  else if ( equalName(name, NAME_msRightUp) )
    r &= ~BUTTON_ms_right;

  return toInt(r);
}


static Name
button_to_name(int press, unsigned int button)
{ switch( button )
  { case Button1:	return press ? NAME_msLeftDown   : NAME_msLeftUp;
    case Button2:	return press ? NAME_msMiddleDown : NAME_msMiddleUp;
    case Button3:	return press ? NAME_msRightDown  : NAME_msRightUp;
  }

  fail;
}



EventObj
CtoEvent(PceWindow window, XEvent *event)
{ Time time;
  int state = 0;
  int x;
  int y;
  Any name;

  switch( event->xany.type )
  { case KeyPress:			/* Key pressed */
    { x     = event->xkey.x;
      y     = event->xkey.y;
      state = event->xkey.state;
      time  = event->xkey.time;
      name  = keycode_to_name(event);
      if ( name == FAIL )
        fail;

      break;
    }
    case ButtonPress:			/* Button pressed/released */
    case ButtonRelease:
    { x     = event->xbutton.x;
      y     = event->xbutton.y;
      state = event->xbutton.state;
      time  = event->xbutton.time;
      name  = button_to_name(event->xany.type == ButtonPress,
				   event->xbutton.button);
      if ( name == FAIL )
        fail;

      break;
    }
    case MotionNotify:			/* Pointer motion events */
    { x     = event->xmotion.x;
      y     = event->xmotion.y;
      state = event->xmotion.state;
      time  = event->xmotion.time;

      if ( state & Button1Mask )
        name = NAME_msLeftDrag;
      else if ( state & Button2Mask )
        name = NAME_msMiddleDrag;
      else if ( state & Button3Mask )
        name = NAME_msRightDrag;
      else
        name = NAME_locMove;

      break;
    }
    case EnterNotify:
    case LeaveNotify:
    { x     = event->xcrossing.x;
      y     = event->xcrossing.y;
      state = event->xcrossing.state;
      time  = event->xcrossing.time;

#     define AnyButtonMask (Button1Mask|Button2Mask|Button3Mask)

      if ( event->xany.type == EnterNotify )
      { name = (state & AnyButtonMask ? NAME_areaResume
		 		      : NAME_areaEnter);
      } else
      { name = (state & AnyButtonMask ? NAME_areaCancel
			    	      : NAME_areaExit);
      }

      break;
    }
   default:				/* unknown event: do not convert */
      fail;
  }

  setLastEventTime(time);

  return answerObject(ClassEvent,
		      name, 
		      window,
		      toInt(x), toInt(y),
		      state_to_buttons(state, name),
		      0);		   
}

		 /*******************************
		 *      COMPATIBILITY HACKS	*
		 *******************************/

#if XT_REVISION <= 3

Status XSetWMProtocols (dpy, w, protocols, count)
    Display *dpy;
    Window w;
    Atom *protocols;
    int count;
{   static Atom a = None;

    if ( a == None )
	a = XInternAtom (dpy, "WM_PROTOCOLS", False);

    XChangeProperty (dpy, w, a, XA_ATOM, 32,
		     PropModeReplace, (unsigned char *) protocols, count);
    return True;
}

#endif

#if !defined(HAVE_XTPOPUPSPRINGLOADED) && !defined(sun)
					/* sun's X11R5 fails on AC_HAVE_FUNC */
					/* for X11 functions */

void XtPopupSpringLoaded (widget)
    Widget widget;
{
    _XtPopup(widget, XtGrabExclusive, True);
}

#endif
