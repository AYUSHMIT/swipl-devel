/*  $Id$

    Copyright (c) 1992 Jan Wielemaker/Pieter Olivier. All rights reserved.
    See ../LICENCE to find out about your rights.
    jan@swi.psy.uva.nl

    Purpose: Machine description for Linux
*/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
The   port to LINUX   was made  by   Pieter  Olivier.   It   has  been
incorperated  into the most  recent   version  (1.5.5) of  the  common
sources, but not tested afterwards.

Updated version 1.6.8 after an important fix sent to me by Peter Barth
(barth@mpi-sb.mpg.de)   and   minor   fixes    from    Philip   Perucc  
(dsc3pzp@nmrdc1.nmrdc.nnmc.navy.mil).  LINUX versions:

	gcc 2.3.3, libc 4.2, Linux 0.99pl2:

Updated version  1.6.14 by  Jan Wielemaker  (got  my own  now).  Fixed 
save_program/1 and included dynamic stacks.  Versions:

	gcc 2.3.3, libc 4.3, Linux 0.99pl7:

Fixed load_foreign/[1,2] and dropped -static from the LDFLAGS and this
appears not to be necessary with linux.

Wed Jun 30 22:03:41 1993
    Updated version 1.7.0.  Now requires the readline library available
    from various ftp sites.  SWI-Prolog uses both termcap and readline.
    The current version (libc 4.4, libreadline.so.1.1) has a bug that
    results in multiple symbol declaration when SWI-Prolog is linked.
    As a temporary fix remove /usr/lib/libtermcap.sa while linking
    SWI-Prolog.

Thu Mar 10 22:14:20 1994
    Updated version 1.8.9. The stdio file structure elements were
    changed with libc 4.5.8 and above. The linux stdio now uses the
    libio structures. The old (and new) versions still don't work
    the way RESET_STDIN is meant to work. There seems to be no portable
    way to handle flushing all to-be-input characters when doing a save/1.
    Kayvan Sylvan (kayvan@Sylvan.COM) also (kayvan@Quintus.COM).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define M_CC			gcc
#define M_OPTIMIZE	        -O2
/*#define M_OPTIMIZE		-g*/
#define M_LDFLAGS		-static
/*#define M_CFLAGS		-ansi -pedantic -Wall -funsigned-char*/
#define M_CFLAGS		-Wall -funsigned-char
#define M_LIBS			-lm -ltermcap -lreadline

			/* compiler */
#define linux			1	/* -ansi provides __linux__ */
#define ANSI			1
#define USG			1
#define O_NO_VOID_POINTER	0
#define O_SHORT_SYMBOLS		0
#define O_UCHAR_PREDEFINED	0	/* type uchar is predefined */
#define _POSIX_SOURCE		1

			/* Operating system */
#define O_PROFILE		1
#define O_SIG_AUTO_RESET	1
#define O_SHARED_MEMORY		0
#define O_CAN_MAP		1
#define O_SHIFT_STACKS		0
#define O_NO_SEGV_ADDRESS	1
#define MAX_VIRTUAL_ADDRESS     (220*1024*1024) /* not sure, but it will do */
#define O_FOREIGN		1
#define LD_COMMAND		"gcc"
#define LD_OPT_OPTIONS		""
#define LD_OPT_ADDR		"-T 0x%x"
#define O_SAVE			1
#define DATA_START		((long) &etext + sizeof(long))
/*#define FIRST_DATA_SYMBOL	etext*/
#define DEFAULT_PATH		":.:/bin:/usr/bin:/usr/local/bin:";
#define SIGNAL_HANDLER_TYPE	void
#define O_STRUCT_DIRECT		0
#define DIR_INCLUDE		<sys/dir.h>
#define DIR_INCLUDE2		<dirent.h>
#define TERMIO_INCLUDE		<termio.h>
#define O_GETCWD		1

/* This is for the newer libc (4.5.8+) */
#ifdef _STDIO_USES_IOSTREAM
#define _gptr _IO_read_ptr
#define _egptr _IO_read_end
#endif

#include <linux/limits.h>

			/* terminal driver */
#define O_READLINE		1
#define O_TERMIOS 		1
#define O_FOLD 			0

			/* Interfaces */
#define O_PCE 			1

#define MACHINE			"i386"
#define OPERATING_SYSTEM  	"linux"


		/********************************
		*      COMPATIBILITY MACROS	*
		********************************/

#define bzero(t, l)	memset(t, 0, l)
#define vfork()		fork()
