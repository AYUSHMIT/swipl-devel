/*  $Id$

    Copyright (c) 1990 Jan Wielemaker. All rights reserved.
    See ../LICENCE to find out about your rights.
    jan@swi.psy.uva.nl

    Purpose: Windows (NT) specific stuff
*/

#if defined(__WINDOWS__) || defined(__WIN32__)

#include <windows.h>
#include <process.h>
#undef TRANSPARENT
#include "pl-incl.h"
#include <stdio.h>
#include <stdarg.h>
#include "pl-stream.h"
#include <console.h>
#include <process.h>

word
pl_window_title(Word old, Word new)
{ char buf[256];

  if ( !isAtom(*new) )
    return warning("window_title/2: instantiation fault");

  rlc_title(stringAtom(*new), buf, sizeof(buf));

  return unifyAtomic(old, lookupAtom(buf));
}


void
PlMessage(const char *fm, ...)
{ va_list(args);
  char buf[1024];

  va_start(args, fm);
  vsprintf(buf, fm, args);
  va_end(args);

  MessageBox(NULL, buf, "SWI-Prolog", MB_OK|MB_TASKMODAL);
}

		 /*******************************
		 *	WinAPI ERROR CODES	*
		 *******************************/

char *
WinError()
{ int id = GetLastError();
  char *msg;

  if ( FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|
		     FORMAT_MESSAGE_IGNORE_INSERTS|
		     FORMAT_MESSAGE_FROM_SYSTEM,
		     NULL,			/* source */
		     id,			/* identifier */
		     LANG_ENGLISH,
		     (LPTSTR) &msg,
		     0,				/* size */
		     NULL) )			/* arguments */
  { Atom a = lookupAtom(msg);

    LocalFree(msg);

    return stringAtom(a);
  } else
    return "Unknown Windows error";
}


		 /*******************************
		 *	  SLEEP/1 SUPPORT	*
		 *******************************/

void
Pause(double t)
{ DWORD msecs = (DWORD)(t * 1000.0);

  Sleep(msecs);
}

		 /*******************************
		 *     SUPPORT FOR SHELL/2	*
		 *******************************/

static int shell_finished;
static int shell_rval;

static void
waitthread(void *handle)
{ 
#if 0
					/* works fine, but how to get */
					/* exit status? */
  WaitForSingleObject(handle, INFINITE);
  shell_rval = 0;
#else
  if ( _cwait(&shell_rval, (int)handle, _WAIT_CHILD) < 0 )
    warning("_cwait() failed: %s\n", OsError());
#endif
  shell_finished = TRUE;
}

int
System(char *command)
{ STARTUPINFO sinfo;
  PROCESS_INFORMATION pinfo;

  memset(&sinfo, 0, sizeof(sinfo));
  sinfo.cb = sizeof(sinfo);

  if ( CreateProcess(NULL,			/* module */
		     command,			/* command line */
		     NULL,			/* Security stuff */
		     NULL,			/* Thread security stuff */
		     FALSE,			/* Inherit handles */
		     NORMAL_PRIORITY_CLASS,	/* flags */
		     NULL,			/* environment */
		     NULL,			/* CWD */
		     &sinfo,			/* startup info */
		     &pinfo) )			/* process into */
  { shell_finished = FALSE;
    if ( _beginthread(waitthread, 10240, pinfo.hProcess) < 0 )
    { warning("Failed to create wait-thread: %s", OsError());
      return -1;
    }
    while ( !shell_finished )
      rlc_dispatch(NULL);

    return shell_rval;
  } else
  { warning("shell('%s') failed: %d\n", command, WinError());
    return -1;
  }
}

		 /*******************************
		 *	BIND STREAM STUFF	*
		 *******************************/

static int
Srlc_read(void *handle, char *buffer, int size)
{ return rlc_read(buffer, size);
}


static int
Srlc_write(void *handle, char *buffer, int size)
{ return rlc_write(buffer, size);
}


static void
rlc_bind_terminal()
{ static IOFUNCTIONS funcs;

  funcs = *Sinput->functions;
  funcs.read  = Srlc_read;
  funcs.write = Srlc_write;

  Sinput->functions  = &funcs;
  Soutput->functions = &funcs;
  Serror->functions  = &funcs;
}


		 /*******************************
		 *	       MAIN		*
		 *******************************/

extern int main(int argc, char **argv);

int PASCAL
WinMain(HANDLE hInstance, HANDLE hPrevInstance,
	LPSTR lpszCmdLine, int nCmdShow)
{ rlc_bind_terminal();

  return rlc_main(hInstance, hPrevInstance, lpszCmdLine, nCmdShow,
		  main, LoadIcon(hInstance, "SWI_Icon"));
}

#endif /*__WINDOWS__*/


