/*  $Id$ $

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1992 University of Amsterdam. All rights reserved.
*/

:- module(emacs, []).

:- use_module(library(pce)).
:- require([ declare_emacs_mode/2
	   , send_list/3
	   ]).

		/********************************
		*         DECLARE MODES		*
		********************************/

fix_mode_name_type :-
	get(@pce, convert, mode_name, type, Type),
	send(Type, name_reference, mode_name_type),
	send(Type, kind, name_of),
	send(Type, slot, context, new(Ctx, chain)),
	send_list(Ctx, append,
		  [ fundamental
		  , prolog
		  , shell
		  ]).

:- initialization fix_mode_name_type.


		 /*******************************
		 *           PROLOG		*
		 *******************************/

:- initialization new(@loading_emacs, object).
					% SWI-Prolog extensions
pce_ifhostproperty(prolog(swi),
		   (:- ensure_loaded(user:library('emacs/swi_prolog')))). 


		 /*******************************
		 *	    LIBRARIES		*
		 *******************************/

:- pce_autoload(file_item, library(file_item)).


		 /*******************************
		 *          KERNEL FILES	*
		 *******************************/

:- consult(window).
:- consult(buffer).
:- consult(buffer_menu).
:- consult(server).
:- consult(fundamental_mode).


		 /*******************************
		 *       AUTOLOAD CLASSES	*
		 *******************************/

:- pce_autoload(emacs_hit_list,		library('emacs/hit_list')).
:- pce_autoload(emacs_process_buffer,	library('emacs/shell')).
:- pce_autoload(emacs_gdb_buffer,	library('emacs/gdb')).
:- pce_autoload(emacs_annotate_buffer,  library('emacs/annotate_mode')).


		 /*******************************
		 *	      MODES		*
		 *******************************/




:- declare_emacs_mode(outline,		library('emacs/outline_mode')).
:- declare_emacs_mode(language,		library('emacs/language_mode')).
:- declare_emacs_mode(prolog,		library('emacs/prolog_mode')).
:- declare_emacs_mode(latex,		library('emacs/latex_mode')).
:- declare_emacs_mode(c,		library('emacs/c_mode')).
:- declare_emacs_mode('c++',		library('emacs/cpp_mode')).
:- declare_emacs_mode(script,		library('emacs/script_mode')).
:- declare_emacs_mode(man,		library('emacs/man_mode')).
:- declare_emacs_mode(text,		library('emacs/text_mode')).
:- declare_emacs_mode(annotate,		library('emacs/annotate_mode')).
:- declare_emacs_mode(gdb,		library('emacs/gdb')).


		 /*******************************
		 *     EMACS GLOBAL OBJECTS	*
		 *******************************/

:- pce_global(@emacs_mark_list,
	      new(emacs_hit_list('Emacs Mark List'))).
:- pce_global(@emacs_base_names,
	      new(chain_table)).		  % file-base --> buffers
:- pce_global(@emacs_buffers,
	      new(dict)).			  % name --> buffer object
:- pce_global(@emacs_modes,
	      new(hash_table)).			  % name --> mode object
:- pce_global(@emacs,
	      new(emacs_buffer_menu(@emacs_buffers))).
:- pce_global(@emacs_comment_column, new(number(40))).
:- pce_global(@emacs_mode_list,			  % regex --> mode
	      new(sheet(attribute(regex('.*\.pl~?$'),   	prolog),
			attribute(regex('\.\(pl\|xpce\|pceemacs\)rc~?'),
				  prolog),
			attribute(regex('.*\.\(tex\|sty\)~?$'), latex),
			attribute(regex('.*\.doc~?$'),	 	latex),
			attribute(regex('.*\.ann~?$'),	 	annotate),
			attribute(regex('.*\.[ch]~?$'), 	c),
			attribute(regex('.*\.C$'),		'c++'),
			attribute(regex('.*\.cc$'),		'c++'),
			attribute(regex('.*\.cpp$'),		'c++'),
			attribute(regex('[Cc]ompose\|README'),	text)))).
:- pce_global(@emacs_default_mode, new(var(value := script))).

:- free(@loading_emacs).

