/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1997 University of Amsterdam. All rights reserved.
*/

:- module(pce_colour_item, []).
:- use_module(library(pce)).

default_palette_colour(red).
default_palette_colour(darkorange).
default_palette_colour(blue).
default_palette_colour(navy).
default_palette_colour(green).
default_palette_colour(yellow).
default_palette_colour(navajowhite).
default_palette_colour(brown).
default_palette_colour(white).
default_palette_colour(black).
default_palette_colour(grey80).
default_palette_colour(grey50).

:- pce_begin_class(colour_item, dialog_group,
		   "Item for selecting a colour").

variable(message,	code*,        both, "Executed message").
variable(modified,	bool := @off, both, "Item was modified").

initialise(CI,
	   Name:[name], Selection:[colour], Msg:[code]*,
	   Palette:[chain]) :->
	(   Palette == @default
	->  make_default_palette(Palette1)
	;   Palette1 = Palette
	),
	default(Msg, @nil, Msg1),
	send(CI, slot, message, Msg1),
	send(CI, send_super, initialise, Name, box),
	send(CI, append, new(PM, menu(palette, choice,
				      message(CI, user_selection, @arg1)))),
	send(PM, layout, horizontal),
	send(PM, show_label, @off),
	send(PM, alignment, left),
	send(CI, append, new(RG, dialog_group(right_group, group)), right),
	send_list([PM, RG], reference, point(0,0)),
	send(RG, gap, size(5,5)),
	send(RG, append, box(50,50)),
	send(RG, append, new(Action, button(palette_button))),
	send(Action, label, image('16x16/cpalette1.xpm')),
	send(CI, palette, Palette1),
	send(CI, append, new(R, slider(red,   0, 255, 128))),
	send(CI, append, new(G, slider(green, 0, 255, 128))),
	send(CI, append, new(B, slider(blue,  0, 255, 128))),
	send(CI, append,
	     new(NI, colour_name_item(name, @default,
				      message(CI, user_selection, @arg1)))),
	send(NI, show_label, @off),
	init_slider(R),
	init_slider(G),
	init_slider(B),
	(   Selection \== @default
	->  send(CI, selection, Selection)
	;   send(CI, select_first)
	).


make_default_palette(Palette) :-
	new(Palette, chain),
	forall(default_palette_colour(Colour), send(Palette, append, Colour)).


show_label(CI, Val:bool) :->		% To dialog_group?
	(   Val == @on
	->  send(CI, label, ?(CI, label_name, name))
	;   send(CI, label, '')
	).

proto_box(CI, Box:box) :<-
	"Box used for feedback on selection"::
	get(CI, member, right_group, RG),
	get(RG, member, box, Box).

:- pce_group(selection).

colour_selection(CI, Colour:colour) :->
	"Set the current selection"::
	get(CI, proto_box, Box),
	send(Box, fill_pattern, Colour),
	set_slider(CI, Colour, red),
	set_slider(CI, Colour, green),
	set_slider(CI, Colour, blue),
	get(CI, member, name, NI),
	send(NI, selection, Colour),
	get(CI, member, palette, Palette),
	get(CI, member, right_group, RG),
	get(RG, member, palette_button, AB),
	(   get(Palette?members, find,
		message(Colour, equal, @arg1?value),
		Item)
	->  send(Palette, selection, Item),
	    send(AB, message,
		 message(CI, delete_palette_colour, Box?fill_pattern)),
	    send(AB, label, image('16x16/trashcan.xpm'))
	;   send(Palette, clear_selection),
	    send(AB, message,
		 message(CI, add_palette_colour, Box?fill_pattern)),
	    send(AB, label, image('16x16/cpalette1.xpm'))
	),
	send(CI, modified, @off).

colour_selection(CI, Colour:colour) :<-
	"Get the current selection"::
	get(CI, proto_box, Box),
	get(Box, fill_pattern, Colour),
	send(CI, modified, @off).

selection(CI, Colour:colour) :->
	send(CI, colour_selection, Colour).
selection(CI, Colour:colour) :<-
	get(CI, colour_selection, Colour).

user_selection(CI, Colour:colour) :->
	"User selected a colour"::
	send(CI, colour_selection, Colour),
	send(CI, modified, @on),
	(   get(CI, device, Dev),
	    send(Dev, has_send_method, modified_item)
	->  send(CI?device, modified_item, CI, @on)
	;   true
	).

clear(_CI) :->
	"Clear selection"::
	true.

select_first(CI) :->
	"Select first in palette"::
	get(CI, member, palette, Menu),
	(   get(Menu?members, head, First)
	->  send(CI, colour_selection, First?value)
	;   true			% no colours in palette
	).

