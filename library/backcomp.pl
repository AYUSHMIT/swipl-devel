/*  $Id$

    Designed and implemented by Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1994 University of Amsterdam. All rights reserved.
*/

:- module(backward_compatibility,
	  [ '$arch'/2,
	    '$version'/1,
	    '$home'/1,
	    display/1,
	    display/2,
	    displayq/1,
	    displayq/2
	  ]).

'$arch'(Arch, unknown) :-
	feature(arch, Arch).

'$version'(Version) :-
	feature(version, Version).

'$home'(Home) :-
	feature(home, Home).

display(Term) :-
	write_term(Term, [ignore_ops(true)]).
display(Stream, Term) :-
	write_term(Stream, Term, [ignore_ops(true)]).

%	or write_canonical/[1,2]

displayq(Term) :-
	write_term(Term, [ignore_ops(true),quoted(true)]).
displayq(Stream, Term) :-
	write_term(Stream, Term, [ignore_ops(true),quoted(true)]).

