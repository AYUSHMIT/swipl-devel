/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1994 University of Amsterdam. All rights reserved.
*/

:- module(pl_show_help,
	  [ show_help_hook/2
	  ]).

:- pce_global(@pl_help_view, make_pl_help_view).

make_pl_help_view(V) :-
	new(V, view('Prolog Help View')),
	send(V, confirm_done, @off).


show_help_hook(What, HelpFile) :-
	term_to_atom(What, Atom),
	new(S, string(Atom)),
	send(S, translate, ' ', @nil),
	send(@pl_help_view?frame, label, string('Prolog help on "%s"', S)),
	send(S, done),
	send(@pl_help_view, load, HelpFile),
	send(@pl_help_view, open),
	send(@pl_help_view, expose).
