/*  $Id$

    Copyright (c) 1990 Jan Wielemaker. All rights reserved.
    See ../LICENCE to find out about your rights.
    jan@swi.psy.uva.nl

    Purpose: Prologs main module
*/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Get the ball rolling.  The main task of  this  module  is  command  line
option  parsing,  initialisation  and  handling  of errors and warnings.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*#define O_DEBUG 1*/

#include "pl-incl.h"
#include "pl-save.h"
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

forwards void	usage(void);
static void	version();
static void	arch();

#define	optionString(s) { if (argc > 1) \
			  { s = argv[1]; argc--; argv++; \
			  } else \
			    usage(); \
			}

static char *
findHome(char *symbols, char *def)
{ char *home;
  char plp[MAXPATHLEN];

  if ( !(home = getenv("SWI_HOME_DIR")) )
    home = getenv("SWIPL");
  if ( home && (home = PrologPath(home, plp)) && ExistsDirectory(home) )
    return store_string(home);

  if ( (home = symbols) )
  { char buf[MAXPATHLEN];
    char parent[MAXPATHLEN];
    IOSTREAM *fd;

    strcpy(parent, DirName(DirName(AbsoluteFile(home))));
    Ssprintf(buf, "%s/swipl", parent);

    if ( (fd = Sopen_file(buf, "r")) )
    { if ( Sfgets(buf, sizeof(buf), fd) )
      { int l = strlen(buf);

	while(l > 0 && buf[l-1] <= ' ')
	  l--;
	buf[l] = EOS;

#if O_XOS
      { char buf2[MAXPATHLEN];
	_xos_canonical_filename(buf, buf2);
	strcpy(buf, buf2);
      }
#endif

	if ( !IsAbsolutePath(buf) )
	{ char buf2[MAXPATHLEN];

	  Ssprintf(buf2, "%s/%s", parent, buf);
	  home = AbsoluteFile(buf2);
	} else
	home = AbsoluteFile(buf);

	if ( ExistsDirectory(home) )
	{ Sclose(fd);
	  return store_string(home);
	}
      }
	Sclose(fd);
    }
  }

  if ( ExistsDirectory(def) )
    return def;

#if tos || __DOS__ || __WINDOWS__
#if tos
#define HasDrive(c) (Drvmap() & (1 << (c - 'a')))
#else
#define HasDrive(c) 1
#endif
  { char *drv;
    static char drvs[] = "cdefghijklmnopab";
    char home[MAXPATHLEN];

    for(drv = drvs; *drv; drv++)
    { Ssprintf(home, "/%c:/pl", *drv);
      if ( HasDrive(*drv) && ExistsDirectory(home) )
        return store_string(home);
    }
  }
#endif

  fatalError("Can't find Prolog Home Directory");
  return (char *) NULL;
}

/*
  -- atoenne -- convert state to an absolute path. This allows relative
  SWI_HOME_DIR and cleans up non-canonical paths.
*/

#ifndef IS_DIR_SEPARATOR
#define IS_DIR_SEPARATOR(c) ((c) == '/')
#endif

static char *
proposeStartupFile(char *symbols)
{ char state[MAXPATHLEN];

  if ( !symbols )
    if ( (symbols = Symbols()) )
      symbols = DeRefLink(symbols);

  if ( symbols )
  { char *s, *dot = NULL;

    strcpy(state, symbols);
    for(s=state; *s; s++)
    { if ( *s == '.' )
	dot = s;
      if ( IS_DIR_SEPARATOR(*s) )
	dot = NULL;
    }
    if ( dot )
      *dot = EOS;

    strcat(state, ".qlf");

    return store_string(state);
  }

  Ssprintf(state, "%s/startup/startup.%s",
	  systemDefaults.home, systemDefaults.arch);

  return store_string(AbsoluteFile(state));
}


static char *
findState(char *symbols)
{ char state[MAXPATHLEN];
  char *full;

  full = proposeStartupFile(symbols);
  if ( AccessFile(full, ACCESS_READ) )
    return full;

  Ssprintf(state, "%s/startup/startup.%s",
	  systemDefaults.home, systemDefaults.arch);
  full = AbsoluteFile(state);
  if ( AccessFile(full, ACCESS_READ) )
    return store_string(full);

  Ssprintf(state, "%s/startup/startup", systemDefaults.home);
  full = AbsoluteFile(state);
  if ( AccessFile(full, ACCESS_READ) )
    return store_string(full);

  return NULL;
}


static void
warnNoFile(char *file)
{ AccessFile(file, ACCESS_READ);	/* just to set errno */

  Sfprintf(Serror, "    no `%s': %s\n", file, OsError());
}


