/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1993 University of Amsterdam. All rights reserved.
*/


:- module(pce_draw,
	  [ pcedraw/0			% start pcedraw
	  , pcedraw/1			% and load a file into it
	  , draw_toplevel/0		% toplevel when compiled
	  , draw_begin_shape/4		% user-defined shapes
	  , draw_end_shape/0
	  ]).

:- use_module(library(pce)).
:- require([ checklist/2
	   ]).

:- consult(library('draw/draw')).

pcedraw :-
	draw.
pcedraw(File) :-
	draw(File).


		/********************************
		*   CREATE STAND ALONE PROGRAM	*
		********************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
For SWI-Prolog, PceDraw can be turned into a stand-alone program by the
followoing command:

	xpce -o pcedraw -f none -g draw_toplevel -c pcedraw.pl

This command can be started with

	pcedraw file ...
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */



draw_toplevel :-
	'$argv'(Args),		% command line arguments
	files(Args, Files),
	(   Files = [_|_]
	->  checklist(draw, Files)
	;   draw
	).


%	The SWI-Prolog command line args when restarting a compiled file
%	are: <executable> -x <Startup> <UserArg1> ...

files([], []).
files(['-x', _Startup|Files], Files) :- !.
files([_|Args], Files) :-
	files(Args, Files).