:- pce_group(apply).

apply(SE, Always:[bool]) :->
	"Forward <-selection over <-message"::
	(   (   Always == @on
	    ;	get(SE, modified, @on)
	    ),
	    get(SE, message, Msg),
	    Msg \== @nil
	->  get(SE, selection, Value),
	    ignore(send(Msg, forward, Value))
	;   true
	).

	
		 /*******************************
		 *	       SLIDERS		*
		 *******************************/

init_slider(Slider) :-
	get(Slider, name, Colour),
	new(I, image(@nil, 16, 16, pixmap)),
	send(I, background, Colour),
	send(I, clear),
	send(Slider, label, I),
	send(Slider, width, 100),
	send(Slider, drag, @on),
	send(Slider, attribute, hor_stretch, 100),
	get(Slider, device, CI),
	send(Slider, show_value, @off),
	send(Slider, message, message(CI, slider_dragged)).

set_slider(CI, Colour, SliderName) :-
	get(CI, member, SliderName, Slider),
	get(Colour, SliderName, Value),
	Value256 is Value // 256,
	send(Slider, selection, Value256).

slider_dragged(CI) :->
	current_slider_value(CI, red,   R),
	current_slider_value(CI, green, G),
	current_slider_value(CI, blue,  B),
	send(CI, user_selection, colour(@default, R, G, B)).

current_slider_value(CI, Colour, Value) :-
	get(CI, member, Colour, Slider),
	get(Slider, selection, Value256),
	Value is Value256 * 257.


		 /*******************************
		 *	       PALETTE		*
		 *******************************/

palette(CI, Palette:chain) :->
	"Fill the palette menu"::
	get(CI, member, palette, Menu),
	send(Menu, clear),
	send(Palette, for_all, message(CI, add_palette_colour, @arg1, @off)),
	send(CI, adjust_palette_size).
palette(CI, Palette:chain) :<-
	"Fetch current palette as chain of colours"::
	get(CI, member, palette, Menu),
	get(Menu?members, map, @arg1?value, Palette).


add_palette_colour(CI, Colour:colour, Adjust:[bool]) :->
	"Add colour to the palette"::
	get(CI, member, palette, Menu),
	send(Menu, append, menu_item(Colour)),
	(   Adjust \== @off
	->  send(CI, adjust_palette_size),
	    send(CI, user_selection, Colour)
	;   true
	).

delete_palette_colour(CI, Colour:colour, Adjust:[bool]) :->
	"Add colour to the palette"::
	get(CI, member, palette, Palette),
	get(Palette?members, find,
	    message(Colour, equal, @arg1?value),
	    Item),
	send(Palette, delete, Item),
	(   Adjust \== @off
	->  send(CI, adjust_palette_size),
	    send(CI, user_selection, Palette?selection)
	;   true
	).

adjust_palette_size(CI) :->
	get(CI, member, palette, Menu),
	get(Menu?members, size, Entries),
	(   Entries == 0
	->  true
	;   palette_dimensions(Entries, NW, NH),
	    send(Menu, columns, NH),
	    IW is 125 // NW,
	    IH is 100 // NH,
	    send(Menu?members, for_all,
		 message(@prolog, resize_colour_item, @arg1, IW, IH))
	).

resize_colour_item(Item, IW, IH) :-
	get(Item, value, Colour),
	new(I, image(@nil, IW, IH, pixmap)),
	send(I, background, Colour),
	send(I, clear),
	send(Item, label, I).


%	palette_dimensions(+Entries, -Width, -Height)
%
%	Attempts to find a nice 2-dimensional layout for `Entries' cells
%	in a total size of `Width' x `Height'.

palette_dimensions(Entries, Width, Height) :- % perfect divisors
	findall(D, divisor(Entries, D), Ds),
	Ds \== [],
	best_divisor(Ds, Entries, 1000/_, OK/Height),
	OK < 0.4,
	Width is Entries / Height.
palette_dimensions(Entries, Width, Height) :-
	Width is integer(sqrt(Entries)*1.1),
	Height is (Entries+Width-1)//Width.

best_divisor([H|T], Entries, Ok0/I0, R) :-
	W is Entries/H,
	Ok1 is abs(H/W-5/4),
	(   Ok1 < Ok0
	->  best_divisor(T, Entries, Ok1/H, R)
	;   best_divisor(T, Entries, Ok0/I0, R)
	).

divisor(N, D) :-
	Max is ceil(sqrt(N)),
	between(1, Max, D),
	N mod D =:= 0.

