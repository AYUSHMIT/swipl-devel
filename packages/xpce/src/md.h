/*  $Id$

    Part of XPCE

    Author:  Jan Wielemaker and Anjo Anjewierden
    E-mail:  jan@swi.psy.uva.nl
    WWW:     http://www.swi.psy.uva.nl/projects/xpce/
    Copying: GPL-2.  See the file COPYING or http://www.gnu.org

    Copyright (C) 1990-2001 SWI, University of Amsterdam. All rights reserved.
*/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
autoconf/config.h based machine-binding file.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef _MD_INCLUDED
#define _MD_INCLUDED

#if defined(WIN32) || defined(__WIN32__)
# include "md/md-win32.h"
#else
# ifdef __WINDOWS__
# include "md/md-mswin.h"
# else
# include <config.h>
# endif
#endif

		 /*******************************
		 *	    DEFINE UNIX?	*
		 *******************************/

#if !defined(__unix__) && defined(_AIX)
#define __unix__ 1
#endif

		 /*******************************
		 *	      ALLOCA		*
		 *******************************/

/* AIX requires this to be the first thing in the file.  */
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not __GNUC__ */
#if HAVE_ALLOCA_H
#include <alloca.h>
#else /* not HAVE_ALLOCA_H */
#ifdef _AIX
#pragma alloca
#else /* not _AIX */
char *alloca ();
#endif /* not _AIX */
#endif /* not HAVE_ALLOCA_H */
#endif /* not __GNUC__ */


		 /*******************************
		 *          STDC_HEADERS	*
		 *******************************/

#if STDC_HEADERS || HAVE_STRING_H
#include <string.h>
/* An ANSI string.h and pre-ANSI memory.h might conflict.  */
#if !STDC_HEADERS && HAVE_MEMORY_H
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
/* memory.h and strings.h conflict on some systems.  */
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifndef HAVE_MEMMOVE
#define memmove(to, from, size)	bcopy(from, to, size)
#endif

		 /*******************************
		 *	SOME SYSTEM STUFF	*
		 *******************************/

#endif /*_MD_INCLUDED*/
