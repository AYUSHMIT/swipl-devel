/*  $Id$

    Part of XPCE

    Author:  Jan Wielemaker and Anjo Anjewierden
    E-mail:  jan@swi.psy.uva.nl
    WWW:     http://www.swi.psy.uva.nl/projects/xpce/
    Copying: GPL-2.  See the file COPYING or http://www.gnu.org

    Copyright (C) 1990-2001 SWI, University of Amsterdam. All rights reserved.
*/


:- module(pce_manual,
	  [ manpce/0
	  , manpce/1
	  ]).
:- use_module(library(pce)).
:- consult(
	[ 'man/util'			% Common utilities
	, 'man/p_card'			% General card infra-structure
	, 'man/p_data'			% Manual specific infra-structure
	, 'man/v_manual'		% Top level window
	]).
:- require([ term_to_atom/2
	   ]).

:- pce_autoload(man_class_browser,	library('man/v_class')).
:- pce_autoload(man_editor,		library('man/v_editor')).
:- pce_autoload(man_card_editor,	library('man/v_card')).
:- pce_autoload(man_summary_browser,	library('man/v_summary')).
:- pce_autoload(man_class_hierarchy,	library('man/v_hierarchy')).
:- pce_autoload(man_search_tool,	library('man/v_search')).
:- pce_autoload(man_index_manager,	library('man/man_index')).
:- pce_autoload(man_topic_browser,	library('man/v_topic')).
:- pce_autoload(man_module_browser,	library('man/v_module')).
:- pce_autoload(man_statistics,		library('man/v_statistics')).
:- pce_autoload(isp_frame,		library('man/v_inspector')).
:- pce_autoload(vis_frame,		library('man/v_visual')).
:- pce_autoload(man_instance_browser,	library('man/v_instance')).
:- pce_autoload(man_global,		library('man/v_global')).
:- pce_autoload(man_object_browser,	library('man/v_global')).
:- pce_autoload(man_error_browser,	library('man/v_error')).
:- pce_autoload(man_group_browser,	library('man/v_group')).

:- pce_global(@manual, new(man_manual)).

manpce :-
	send(@manual, open).


manpce(Spec) :-
	(   (   method(Spec, Object)
	    ;	atom(Spec),
		term_to_atom(S0, Spec),
		method(S0, Object)
	    )
	->  send(@manual, manual, Object)
	;   term_to_atom(Spec, Atom),
	    send(@pce, report, warning,
		 '[manpce/1: Cannot find: %s]', Atom),
	    fail
	).


method(Obj, Obj) :-
	object(Obj), !.
method(ClassName, Class) :-
	atom(ClassName),
	get(@pce, convert, ClassName, class, Class).
method(->(Receiver, Selector), Method) :- !,
	(   atom(Receiver)
	->  get(@pce, convert, Receiver, class, Class),
	    get(Class, send_method, Selector, Method)
	;   object(Receiver)
	->  get(Receiver, send_method, Selector, tuple(_, Method))
	).
method(<-(Receiver, Selector), Method) :- !,
	(   atom(Receiver)
	->  get(@pce, convert, Receiver, class, Class),
	    get(Class, get_method, Selector, Method)
	;   object(Receiver)
	->  get(Receiver, get_method, Selector, tuple(_, Method))
	).