static void
warnNoState()
{ char state[MAXPATHLEN];
  char *full;

  Sfprintf(Serror, "[FATAL ERROR: Failed to find startup file\n");
  warnNoFile(proposeStartupFile(NULL));
  Ssprintf(state, "%s/startup/startup.%s",
	  systemDefaults.home, systemDefaults.arch);
  full = AbsoluteFile(state);
  warnNoFile(full);
  Ssprintf(state, "%s/startup/startup", systemDefaults.home);
  full = AbsoluteFile(state);
  warnNoFile(full);
  Sfprintf(Serror,
	  "\nUse\n\t`%s -o startup-file -b boot/init.pl -c boot/load.pl'\n",
	  mainArgv[0]);
  Sfprintf(Serror, "\nto create one]\n");

  Halt(1);
}


#if O_LINK_PCE
foreign_t
pl_pce_init()
{ prolog_pce_init(mainArgc, mainArgv);

  succeed;
}
#endif

int
startProlog(int argc, char **argv, char **env)
{ char *s;
  int n;
  char *state, *symbols;
  bool compile;
  bool explicit_state = FALSE;
  bool explicit_compile_out = FALSE;

  mainArgc			= argc;
  mainArgv			= argv;
  mainEnv			= env;

  DEBUG(1, Sdprintf("System compiled at %s %s\n", __TIME__, __DATE__));

#if O_MALLOC_DEBUG
  malloc_debug(O_MALLOC_DEBUG);
#endif

  status.debugLevel = 0;
  DEBUG(1, Sdprintf("OS ...\n"));
  initOs();				/* initialise OS bindings */

  if ( status.dumped == FALSE )
  { char plp[MAXPATHLEN];

    symbols = Symbols();
    if ( symbols ) 
      symbols = store_string(DeRefLink(symbols));

    systemDefaults.arch        = ARCH;
    systemDefaults.home	       = findHome(symbols,
					  store_string(PrologPath(PLHOME,plp)));
#ifdef O_XOS
  { char buf[MAXPATHLEN];
    _xos_limited_os_filename(systemDefaults.home, buf);
    systemDefaults.home = store_string(buf);
  }
#endif

    systemDefaults.state       = findState(symbols);
    systemDefaults.startup     = store_string(PrologPath(DEFSTARTUP, plp));
    systemDefaults.local       = DEFLOCAL;
    systemDefaults.global      = DEFGLOBAL;
    systemDefaults.trail       = DEFTRAIL;
    systemDefaults.argument    = DEFARGUMENT;
    systemDefaults.goal	       = "'$welcome'";
    systemDefaults.toplevel    = "prolog";
    systemDefaults.notty       = FALSE;

  } else
  { DEBUG(1, Sdprintf("Restarting from dumped state\n"));
  }

  compile			= FALSE;
  state				= systemDefaults.state;
  status.io_initialised		= FALSE;
  status.initialised		= FALSE;
  status.notty			= systemDefaults.notty;
  status.boot			= FALSE;
  status.extendMode		= TRUE;
  status.autoload		= TRUE;

  argc--; argv++;

					/* EMACS inferior processes */
					/* PceEmacs inferior processes */
  if ( ((s = getenv("EMACS")) != NULL && streq(s, "t")) ||
       ((s = getenv("INFERIOR")) != NULL && streq(s, "yes")) )
    status.notty = TRUE;

  for(n=0; n<argc; n++)			/* need to check this first */
  { DEBUG(2, Sdprintf("argv[%d] = %s\n", n, argv[n]));
    if (streq(argv[n], "-b") )
      status.boot = TRUE;
  }

  DEBUG(1, {if (status.boot) Sdprintf("Boot session\n");});

  if ( argc >= 2 && streq(argv[0], "-r") )
  { loaderstatus.restored_state = lookupAtom(AbsoluteFile(argv[1]));
    argc -= 2, argv += 2;		/* recover; we've done this! */
  }

  if ( argc >= 2 && streq(argv[0], "-x") )
  { state = argv[1];
    argc -= 2, argv += 2;
    explicit_state = TRUE;
    DEBUG(1, Sdprintf("Startup file = %s\n", state));
#ifdef ASSOCIATE_STATE
  } else if ( argc == 1 && stripostfix(argv[0], ASSOCIATE_STATE) )
  { state = argv[0];
    argc--, argv++;
    explicit_state = TRUE;
    DEBUG(1, Sdprintf("Startup file = %s\n", state));
#endif /*ASSOCIATE_STATE*/
  }
  
  if ( argc >= 1 && streq(argv[0], "-help") )
    usage();
  if ( argc >= 1 && streq(argv[0], "-arch") )
    arch();
  if ( argc >= 1 && streq(argv[0], "-v") )
    version();

#define K * 1024L

  if ( status.boot == FALSE && status.dumped == FALSE )
  { if ( state == NULL )
      warnNoState();

    DEBUG(1, Sdprintf("Scanning %s for options\n", state));
    if ( loadWicFile(state, TRUE, TRUE) == FALSE )
      Halt(1);
    DEBUG(2, Sdprintf("options.localSize    = %ld\n", options.localSize));
    DEBUG(2, Sdprintf("options.globalSize   = %ld\n", options.globalSize));
    DEBUG(2, Sdprintf("options.trailSize    = %ld\n", options.trailSize));
    DEBUG(2, Sdprintf("options.argumentSize = %ld\n", options.argumentSize));
    DEBUG(2, Sdprintf("options.goal         = %s\n",  options.goal));
    DEBUG(2, Sdprintf("options.topLevel     = %s\n",  options.topLevel));
    DEBUG(2, Sdprintf("options.initFile     = %s\n",  options.initFile));
  } else
  { options.compileOut	  = "a.out";
    options.localSize	  = systemDefaults.local    K;
    options.globalSize	  = systemDefaults.global   K;
    options.trailSize	  = systemDefaults.trail    K;
    options.argumentSize  = systemDefaults.argument K;
    options.goal	  = systemDefaults.goal;
    options.topLevel	  = systemDefaults.toplevel;
    options.initFile      = systemDefaults.startup;
  }

  for( ; argc > 0 && (argv[0][0] == '-' || argv[0][0] == '+'); argc--, argv++ )
  { if ( streq(&argv[0][1], "tty") )
    { status.notty = (argv[0][0] == '-');
      continue;
    }
    if ( streq(&argv[0][1], "-" ) )	/* pl <plargs> -- <app-args> */
      break;

    s = &argv[0][1];
    while(*s)
    { switch(*s)
      { case 'd':	if (argc > 1)
			{ status.debugLevel = atoi(argv[1]);
			  argc--, argv++;
			} else
			  usage();
			break;
	case 'O':	status.optimise = TRUE;
			break;
  	case 'o':	optionString(options.compileOut);
			explicit_compile_out = TRUE;
			break;
	case 'f':	optionString(options.initFile);
			break;
	case 'g':	optionString(options.goal);
			break;
	case 't':	optionString(options.topLevel);
			break;
	case 'c':	compile = TRUE;
			break;
	case 'b':	status.boot = TRUE;
			break;
	case 'B':
#if !O_DYNAMIC_STACKS
			options.localSize    = 32 K;
			options.globalSize   = 8 K;
			options.trailSize    = 8 K;
			options.argumentSize = 1 K;
#endif
			goto next;
	case 'L':	options.localSize    = atoi(++s) K; goto next;
	case 'G':	options.globalSize   = atoi(++s) K; goto next;
	case 'T':	options.trailSize    = atoi(++s) K; goto next;
	case 'A':	options.argumentSize = atoi(++s) K; goto next;
      }
      s++;
    }
    next:;
  }
#undef K
  
  DEBUG(1, Sdprintf("Command line options parsed\n"));

  setupProlog();
  systemMode(TRUE);

  if ( status.boot )
  { if ( !explicit_compile_out )
      options.compileOut = proposeStartupFile(NULL);

    status.autoload = FALSE;
    if ( compileFileList(options.compileOut, argc, argv) == TRUE )
    {
#if defined(__WINDOWS__) || defined(__WIN32__)
      PlMessage("Boot compilation has created %s", options.compileOut);
#else
      if ( !explicit_compile_out )
	Sfprintf(Serror, "Result stored in %s\n", options.compileOut);
#endif
      Halt(0);
    }

    Halt(1);
  }

  if ( state != NULL )
  { status.boot = TRUE;
    if ( loadWicFile(state, TRUE, FALSE) == FALSE )
      Halt(1);
    status.boot = FALSE;
  }

  reinitForeign(mainArgc, mainArgv);	/* run PL_reinit_hook() functions */
  systemMode(FALSE);
  status.dumped = TRUE;
  status.initialised = TRUE;

#if O_LINK_PCE
  PL_register_foreign("$pce_init", 0, pl_pce_init, PL_FA_TRANSPARENT, 0);
#endif

  DEBUG(1, Sdprintf("Starting Prolog Engine\n"));

  if ( compile )
  { Halt(prolog(lookupAtom("$compile")) ? 0 : 1);
  }
    
  return prolog(lookupAtom("$initialise"));
}


