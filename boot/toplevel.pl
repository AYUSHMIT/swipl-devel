/*  $Id$

    Copyright (c) 1990 Jan Wielemaker. All rights reserved.
    jan@swi.psy.uva.nl

    Purpose: top level user interaction
*/

:- module($toplevel,
	[ $initialise/0			% start Prolog (does not return)
	, $toplevel/0			% Prolog top-level (re-entrant)
	, $abort/0 			% restart after an abort
	, $break/0 			% live in a break
	, $compile/0 			% `-c' toplevel
	, $welcome/0			% banner
	, prolog/0 			% user toplevel predicate
	, $set_prompt/1			% set the main prompt
	, at_initialization/1		% goals to run at initialization
	, (initialization)/1		% initialization goal (directive)
	]).


		/********************************
		*         INITIALISATION        *
		*********************************/

:- dynamic
	loaded_init_file/1.		% already loaded init files

$welcome :-
	print_message(help, welcome).

$load_init_file(none) :- !.
$load_init_file(Base) :-
	loaded_init_file(Base), !.
$load_init_file(InitFile) :-
	is_absolute_file_name(InitFile), !,
	ensure_loaded(user:InitFile).
$load_init_file(Base) :-
	absolute_file_name(user_profile(Base),
			   [ access(read),
			     file_errors(fail)
			   ], InitFile),
	asserta(loaded_init_file(Base)),
	ensure_loaded(user:InitFile).
$load_init_file(_).

$load_system_init_file :-
	loaded_init_file(system), !.
$load_system_init_file :-
	$option(system_init_file, Base, Base),
	(   Base == none
	->  asserta(loaded_init_file(system))
	;   current_prolog_flag(home, Home),
	    file_name_extension(Base, rc, Name),
	    concat_atom([Home, '/', Name], File),
	    access_file(File, read),
	    asserta(loaded_init_file(system)),
	    load_files(user:File, [silent(true)]), !
	).
$load_system_init_file.

$load_gnu_emacs_interface :-
	getenv('EMACS', t),
	current_prolog_flag(argv, Args),
	memberchk('+C', Args), !,
	user:ensure_loaded(library(emacs_interface)).
$load_gnu_emacs_interface.

		 /*******************************
		 *	 AT_INITIALISATION	*
		 *******************************/

:- module_transparent
	at_initialization/1,
	(initialization)/1.
:- dynamic
	$at_initialization/1.

at_initialization(Spec) :-
	$strip_module(Spec, Module, Goal),
	'$toplevel':assert($at_initialization(Module:Goal)).

$run_at_initialization :-
	\+ current_prolog_flag(saved_program, true), !.
$run_at_initialization :-
	(   $at_initialization(Goal),
	    (   catch(Goal, E,
		      print_message(error, initialization_exception(Goal, E)))
	    ->  fail
	    ;   print_message(warning, goal_failed(at_initialization, Goal)),
		fail
	    )
	;   true
	).

%	initialization(+Goal)
%
%	Runs `Goal' both a load and initialization time.

initialization(Goal) :-
	at_initialization(Goal),
	Goal.


		 /*******************************
		 *     FILE SEARCH PATH (-p)	*
		 *******************************/

$set_file_search_paths :-
	current_prolog_flag(argv, Argv),
	append(H, ['-p', Path|_], Argv),
	\+ member(H, '--'),
	(   atom_chars(Path, Chars),
	    (	phrase($search_path(Name, Aliases), Chars)
	    ->	reverse(Aliases, Aliases1),
	        forall(member(Alias, Aliases1),
		       asserta(user:file_search_path(Name, Alias)))
	    ;   print_message(error, commandline_arg_type(p, Path))
	    )
	->  true
	),
	fail ; true.

$search_path(Name, Aliases) -->
	$string(NameChars),
	[=], !,
	{atom_chars(Name, NameChars)},
	$search_aliases(Aliases).

$search_aliases([Alias|More]) -->
	$string(AliasChars),
	[:], !,
	{ $make_alias(AliasChars, Alias) },
	$search_aliases(More).
$search_aliases([Alias]) -->
	$string(AliasChars),
	$eos, !,
	{ $make_alias(AliasChars, Alias) }.

$string(X) --> {X=[_|_]}, X.

$eos([], []).

$make_alias(Chars, Alias) :-
	catch(term_to_atom(Alias, Chars), _, fail),
	(   atom(Alias)
	;   functor(Alias, F, 1),
	    F \== /
	), !.
$make_alias(Chars, Alias) :-
	atom_chars(Alias, Chars).


		 /*******************************
		 *   LOADING ASSIOCIATED FILES	*
		 *******************************/

$load_associated_file :-
	current_prolog_flag(associate, Ext),
	current_prolog_flag(argv, [_,OsFile]),
	prolog_to_os_filename(File, OsFile),
	file_name_extension(_, Ext, File),
	access_file(File, read),
	file_directory_name(File, Dir),
	chdir(Dir),
	consult(user:File), !,
	atom_concat('SWI-Prolog -- ', File, Title),
	G = user:window_title(_, Title),
	(   current_predicate(_, G)
	->  G
	;   true
	),
	nl.
$load_associated_file.


		/********************************
		*        TOPLEVEL GOALS         *
		*********************************/

:- flag($banner_goal, _, $welcome).
:- flag($qid, _, 1).

$initialise :-
	catch(initialise_prolog, E,
	      (print_message(error, initialization_exception(E)),
	       fail
	      )).

initialise_prolog :-
	$clean_history,
	$set_file_search_paths,
	$run_at_initialization,
	$load_system_init_file,
	$load_gnu_emacs_interface,
	$option(init_file, File, File), 
	$load_init_file(File), 
	$option(goal, GoalAtom, GoalAtom), 
	term_to_atom(Goal, GoalAtom), 
	(   Goal == $welcome
	->  flag($banner_goal, TheGoal, TheGoal)
	;   TheGoal = Goal
	),
	ignore(user:TheGoal),
	$load_associated_file.

$abort :-
	see(user), 
	tell(user), 
	flag($break_level, _, 0), 
	flag($compilation_level, _, 0),
	$calleventhook(abort),
	print_message(informational, '$aborted'),
	$toplevel.

$break :-
	flag($break_level, Old, Old+1), 
	flag($break_level, New, New), 
	print_message(informational, break(enter(New))),
	$runtoplevel,
	print_message(informational, break(exit(New))),
	flag($break_level, _, Old), !.

:- $hide($toplevel, 0).			% avoid in the GUI stacktrace

$toplevel :-
	$runtoplevel,
	print_message(informational, halt).

$runtoplevel :-
	$option(toplevel, TopLevelAtom, TopLevelAtom), 
	term_to_atom(TopLevel, TopLevelAtom), 
	user:TopLevel.

%	$compile
%	Toplevel called when invoked with -c option.

$compile :-
	$set_file_search_paths,
	catch($compile_wic, E, (print_message(error, E), halt(1))).


		/********************************
		*    USER INTERACTIVE LOOP      *
		*********************************/

prolog :-
	flag($tracing, _, off), 
	flag($break_level, BreakLev, BreakLev), 
	repeat, 
	    (   $module(TypeIn, TypeIn), 
		$system_prompt(TypeIn, BreakLev, Prompt),
		prompt(Old, '|    '), 
		trim_stacks,
		read_query(Prompt, Goal, Bindings),
		prompt(_, Old),
		call_expand_query(Goal, ExpandedGoal,
				  Bindings, ExpandedBindings)
	    ->  $execute(ExpandedGoal, ExpandedBindings)
	    ), !.


read_query(Prompt, Goal, Bindings) :-
	current_prolog_flag(history, N),
	integer(N),
	N =< 0, !,
	remove_history_prompt(Prompt, Prompt1),
	repeat,				% over syntax errors
	prompt1(Prompt1),
	(   current_prolog_flag(readline, true)
	->  catch($raw_read(user_input, Line), E,
		  (print_message(error, E),
		   (   E = error(syntax_error(_), _)
		   ->  fail
		   ;   throw(E)
		   ))),
	    atom_codes(Line, LineChars),
	    append(LineChars, ".", CompleteLine),
	    catch(user:rl_add_history(CompleteLine), _, true),
	    catch(atom_to_term(Line, Goal, Bindings), E,
		  (   print_message(error, E),
		      fail
		  ))
	;   read_term(user_input, Goal, [variable_names(Bindings)])
	), !.
read_query(Prompt, Goal, Bindings) :-
	seeing(Old), see(user_input),
	(   read_history(h, '!h', 
			 [trace, end_of_file], 
			 Prompt, Goal, Bindings)
	->  see(Old)
	;   see(Old),
	    fail
	).

remove_history_prompt(Prompt0, Prompt) :-
	atom_chars(Prompt0, Chars0),
	clean_history_prompt_chars(Chars0, Chars1),
	delete_leading_blanks(Chars1, Chars),
	atom_chars(Prompt, Chars).

clean_history_prompt_chars([], []).
clean_history_prompt_chars(['%', !|T], T) :- !.
clean_history_prompt_chars([H|T0], [H|T]) :-
	clean_history_prompt_chars(T0, T).
 
delete_leading_blanks([' '|T0], T) :- !,
	delete_leading_blanks(T0, T).
delete_leading_blanks(L, L).


set_default_history :-
	(   current_prolog_flag(readline, true)
	->  set_prolog_flag(history, 0)
	;   set_prolog_flag(history, 15)
	).

:- initialization set_default_history.


		/********************************
		*            PROMPTING		*
		********************************/

:- dynamic
	$prompt/1.

$prompt("%m%d%l%! ?- ").

$set_prompt(P) :-
	atom_codes(P, S),
	retractall($prompt(_)),
	assert($prompt(S)).


$system_prompt(Module, BrekLev, Prompt) :-
	$prompt(P0),
	(    Module \== user
	->   $substitute("%m", [Module, ": "], P0, P1)
	;    $substitute("%m", [], P0, P1)
	),
	(    BrekLev \== 0
	->   $substitute("%l", ["[", BrekLev, "] "], P1, P2)
	;    $substitute("%l", [], P1, P2)
	),
	(    tracing
	->   $substitute("%d", ["[trace] "], P2, P3)
	;    $debugging
	->   $substitute("%d", ["[debug] "], P2, P3)
	;    $substitute("%d", [], P2, P3)
	),
	atom_chars(Prompt, P3).
	
$substitute(From, T, Old, New) :-
	convert_to(T, T0),
	flatten(T0, To),
	append(Pre, S0, Old),
	append(From, Post, S0) ->
	append(Pre, To, S1),
	append(S1, Post, New), !.
$substitute(_, _, Old, Old).
	
convert_to([], []).
convert_to([A|T], [S|R]) :-
	atomic(A), !,
	name(A, S),
	convert_to(T, R).
convert_to([S|T], [S|R]) :-
	convert_to(T, R).

		/********************************
		*           EXECUTION		*
		********************************/

$execute(Var, _) :-
	var(Var), !,
	print_message(informational, var_query(Var)),
	fail.
$execute(end_of_file, _) :- !.
$execute(Goal, Bindings) :-
	$module(TypeIn, TypeIn), 
	TypeIn:$dwim_correct_goal(Goal, Bindings, Corrected), !, 
	$execute_goal(Corrected, Bindings).
$execute(_, _) :-
	notrace, 
	print_message(query, query(no)),
	fail.

$execute_goal(trace, []) :-
	trace, 
	print_message(query, query(yes)), !,
	fail.
$execute_goal(Goal, Bindings) :-
	$module(TypeIn, TypeIn), 
	flag($qid, Qid, Qid+1),
	TypeIn:asserta(($user_query(Qid, Bindings) :- Goal), Ref),
	$set_user_goal_attributes(TypeIn),
	(   TypeIn:$user_query(Qid, Bindings),
	    flush,
	    call_expand_answer(Bindings, NewBindings),
	    (	write_bindings(NewBindings)
	    ->	!,
	        notrace,
		erase(Ref),
		fail
	    )
	;   notrace, 
	    print_message(query, query(no)),
	    erase(Ref),
	    fail
	).

$set_user_goal_attributes(TypeIn) :-
	TypeIn:(($hide($user_query, 2),
		 $show_childs($user_query, 2))).

write_bindings([]) :- !, 
	print_message(query, query(yes)).
write_bindings(Bindings) :-
	repeat,
	    print_message(query, query(yes, Bindings)),
	    get_respons(Action),
	(   Action == redo
	->  !, fail
	;   Action == show_again
	->  fail
	;   !,
	    print_message(query, query(yes))
	).

:- flag($toplevel_print_predicate, _, print).

get_respons(Action) :-
	repeat,
	    ttyflush,
	    get_single_char(Char),
	    answer_respons(Char, Action),
	    (   Action == again
	    ->  print_message(query, query(action)),
		fail
	    ;   !
	    ).

answer_respons(Char, again) :-
	memberchk(Char, "?h"), !,
	print_message(help, query(help)).
answer_respons(Char, redo) :-
	memberchk(Char, ";nrNR"), !,
	print_message(query, if_tty(';')).
answer_respons(Char, redo) :-
	memberchk(Char, "tT"), !,
	trace,
	print_message(query, if_tty('; [trace]')).
answer_respons(Char, continue) :-
	memberchk(Char, [0'c, 0'a, 0' , 10, 13, 0'y, 0'Y]), !.
answer_respons(0'b, show_again) :- !,
	break.
answer_respons(Char, show_again) :-
	print_predicate(Char, Pred, Format), !,
	print_message(query, if_tty(Pred)),
	set_prolog_flag(answer_format, Format).
answer_respons(-1, show_again) :- !,
	print_message(query, halt('EOF')),
	halt(0).
answer_respons(Char, again) :-
	print_message(query, no_action(Char)).

print_predicate(0'd, [quoted], '~q').
print_predicate(0'q, [quoted], '~q').
print_predicate(0'w, [write],  '~w').
print_predicate(0'p, [print],  '~p').


		 /*******************************
		 *	    EXPANSION		*
		 *******************************/

:- user:dynamic(expand_query/4).
:- user:multifile(expand_query/4).

call_expand_query(Goal, Expanded, Bindings, ExpandedBindings) :-
	user:expand_query(Goal, Expanded, Bindings, ExpandedBindings), !.
call_expand_query(Goal, Goal, Bindings, Bindings).


:- user:dynamic(expand_answer/2).
:- user:multifile(expand_answer/2).

call_expand_answer(Goal, Expanded) :-
	user:expand_answer(Goal, Expanded), !.
call_expand_answer(Goal, Goal).

