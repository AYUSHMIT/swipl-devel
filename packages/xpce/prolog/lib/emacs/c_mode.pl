/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    M-mail: jan@swi.psy.uva.nl

    Copyright (C) 1993 University of Amsterdam. All rights reserved.
*/

:- module(emacs_c_mode, []).
:- use_module(library(pce)).
:- require([ between/3
	   , forall/2
	   , memberchk/2
	   ]).

:- pce_begin_class(emacs_c_mode, emacs_language_mode).

:- initialization
	new(KB, emacs_key_binding(c, language)),
	send(KB, function, '{',		insert_c_begin),
	send(KB, function, '\C-cRET',	prototype_mark),
	send(KB, function, '\C-c\C-p',	add_prototype),
	send(KB, function, '\C-c\C-n',	insert_NAME_).

:- initialization
	new(X, syntax_table(c)),
	send(X, syntax, '"',  string_quote, '\'),
	send(X, syntax, '''', string_quote, '\'),
	
	send(X, add_syntax, '/',  comment_start, '*'),
	send(X, add_syntax, '*',  comment_end, '/'),

	send(X, paragraph_end, regex('\s *$\|/\*\|\*/')).

:- initialization
	new(MM, emacs_mode_menu(c, language)),
	send(MM, append, gdb, gdb).


:- pce_global(@c_indent, new(number(2))).
:- pce_global(@c_undent_regex, new(regex('{\|else\|\w+:'))).

indent_line(E, Times:[int]) :->
	"Indent according to C-mode"::
	default(Times, 1, Tms),
	(   between(1, Tms, N),
	    send(E, beginning_of_text_on_line),
	    (	(   send(E, indent_close_bracket_line)
		;   send(E, indent_expression_line, ')]')
		;   send(E, indent_statement)
		;   send(E, align_with_previous_line, '\s *\({\s *\)*')
		)
	    ->	true
	    ),
	    (	N == Tms
	    ->	true
	    ;	send(E, next_line)
	    ),
	    fail
	;   true
	).


backward_skip_statement(TB, Here, Start) :-
	get(TB, skip_comment, Here, 0, H1),
	(   (	H1 == 0   
	    ;	get(TB, character, H1, C1),
		memberchk(C1, "{;}")
	    ),
	    get(TB, skip_comment, H1+1, H2),
	    \+ send(regex(else), match, TB, H2)
	->  Start = H2
	;   get(TB, scan, H1, term, -1, start, H2),
	    H3 = H2 - 1,
	    backward_skip_statement(TB, H3, Start)
	).


backward_statement(E, Here:[int], There:int) :<-
	"Find start of C-statement"::
	default(Here, E?caret, Caret),
	get(E, text_buffer, TB),
	get(TB, skip_comment, Caret, 0, H1),
	(   get(TB, character, H1, Chr),
	    memberchk(Chr, "};")
	->  get(TB, scan, H1+1, term, -1, H2a),
	    H2 is H2a - 1
	;   H2 = H1
	),
	backward_skip_statement(TB, H2, There).


backward_statement(E) :->
	"Go back one statement"::
	get(E, backward_statement, Start),
	send(E, caret, Start).


indent_statement(E) :->
	"Indent statement in { ... } context"::
	get(E, text_buffer, TB),
	get(E, caret, Caret),
	get(E, matching_bracket, Caret, '}', _), % in C-body
	get(TB, skip_comment, Caret-1, 0, P0),
	get(TB, character, P0, Chr),
	(   memberchk(Chr, ";}")	% new statement
	->  get(E, backward_statement, P0+1, P1),
	    back_prefixes(E, P1, P2),
	    get(E, column, P2, Col),
	    send(E, align_line, Col)
	;   memberchk(Chr, "{")		% first in compound block
	->  get(E, column, P0, Col),
	    send(E, align, Col + @c_indent)
	;   \+ memberchk(Chr, ";,"),	% for, while, if, ...
	    (   get(TB, matching_bracket, P0, P1)
	    ->  true
	    ;   P1 = P0
	    ),
	    get(TB, scan, P1, word, 0, start, P2),
	    back_prefixes(E, P2, P3),
	    get(E, column, P3, Column),
	    (   send(@c_undent_regex, match, TB, Caret)
	    ->  send(E, align, Column)
	    ;   send(E, align, Column + @c_indent)
	    )
	).


back_prefixes(E, P0, P) :-
	get(E, text_buffer, TB),
	get(TB, scan, P0, line, 0, start, SOL),
	(   get(regex('\s(\|:'), search, TB, P0, SOL, P1)
	->  P2 is P1 + 1
	;   P2 = SOL
	),
	get(TB, skip_comment, P2, P0, P).


insert_c_begin(E, Times:[int], Id:[event_id]) :->
	"Insert and adjust the inserted '{'"::
	send(E, insert_self, Times, Id),
	get(E, caret, Caret),
	get(E, text_buffer, TB),
	get(TB, scan, Caret, line, 0, start, SOL),
	(   send(regex(string('\\s *%c', Id)), match, TB, SOL, Caret)
	->  new(F, fragment(TB, Caret, 0)),
	    send(E, indent_line),
	    send(E, caret, F?start),
	    free(F)
	;   true
	).


		 /*******************************
		 *	   PROTOPTYPES		*
		 *******************************/

:- pce_global(@emacs_makeproto, make_makeproto).

make_makeproto(P) :-
	new(P, process(makeproto)),
	send(P, open).


prototype_mark(E) :->
	"Make a mark for local prototype definitions"::
	get(E, caret, Caret),
	get(E, text_buffer, TB),
	send(TB, attribute, attribute(prototype_mark_location,
				      fragment(TB, Caret, 0))),
	send(E, report, status, 'Prototype mark set').

add_prototype(E) :->
	"Add prototype for pointed function"::
	get(E, caret, Caret),
	get(E, text_buffer, TB),
	get(TB, scan, Caret, paragraph, 0, start, SOH),
	get(TB, find, SOH, '{', 1, end, EOH),
	get(TB, contents, SOH, EOH-SOH, Header),
	send(@emacs_makeproto, format, '%s\n}\n', Header),
	send(@pce, format, '%s\n', Header),
	get(TB, prototype_mark_location, Fragment),
	get(Fragment, end, End),
	forall(get(@emacs_makeproto, read_line, 100, Prototype),
	       send(Fragment, insert, @default, Prototype)),
	send(E, caret, End).
	
	
		 /*******************************
		 *         XPCE THINGS		*
		 *******************************/

insert_NAME_(M) :->
	"Insert NAME_"::
	send(M, format, 'NAME_').

:- pce_end_class.