:- pce_end_class.


		 /*******************************
		 *	    COLOUR NAMES	*
		 *******************************/

colour_bits(3).

:- dynamic
	user:file_search_path/2.
:- multifile
	user:file_search_path/2.

user:file_search_path(x11, OpenWin) :-
	get(@pce, environment_variable, 'OPENWINHOME', OpenWin).
user:file_search_path(x11, '/usr/lib/X11').
user:file_search_path(x11, PceLib) :-
	get(@pce, window_system, windows),
	get(@pce, home, PceHome),
	concat(PceHome, '/lib', PceLib).


:- pce_global(@colour_table,     make_colour_table).
:- pce_global(@colour_name_list, make_colour_name_list).
:- pce_global(@rgb_table,        make_rgb_table).

make_colour_table(DB) :-
	new(DB, hash_table),
	absolute_file_name(x11(rgb),
			   [ extensions([txt]),
			     access(read)
			   ], DataBase),
	new(F, file(DataBase)),
	(   send(F, open, read)
	->  repeat,
	    (	get(F, read_line, String)
	    ->  get(String, scan, '%d%d%d%*[ 	]%[a-zA-Z0-9_ ]',
		    vector(R, G, B, Name)),
		send(Name, translate, ' ', '_'),
		send(Name, downcase),
		get(Name, value, Atom),
		RGB is R<<16 + G<<8 + B,
		(   get(DB, member, Atom, _)
		->  true
		;   send(DB, append, Atom, RGB)
		),
		fail
	    ;	!,
	        send(F, close)
	    )
	;   send(@nil, report, error,
		 'Cannot read colour database %s', DataBase)
	).

make_colour_name_list(N) :-
	new(N, chain),
	send(@colour_table, for_all, message(N, append, @arg1)),
	send(N, sort).			% ???
	
make_rgb_table(DB) :-
	new(DB, hash_table),
	send(@colour_table, for_all,
	     message(DB, append,
		     ?(@prolog, quant_rgb, @arg2),
		     @arg1)).

quant_rgb(RGB0, RGB) :-
	colour_bits(Bits),
	EBits is 8-Bits,
	Mask is (1<<Bits)-1,
	Mask8 is Mask << EBits,
	RGB is (((RGB0>>16)/\Mask8)>>EBits)<<(2*Bits) +
	       (((RGB0>>8)/\Mask8)>>EBits)<<Bits +
	       ((RGB0/\Mask8)>>EBits).

:- pce_begin_class(colour_name_item, text_item,
		   "Completing item for colour-names").

initialise(NI, Name:[name], Selection:[colour], Msg:[code]*) :->
	(   Selection == @default
	->  CName = ''
	;   get(Selection, print_name, CName)
	),
	default(Name, colour, TheName),
	send(NI, send_super, initialise, TheName, Selection, Msg),
	send(NI, value_set, @colour_name_list).

selection(NI, Selection:colour) :<-
	"Get selection as a colour object"::
	get(NI, get_super, selection, ColourName),
	get(@pce, convert, ColourName, colour, Selection).

selection(NI, Selection:colour) :->
	"Display named colour"::
	standardise_colour_name(Selection, TheName),
	send(NI, send_super, selection, TheName).

standardise_colour_name(Colour, XName) :-
	get(Colour, name, In),
	get(In, character, 0, 0'#),
	get(Colour, red, R),
	get(Colour, green, G),
	get(Colour, blue, B),
	colour_bits(Bits),
	Mask is (1<<Bits)-1,
	EBits is 16-Bits,
	Mask16 is Mask << EBits,
	RGB is (((R/\Mask16)>>EBits)<<(2*Bits)) +
	       (((G/\Mask16)>>EBits)<<Bits) +
	       ((B/\Mask16)>>EBits),
	get(@rgb_table, member, RGB, XName), !.
standardise_colour_name(Colour, Name) :-
	get(Colour, name, Name).
	

:- pce_end_class.

:- pce_begin_class(colour_palette_item, colour_item,
		   "Editor for a colour-palette").

initialise(PI, Name:[name], Selection:[chain], Msg:[code]*) :->
	send(PI, send_super, initialise, Name, @default, Msg, Selection).

selection(PI, Selection:chain) :->
	send(PI, palette, Selection),
	send(PI, modified, @off).
selection(PI, Selection:chain) :<-
	get(PI, palette, Selection),
	send(PI, modified, @off).

:- pce_end_class.


test :-
	new(D, dialog),
	send(D, append, colour_item(colour, red)),
	send(D, open).