static void
usage()
{ static char *lines[] = {
    "%s: Usage:\n",
    "    1) %s -help     Display this message\n",
    "    2) %s -v        Display version information\n",
    "    3) %s [options]\n",
    "    4) %s [options] [-o output] -c file ...\n",
    "    5) %s [options] [-o output] -b bootfile -c file ...\n",
    "Options:\n",
    "    -x state        Start from state (must be first)\n",
    "    -[LGTAP]kbytes  Specify {Local,Global,Trail,Argument,Lock} stack sizes\n",
    "    -B              Small stack sizes to prepare for boot\n",
    "    -t toplevel     Toplevel goal\n",
    "    -g goal         Initialisation goal\n",
    "    -f file         Initialisation file\n",
    "    [+/-]tty        Allow tty control\n",
    "    -O              Optimised compilation\n",
    NULL
  };
  char **lp = lines;

  for(lp = lines; *lp; lp++)
    Sfprintf(Serror, *lp, BaseName(mainArgv[0]));

  Halt(1);
}

static void
version()
{ Sprintf("SWI-Prolog version %s for %s\n", PLVERSION, ARCH);

  Halt(0);
}


static void
arch()
{ Sprintf("%s\n", ARCH);

  Halt(0);
}


#include <stdarg.h>

bool
sysError(const char *fm, ...)
{ va_list args;

  va_start(args, fm);
  vsysError(fm, args);
  va_end(args);

  PL_fail;
}


