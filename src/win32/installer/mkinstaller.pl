/*  $Id$

    Part of SWI-Prolog

    Author:        Jan Wielemaker and Anjo Anjewierden
    E-mail:        jan@swi.psy.uva.nl
    WWW:           http://www.swi-prolog.org
    Copyright (C): 1985-2002, University of Amsterdam

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

name :-
	version(Major, Minor, Patch),
	format('Name "SWI-Prolog ~w.~w.~w"~n',
	       [Major, Minor, Patch]).

outfile :-
	version(Major, Minor, Patch),
	format('OutFile "w32pl~w~w~w.exe"~n',
	       [Major, Minor, Patch]).


version(Major, Minor, Patch) :-
	current_prolog_flag(version, V),
	Major is V//10000,
	Minor is V//100 mod 100,
	Patch is V mod 100.

run :-
	tell('version.nsi'),
	name,
	outfile,
	told.

:- run,halt.
:- halt(1).
	
