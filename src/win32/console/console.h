/*  $Id$

    Part of SWI-Prolog

    Author:        Jan Wielemaker
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

#ifndef _CONSOLE_H_INCLUDED
#define _CONSOLE_H_INCLUDED

#ifndef RLC_VENDOR
#define RLC_VENDOR "SWI"
#endif

#ifndef _export
#ifdef _MAKE_DLL
#define _export _declspec(dllexport)
#else
#define _export extern
#endif
#endif

#include <signal.h>

#define RLC_APPTIMER_ID	100		/* >=100: application timer */

typedef struct
{ int	 	 first;
  int	 	 last;
  int	 	 size;			/* size of the buffer */
  unsigned char *buffer;		/* character buffer */
  int		 flags;			/* flags for the queue */
} rlc_queue, *RlcQueue;

#define RLC_EOF	0x1			/* Flags on the queue */

typedef struct
{ int		mark_x;
  int		mark_y;
} rlc_mark, *RlcMark;

typedef void	(*RlcUpdateHook)(void);	/* Graphics update hook */
typedef void	(*RlcTimerHook)(int);	/* Timer fireing hook */
typedef int	(*RlcRenderHook)(int);	/* Render one format */
typedef void	(*RlcRenderAllHook)(void); /* Render all formats */
typedef int	(*RlcMain)(int, char**); /* the main() function */
typedef void	(*RlcInterruptHook)(int); /* Hook for Control-C */
typedef void	(*RlcResizeHook)(int, int); /* Hook for window change */
typedef void	(*RlcMenuHook)(const char *id); /* Hook for menu-selection */

#ifdef _WINDOWS_			/* <windows.h> is included */
					/* rlc_color(which, ...) */
#define RLC_WINDOW	  (0)		/* window background */
#define RLC_TEXT	  (1)		/* text color */
#define RLC_HIGHLIGHT	  (2)		/* selected text background */
#define RLC_HIGHLIGHTTEXT (3)		/* selected text */

_export HANDLE	rlc_hinstance(void);	/* hInstance of WinMain() */
_export HWND	rlc_hwnd(void);		/* HWND of console window */
_export int	rlc_main(HANDLE hI, HANDLE hPrevI,
			 LPSTR cmd, int show, RlcMain main, HICON icon);
_export void	rlc_icon(HICON icon);	/* Change icon of main window */
_export COLORREF rlc_color(int which, COLORREF color);
#endif /*_WINDOWS_*/

_export RlcUpdateHook	rlc_update_hook(RlcUpdateHook updatehook);
_export RlcTimerHook	rlc_timer_hook(RlcTimerHook timerhook);
_export RlcRenderHook   rlc_render_hook(RlcRenderHook renderhook);
_export RlcRenderAllHook rlc_render_all_hook(RlcRenderAllHook renderallhook);
_export RlcInterruptHook rlc_interrupt_hook(RlcInterruptHook interrupthook);
_export RlcResizeHook	rlc_resize_hook(RlcResizeHook resizehook);
_export RlcMenuHook	rlc_menu_hook(RlcMenuHook menuhook);
_export int		rlc_copy_output_to_debug_output(int docopy);

_export void		rlc_title(char *title, char *old, int size);
_export void		rlc_dispatch(RlcQueue q);
_export void		rlc_yield(void);
_export RlcQueue	rlc_input_queue(void); /* libs stdin queue */
_export RlcQueue	rlc_make_queue(int size);
_export void		rlc_free_queue(RlcQueue q);
_export int		rlc_from_queue(RlcQueue q);
_export int		rlc_is_empty_queue(RlcQueue q);
_export void		rlc_empty_queue(RlcQueue q);
_export void		rlc_word_char(int chr, int isword);
_export int		rlc_is_word_char(int chr);
_export int		rlc_iswin32s(void);	/* check for Win32S */

_export void		rlc_get_mark(RlcMark mark);
_export void		rlc_goto_mark(RlcMark mark, const char *data, int offset);
_export void		rlc_erase_from_caret();
_export void		rlc_putchar(int chr);
_export char *		rlc_read_screen(RlcMark from, RlcMark to);
_export void		rlc_update(void);

_export void		rlc_free(void *ptr);
_export void *		rlc_malloc(int size);
_export void *		rlc_realloc(void *ptr, int size);

_export int		rlc_read(char *buf, int count);
_export int		rlc_write(char *buf, unsigned int count);
_export int		rlc_close(void);

_export int		getch(void);
_export int		getche(void);
_export int		getkey(void);
_export int		kbhit(void);
_export void		ScreenInit(void);
_export void		ScreenGetCursor(int *row, int *col);
_export void		ScreenSetCursor(int row, int col);
_export int		ScreenCols(void);
_export int		ScreenRows(void);
_export const char *	rlc_prompt(const char *prompt);
_export void		rlc_clearprompt(void);

_export int		rlc_insert_menu_item(const char *menu,
					     const char *label,
					     const char *before);
_export int		rlc_insert_menu(const char *label,
					const char *before);


		 /*******************************
		 *	 LINE EDIT STUFF	*
		 *******************************/

typedef struct _line
{ rlc_mark	origin;			/* origin of edit */
  int   	point;			/* location of the caret */
  int		size;			/* # characters in buffer */
  int   	allocated;		/* # characters allocted */
  int		change_start;		/* start of change */
  int		complete;		/* line is completed */
  int		reprompt;		/* repeat the prompt */
  char	       *data;			/* the data (malloc'ed) */
} line, *Line;

#define COMPLETE_MAX_WORD_LEN 256
#define COMPLETE_MAX_MATCHES 100

#define COMPLETE_INIT	   0
#define COMPLETE_ENUMERATE 1
#define COMPLETE_CLOSE	   2

typedef int (*RlcCompleteFunc)(struct _complete_data *);

typedef struct _complete_data
{ Line		line;			/* line we are completing */
  int		call_type;		/* COMPLETE_* */
  int		replace_from;		/* index to start replacement */
  int		quote;			/* closing quote */
  int		case_insensitive;	/* if TRUE: insensitive match */
  char		candidate[COMPLETE_MAX_WORD_LEN];
  char		buf_handle[COMPLETE_MAX_WORD_LEN];
  RlcCompleteFunc function;		/* function for continuation */
  void	       *ptr_handle;		/* pointer handle for client */
  long		num_handle;		/* numeric handle for client */
} rlc_complete_data, *RlcCompleteData;

_export RlcCompleteFunc rlc_complete_hook(RlcCompleteFunc func);

_export char	*read_line(void);
_export int	rlc_complete_file_function(RlcCompleteData data);
_export void	rlc_init_history(int auto_add, int size);
_export void	rlc_add_history(const char *line);
_export int	rlc_bind(int chr, const char *fname);

#endif /* _CONSOLE_H_INCLUDED */
