/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1994 University of Amsterdam. All rights reserved.
*/

:- module(dialog,
	  [ dialog/0			% start dialog editor
	  ]).
:- use_module(library(pce)).

dialog :-
	ensure_loaded(library('dialog/dialog')),
	send(new(dia_editor), open).
	
