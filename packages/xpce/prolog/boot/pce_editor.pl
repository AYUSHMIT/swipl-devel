/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1993 University of Amsterdam. All rights reserved.
*/

:- module(pce_editor_buttons, []).
:- use_module(pce_global).
:- use_module(pce_principal, [send/2, send/3, send/4, send/6, get/3, new/2]).
:- use_module(pce_realise, [pce_extended_class/1]).
:- use_module(pce_error, [pce_catch_error/2]).


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This module defines how the pointer-buttons are handled by class editor.
The   method   `editor   ->event'   will   check   the   global   object
@editor_recogniser.  If it exists,  it  will   be  invoked  *after*  the
general device event method, typing and area enter/exit events have been
tried.  If it does not exist, the predefined button actions are invoked.

This module attempts to avoid using modifiers and be consistent with
other X11 applications:

	* Left-click sets the caret
	* Left-drag makes a selection
	* Right click/drag extends the selection if it exists
	* left-double/tripple click selects a word/line.  Subsequent
	  dragging extends the selection with this unit.

Comments are welcome!
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


:- pce_global(@editor_recogniser, make_editor_recogniser).

make_editor_recogniser(R) :-
	new(Editor, @event?receiver),
	new(Image, Editor?image),
	new(Index, ?(Image, index, @event)),

	new(R, handler_group),
	send(R, attribute, attribute(saved_caret, 0)),
	send(R, attribute, attribute(down_index, 0)),

	new(DownIndex, R?down_index),

	send(R, append,
	     new(L, click_gesture(left, '', single,
			  and(message(Editor, caret, DownIndex),
			      message(Editor, selection_unit, character),
			      message(Editor, selection_origin, DownIndex)),
			  and(message(R, saved_caret, Editor?caret),
			      message(R, down_index, Index))))),
	send(L, max_drag_distance, 25),			  
				  
	send(R, append,
	     click_gesture(middle, '', single,
			   message(Editor, import_selection))),

	make_make_selection_gesture(R, MakeSelectionGesture),
	make_extend_selection_gesture(ExtendGesture),
	send(R, append, MakeSelectionGesture),
	send(R, append, ExtendGesture).

	
make_make_selection_gesture(R, G) :-
	new(G, gesture),
	new(Editor, @arg1?receiver),
	new(Image, Editor?image),
	new(Index, ?(Image, index, @arg1)),
	
	send(G, send_method,
	     send_method(initiate, vector(event),
		 and(message(Editor, selection_unit,
			     when(@arg1?multiclick == single,
				  character,
				  progn(message(Editor, caret, R?saved_caret),
					when(@arg1?multiclick == double,
					     word, line)))),
		     message(Editor, selection_origin, Index)))),
	send(G, send_method,
	     send_method(drag, vector(event),
			 message(Editor, selection_extend, Index))),
	send(G, send_method,
	     send_method(terminate, vector(event),
			 and(message(Editor, selection_extend, Index),
			     message(Editor, export_selection)))).
					 

make_extend_selection_gesture(G) :-
	new(G, gesture(right)),
	new(Editor, @arg1?receiver),
	new(Image, Editor?image),
	new(Index, ?(Image, index, @arg1)),
	
	send(G, condition, Editor?selection_start \== Editor?selection_end),

	send(G, send_method,
	     send_method(initiate, vector(event),
		 and(if(@arg1?multiclick \== single,
			message(Editor, selection_unit,
			     when(@arg1?multiclick == double,
				  word,
				  line))),
		     message(Editor, selection_extend, Index)))),
	send(G, send_method,
	     send_method(drag, vector(event),
			 message(Editor, selection_extend, Index))),
	send(G, send_method,
	     send_method(terminate, vector(event),
			 and(message(Editor, selection_extend, Index),
			     message(Editor, export_selection)))).
					 

:- pce_extend_class(editor).

export_selection(E) :->
	"Export the selection to the X11 world"::
	send(E, selection_to_cut_buffer),
	get(E, display, Display),
	(   get(Display, selection_owner, E)
	->  true
	;   send(Display, selection_owner, E, primary,
		 @receiver?selected,
		 message(@receiver, selection, 0, 0))
	).


import_selection(E) :->
	"Import the (primary) selection or the cut_buffer"::
	(   get(E, editable, @on)
	->  get(E, display, Display),
	    (   pce_catch_error(get_selection, get(Display, selection, String)),
		send(E, insert, String),
		send(String, done)
	    ;   send(E, insert_cut_buffer)
	    )
	;   send(E, report, warning, 'Text is read-only'),
	    fail
	).
	

:- pce_end_class.