bool
fatalError(const char *fm, ...)
{ va_list args;

  va_start(args, fm);
  vfatalError(fm, args);
  va_end(args);

  PL_fail;
}


bool
warning(const char *fm, ...)
{ va_list args;

  va_start(args, fm);
  vwarning(fm, args);
  va_end(args);

  PL_fail;
}


bool
vsysError(const char *fm, va_list args)
{ Sfprintf(Serror, "[PROLOG INTERNAL ERROR:\n\t");
  Svfprintf(Serror, fm, args);
  if ( gc_status.active )
  { Sfprintf(Serror,
	    "\n[While in %ld-th garbage collection; skipping stacktrace]\n",
	    gc_status.collections);
  }
#if defined(O_DEBUGGER)
  else
  { Sfprintf(Serror, "\n[Switched to system mode: style_check(+dollar)]\n");
    debugstatus.styleCheck |= DOLLAR_STYLE;
    Sfprintf(Serror, "PROLOG STACK:\n");
    backTrace(NULL, 10);
    Sfprintf(Serror, "]\n");
  }
#endif /*O_DEBUGGER*/

action:
  Sprintf("\nAction? "); Sflush(Soutput);
  ResetTty();
  switch(getSingleChar())
  { case 'a':
      pl_abort();
      break;
    case 'e':
      Halt(3);
      break;
    default:
      Sprintf("Unknown action.  Valid actions are:\n"
	      "\ta\tabort to toplevel\n"
	      "\te\texit Prolog\n");
      goto action;
  }

  pl_abort();
  Halt(3);
  PL_fail;
}


bool
vfatalError(const char *fm, va_list args)
{
#if defined(__WINDOWS__) || defined(__WIN32__)
  char msg[500];
  Ssprintf(msg, "[FATAL ERROR:\n\t");
  Svsprintf(&msg[strlen(msg)], fm, args);
  Ssprintf(&msg[strlen(msg)], "]");
  
  PlMessage(msg);
#else
  Sfprintf(Serror, "[FATAL ERROR:\n\t");
  Svfprintf(Serror, fm, args);
  Sfprintf(Serror, "]\n");
#endif

  Halt(2);
  PL_fail;
}


bool
vwarning(const char *fm, va_list args)
{ toldString();

  if ( ReadingSource && !status.boot && status.initialised )
  { fid_t cid = PL_open_foreign_frame();
    term_t argv = PL_new_term_refs(3);
    predicate_t pred = PL_pred(FUNCTOR_exception3, MODULE_user);
    term_t a = PL_new_term_ref();
    char message[LINESIZ];
    qid_t qid;
    int rval;

    Svsprintf(message, fm, args);

    PL_put_atom(   argv+0, ATOM_warning);
    PL_put_functor(argv+1, FUNCTOR_warning3);
    PL_get_arg(1, argv+1, a); PL_unify_atom(a, source_file_name);
    PL_get_arg(2, argv+1, a); PL_unify_integer(a, source_line_no);
    PL_get_arg(3, argv+1, a); PL_unify_string_chars(a, message);
    
    qid = PL_open_query(MODULE_user, FALSE, pred, argv);
    rval = PL_next_solution(qid);
    PL_close_query(qid);
    PL_discard_foreign_frame(cid);

    if ( !rval )
    { Sfprintf(Serror, "[WARNING: (%s:%d)\n\t%s]\n",
	       stringAtom(source_file_name), source_line_no, message);
    }

    PL_fail;				/* handled */
  }

  Sfprintf(Serror, "[WARNING: ");
  Svfprintf(Serror, fm, args);
  Sfprintf(Serror, "]\n");

  pl_trace();

  PL_fail;
}
