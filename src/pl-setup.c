/*  pl-setup.c,v 1.26 1995/02/07 12:12:31 jan Exp

    Copyright (c) 1990 Jan Wielemaker. All rights reserved.
    See ../LICENCE to find out about your rights.
    jan@swi.psy.uva.nl

    Purpose: Initialise the system
*/

/*#define O_DEBUG 1*/

#define GLOBAL				/* allocate global variables here */
#include "pl-incl.h"
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#undef ulong
#define ulong unsigned long
#undef max
#define max(a,b) ((a) > (b) ? (a) : (b))

#define K * 1024

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This module initialises the system and defines the global variables.  It
also holds the code  for  dynamically  expanding  stacks  based  on  MMU
access.   Finally  it holds the code to handle signals transparently for
foreign language code or packages with which Prolog was linked together.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

forwards void initStacks(long, long, long, long);
forwards void initFeatures(void);
forwards void initSignals(void);

#undef I
#define I TAGEX_INDIRECT

const unsigned int tagtypeex[] = 
{
	    /* var     int    float   atom   string   list    term     ref */
/* static */	0,	0,	0,	0,	0,	0,	0,	0,
/* heap */	0,	I,	I,	0,	I,	0,	0,	0,
/* global */	0,	I,	I,	0,	I,	0,	0,	0,
/* local */	0,	0,	0,	0,	0,	0,	0,	0
};

#undef I

void
setupProlog(void)
{ DEBUG(1, Sdprintf("Starting Heap Initialisation\n"));

  GD->critical = 0;
  LD->aborted = FALSE;
  signalled = 0;

  startCritical;
  initMemAlloc();
#if HAVE_SIGNAL
  DEBUG(1, Sdprintf("Prolog Signal Handling ...\n"));
  initSignals();
#endif
  DEBUG(1, Sdprintf("Stacks ...\n"));
  initStacks(GD->options.localSize, 
	     GD->options.globalSize, 
	     GD->options.trailSize, 
	     GD->options.argumentSize);

  lTop = lBase;
  tTop = tBase;
  gTop = gBase;
  aTop = aBase;

  base_addresses[STG_LOCAL]  = (unsigned long)lBase;
  base_addresses[STG_GLOBAL] = (unsigned long)gBase;
  base_addresses[STG_TRAIL]  = (unsigned long)tBase;
  DEBUG(1, Sdprintf("base_addresses[STG_LOCAL] = %p\n",
		    base_addresses[STG_LOCAL]));
  DEBUG(1, Sdprintf("base_addresses[STG_GLOBAL] = %p\n",
		    base_addresses[STG_GLOBAL]));
  DEBUG(1, Sdprintf("base_addresses[STG_TRAIL] = %p\n",
		    base_addresses[STG_TRAIL]));

#ifdef O_LIMIT_DEPTH
  depth_limit   = (unsigned long)DEPTH_NO_LIMIT;
  depth_reached = 0;
#endif

  emptyStacks();
  DEBUG(1, Sdprintf("Atoms ...\n"));
  initAtoms();
  DEBUG(1, Sdprintf("Features ...\n"));
  initFeatures();
  DEBUG(1, Sdprintf("Functors ...\n"));
  initFunctors();
  DEBUG(1, Sdprintf("Modules ...\n"));
  initTables();
  initModules();
  DEBUG(1, Sdprintf("Records ...\n"));
  initRecords();
  DEBUG(1, Sdprintf("Flags ...\n"));
  initFlags();
  DEBUG(1, Sdprintf("Foreign Predicates ...\n"));
  initBuildIns();
  DEBUG(1, Sdprintf("Operators ...\n"));
  initOperators();
  DEBUG(1, Sdprintf("Arithmetic ...\n"));
  initArith();
  DEBUG(1, Sdprintf("Tracer ...\n"));
  initTracer();
  debugstatus.styleCheck = SINGLETON_CHECK;
  DEBUG(1, Sdprintf("wam_table ...\n"));
  initWamTable();
  DEBUG(1, Sdprintf("IO ...\n"));
  initIO();
  DEBUG(1, Sdprintf("Loader ...\n"));
  resetLoader();
  DEBUG(1, Sdprintf("Symbols ...\n"));
  getSymbols();
  DEBUG(1, Sdprintf("Term ...\n"));
  resetTerm();
  GD->io_initialised = TRUE;

  endCritical;

  environment_frame = (LocalFrame) NULL;
  LD->statistics.inferences = 0;
#if O_STORE_PROGRAM || O_SAVE
  GD->cannot_save_program = NULL;
#else
  GD->cannot_save_program = "Not supported on this machine";
#endif

#if O_XWINDOWS
  DEBUG(1, Sdprintf("XWindows ...\n");
  initXWindows();
#endif

  DEBUG(1, Sdprintf("Heap Initialised\n"));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Feature interface
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void
CSetFeature(const char *name, const char *value)
{ setFeature(lookupAtom(name), FT_ATOM, lookupAtom(value));
}

static void
CSetIntFeature(const char *name, long value)
{ setFeature(lookupAtom(name), FT_INTEGER, value);
}

static void
initFeatures()
{ CSetFeature("arch",		ARCH);
#if __WIN32__
  if ( iswin32s() )
    CSetFeature("win32s",	"true");
  CSetFeature("windows",	"true");
#endif
  CSetIntFeature("version",	PLVERSION);
  if ( systemDefaults.home )
    CSetFeature("home",		systemDefaults.home);
  CSetFeature("c_libs",		C_LIBS);
  CSetFeature("c_cc",		C_CC);
  CSetFeature("c_ldflags",	C_LDFLAGS);
  CSetFeature("gc",		"true");
  CSetFeature("trace_gc",	"false");
#ifdef O_SAVE
  CSetFeature("save",	       "true");
  CSetFeature("save_program",  "true");
#endif
#ifdef O_STORE_PROGRAM
  CSetFeature("save_program",	"true");
#endif
#if defined(O_FOREIGN) || defined(O_MACH_FOREIGN) || defined(O_AIX_FOREIGN)
  CSetFeature("load_foreign",  "true");
#endif
#if defined(HAVE_DLOPEN) || defined(HAVE_SHL_LOAD)
  CSetFeature("open_shared_object", "true");
#endif
#ifdef O_DLL
  CSetFeature("dll", "true");
#endif
#if O_DYNAMIC_STACKS
  CSetFeature("dynamic_stacks",	"true");
#endif
#ifdef HAVE_POPEN
  CSetFeature("pipe",		"true");
#endif
#ifdef ASSOCIATE_SRC
  CSetFeature("associate",	ASSOCIATE_SRC);
#endif
#ifdef O_DDE
  CSetFeature("dde",		"true");
#endif
#ifdef O_RUNTIME
  CSetFeature("runtime",	"true");
  CSetFeature("debug_on_error",	"false");
  CSetFeature("report_error",	"false");
#else
  CSetFeature("debug_on_error",	"true");
  CSetFeature("report_error",	"true");
#endif
					/* ISO features */
  CSetIntFeature("max_integer", PLMAXINT);
  CSetIntFeature("min_integer", PLMININT);
  CSetIntFeature("max_tagged_integer", PLMAXTAGGEDINT);
  CSetIntFeature("min_tagged_integer", PLMINTAGGEDINT);
  CSetFeature("bounded",	"true");
  if ( (-3 / 2) == -2 )
    CSetFeature("integer_rounding_function", "down");
  else
    CSetFeature("integer_rounding_function", "toward_zero");
  CSetFeature("max_arity", "unbounded");
  CSetFeature("float_format", "%g");
  CSetFeature("character_escapes", "true");
  CSetFeature("tty_control", GD->cmdline.notty ? "false" : "true");
  CSetFeature("allow_variable_name_as_functor", "false");
  CSetIntFeature("toplevel_var_size", 1000);
#if defined(__unix__) || defined(unix)
  CSetFeature("unix", "true");
#endif

#if defined(__DATE__) && defined(__TIME__)
  { char buf[100];

    Ssprintf(buf, "%s, %s", __DATE__, __TIME__);
    CSetFeature("compiled_at",	buf);
  }
#endif
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			   SIGNAL HANDLING

SWI-Prolog catches a number of signals.   Interrupt  is catched to allow
the user to interrupt  normal   execution.  Segmentation  violations are
trapped on machines using the MMU to implement stack overflow checks and
stack expansion. These signal handlers needs  to be preserved over saved
states and the system  should  allow   foreign  language  code to handle
signals without interfering  with  Prologs   signal  handlers.  For this
reason a layer is wired around the OS signal handling.

Code in SWI-Prolog should  call  PL_signal()  rather  than  signal()  to
install  signal  handlers.  SWI-Prolog assumes the handler function is a
void function.  On some systems this gives  some  compiler  warnigns  as
they  define  signal handlers to be int functions.  This should be fixed
some day.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if HAVE_SIGNAL

#ifdef __WIN32__
#define HAVE_SIGNALS !iswin32s()
#else
#define HAVE_SIGNALS 1
#endif

#define PLSIG_PREPARED 0x01		/* signal is prepared */
#define PLSIG_THROW    0x02		/* throw signal(num, name) */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Define the signasls and  their  properties.   This  could  be nicer, but
different systems provide different signals, and   above all, MS systems
provide very few.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static struct signame
{ int 	      sig;
  const char *name;
  int	      flags;
} signames[] = 
{
#ifdef SIGHUP
  { SIGHUP,	"hup",    0},
#endif
  { SIGINT,	"int",    0},
#ifdef SIGQUIT
  { SIGQUIT,	"quit",   0},
#endif
  { SIGILL,	"ill",    PLSIG_THROW},
  { SIGABRT,	"abrt",   0},
  { SIGFPE,	"fpe",    PLSIG_THROW},
#ifdef SIGKILL
  { SIGKILL,	"kill",   0},
#endif
  { SIGSEGV,	"segv",   PLSIG_THROW},
#ifdef SIGPIPE
  { SIGPIPE,	"pipe",   PLSIG_THROW},
#endif
#ifdef SIGALRM
  { SIGALRM,	"alrm",   PLSIG_THROW},
#endif
  { SIGTERM,	"term",   0},
#ifdef SIGUSR1
  { SIGUSR1,	"usr1",   0},
#endif
#ifdef SIGUSR2
  { SIGUSR2,	"usr2",   0},
#endif
#ifdef SIGCHLD
  { SIGCHLD,	"chld",   0},
#endif
#ifdef SIGCONT
  { SIGCONT,	"cont",   0},
#endif
#ifdef SIGSTOP
  { SIGSTOP,	"stop",   0},
#endif
#ifdef SIGTSTP
  { SIGTSTP,	"tstp",   0},
#endif
#ifdef SIGTTIN
  { SIGTTIN,	"ttin",   0},
#endif
#ifdef SIGTTOU
  { SIGTTOU,	"ttou",   0},
#endif
#ifdef SIGTRAP
  { SIGTRAP,	"trap",   0},
#endif
#ifdef SIGBUS
  { SIGBUS,	"bus",    PLSIG_THROW},
#endif
#ifdef SIGSTKFLT
  { SIGSTKFLT,	"stkflt", 0},
#endif
#ifdef SIGURG
  { SIGURG,	"urg",    0},
#endif
#ifdef SIGIO
  { SIGIO,	"io",     0},
#endif
#ifdef SIGPOLL
  { SIGPOLL,	"poll",   0},
#endif
#ifdef SIGXCPU
  { SIGXCPU,	"xcpu",   PLSIG_THROW},
#endif
#ifdef SIGXFSZ
  { SIGXFSZ,	"xfsz",   PLSIG_THROW},
#endif
#ifdef SIGVTALRM
  { SIGVTALRM,	"vtalrm", PLSIG_THROW},
#endif
#ifdef SIGPROF
  { SIGPROF,	"prof",   0},
#endif
#ifdef SIGPWR
  { SIGPWR,	"pwr",    0},
#endif
  { -1,		NULL,     0}
};

static const char *
signal_name(int sig)
{ struct signame *sn = signames;

  for( ; sn->name; sn++ )
  { if ( sn->sig == sig )
      return sn->name;
  }

  return "unknown";
}


static int
signal_index(const char *name)
{ struct signame *sn = signames;

  for( ; sn->name; sn++ )
  { if ( streq(sn->name, name) )
      return sn->sig;
  }

  return 0;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SWI-Prolog main signal handler. Any  signal   arrives  here first, after
which it is dispatched to the real handler.   The task of the handler is
to ensure it is safe to start a query.

There are a few possible problems:

	* The system is writing the body-arguments from the next clause.
	In this case it is working above `lTop'.  So we raise this to the
	maximum offset.

	* The system is performing a garbage collection.  We should block
	signals while in garbage-collection and non-blockable signals should
	raise a fatal error.

	* The system is in a `critical section'.  These are insufficiently
	flagged at the moment.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void
pl_signal_handler(int sig)
{ SigHandler sh = &LD->sig_handlers[sig];
  fid_t fid;
  LocalFrame lTopSave = lTop;
  int saved_current_signal = LD->current_signal;

  if ( gc_status.active )
  { fatalError("Received signal %d (%s) while in %ld-th garbage collection",
	       sig, signal_name(sig), gc_status.collections);
  }

  if ( GD->critical )
  { PL_raise(sig);			/* wait for better times! */
    return;
  }

  blockGC();

  lTop = addPointer(lTop, sizeof(struct localFrame) + MAXARITY*sizeof(word));
  fid = PL_open_foreign_frame();
  LD->current_signal = sig;

  DEBUG(1, Sdprintf("Handling signal %d, pred = %p, handler = %p\n",
		    sig, sh->predicate, sh->handler));

  if ( sh->predicate )
  { term_t sigterm = PL_new_term_ref();
    term_t except;
    qid_t qid;

    PL_put_atom_chars(sigterm, signal_name(sig));
    qid = PL_open_query(NULL,
			PL_Q_CATCH_EXCEPTION,
			sh->predicate,
			sigterm);
    if ( !PL_next_solution(qid) && (except = PL_exception(qid)) )
    { PL_cut_query(qid);
      unblockGC();
      PL_throw(except);
      return;				/* make sure! */
    } else
      PL_close_query(qid);
  } else if ( true(sh, PLSIG_THROW) )
  { char *predname;
    int  arity;

    if ( environment_frame )
    { predname = stringAtom(environment_frame->predicate->functor->name);
      arity    = environment_frame->predicate->functor->arity;
    } else
    { predname = NULL;
      arity    = 0;
    }
      
    PL_error(predname, arity, NULL, ERR_SIGNALLED, sig, signal_name(sig));
    unblockGC();

    PL_throw(exception_term);		/* throw longjmp's */
    return;				/* make sure! */
  } else if ( sh->handler )
  { (*sh->handler)(sig);
  }

  LD->current_signal = saved_current_signal;
  PL_discard_foreign_frame(fid);
  lTop = lTopSave;

  unblockGC();
}


#ifndef SA_RESTART
#define SA_RESTART 0
#endif

handler_t
set_sighandler(int sig, handler_t func)
{
#ifdef HAVE_SIGACTION
  struct sigaction old;
  struct sigaction new;

  memset(&new, 0, sizeof(new));	/* deal with other fields */
  new.sa_handler = func;
  new.sa_flags   = SA_RESTART;

  if ( sigaction(sig, &new, &old) == 0 )
    return old.sa_handler;
  else
    return SIG_DFL;
#else
  return signal(sig, func);
#endif
} 


static SigHandler
prepareSignal(int sig)
{ SigHandler sh = &LD->sig_handlers[sig];

  if ( false(sh, PLSIG_PREPARED) )
  { set(sh, PLSIG_PREPARED);
    sh->saved_handler = set_sighandler(sig, pl_signal_handler);
  }

  return sh;
}


static void
unprepareSignal(int sig)
{ SigHandler sh = &LD->sig_handlers[sig];

  if ( true(sh, PLSIG_PREPARED) )
  { set_sighandler(sig, sh->saved_handler);
    sh->flags         = 0;
    sh->handler       = NULL;
    sh->predicate     = NULL;
    sh->saved_handler = NULL;
  }
}


static void
initSignals(void)
{ struct signame *sn = signames;
  

  for( ; sn->name; sn++)
  { if ( sn->flags )
    { SigHandler sh = prepareSignal(sn->sig);
      sh->flags |= sn->flags;
    }
  }
}


void
resetSignals()
{ LD->current_signal = 0;
  signalled = 0L;
}


void
blockSignals()
{
#ifdef HAVE_SIGPROCMASK
  sigset_t set;

  sigfillset(&set);			/* only part? */
  
  sigprocmask(SIG_BLOCK, &set, NULL);
#endif
}


void
unblockSignals()
{
#ifdef HAVE_SIGPROCMASK
  sigset_t set;

  sigfillset(&set);			/* only part? */
  
  sigprocmask(SIG_UNBLOCK, &set, NULL);
#endif
}


void
unblockSignal(int sig)
{
#ifdef HAVE_SIGPROCMASK
  sigset_t set;

  sigemptyset(&set);
  sigaddset(&set, sig);

  sigprocmask(SIG_UNBLOCK, &set, NULL);
  DEBUG(1, Sdprintf("Unblocked signal %d\n", sig));
#endif
}


handler_t
PL_signal(int sig, handler_t func)
{ if ( HAVE_SIGNALS )
  { handler_t old;
    SigHandler sh;

    if ( sig < 1 || sig > MAXSIGNAL )
    { warning("PL_signal(): illegal signal number: %d", sig);
      return SIG_DFL;
    }

    sh = &LD->sig_handlers[sig];
    if ( true(sh, PLSIG_PREPARED) )
    { old = sh->handler;
      if ( func == sh->saved_handler )
	unprepareSignal(sig);
      else
	sh->handler = func;
    } else
    { sh = prepareSignal(sig);
      old = sh->saved_handler;
      sh->handler = func;
    }

    return old;
  } else
    return SIG_DFL;
}


void
PL_handle_signals()
{ while(!GD->critical && signalled)
  { ulong mask = 1L;
    int sig = 1;

    for( ; ; mask <<= 1, sig++ )
    { if ( signalled & mask )
	pl_signal_handler(sig);
    }
  }
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
signal(+Signal, :Old, :Handler)
	Assign Handler to be called if signal arrises.  Example:

		signal(usr1, Old, handle_user_1/1).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

foreign_t
pl_on_signal(term_t sig, term_t name, term_t old, term_t new)
{ int sign = -1;
  SigHandler sh;
  char *sn;
  atom_t a;
  Module m = NULL;

  if ( PL_get_integer(sig, &sign) && sign >= 1 && sign <= MAXSIGNAL )
  { TRY(PL_unify_atom_chars(name, signal_name(sign)));
  } else if ( PL_get_atom_chars(name, &sn) && (sign = signal_index(sn)) )
  { TRY(PL_unify_integer(sig, sign));
  } else
    return PL_error("signal", 4, NULL, ERR_TYPE, ATOM_signal, sig);

  sh = &LD->sig_handlers[sign];

  if ( false(sh, PLSIG_PREPARED) )		/* not handled */
  { TRY(PL_unify_atom(old, ATOM_default));
  } else if ( true(sh, PLSIG_THROW) )		/* throw exception */
  { TRY(PL_unify_atom(old, ATOM_throw));
  } else if ( sh->predicate )			/* call predicate */
  { Definition def = sh->predicate->definition;

    if ( def->module == MODULE_user )
    { TRY(PL_unify_atom(old, def->functor->name));
    } else
    { TRY(PL_unify_term(old,
			PL_FUNCTOR, FUNCTOR_module2,
			   PL_ATOM, def->module->name, 
			   PL_ATOM, def->functor->name));
    }
  }

  if ( PL_compare(old, new) == 0 )
    succeed;					/* no change */

  PL_strip_module(new, &m, new);

  if ( PL_get_atom(new, &a) )
  { if ( a == ATOM_default )
    { unprepareSignal(sign);
    } else if ( a == ATOM_throw )
    { sh = prepareSignal(sign);
      set(sh, PLSIG_THROW);
      sh->handler   = NULL;
      sh->predicate = NULL;
    } else
    { predicate_t pred = lookupProcedure(PL_new_functor(a, 1), m);
      
      sh = prepareSignal(sign);
      clear(sh, PLSIG_THROW);
      sh->predicate = pred;
    }
  } else
    return PL_error("signal", 4, NULL, ERR_TYPE, ATOM_atom, sig);

  succeed;
}

#endif /*HAVE_SIGNAL*/

		 /*******************************
		 *	       STACKS		*
		 *******************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Create nice empty stacks. exception_bin   and  exception_printed are two
term-references that must be low on  the   stack  to  ensure they remain
valid while the stack is unrolled after an exception.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void
emptyStacks()
{ environment_frame = NULL;
  fli_context       = NULL;
  lTop = lBase;
  tTop = tBase;
  gTop = gBase;
  aTop = aBase;

  PL_open_foreign_frame();
  exception_bin        = PL_new_term_ref();
  exception_printed    = PL_new_term_ref();
  LD->exception.tmp    = PL_new_term_ref();
}


#if O_DYNAMIC_STACKS

static void init_stack(Stack s, char *name,
		       caddress base, long limit, long minsize);
static void gcPolicy(Stack s, int policy);

#ifndef NO_SEGV_HANDLING
#ifdef SIGNAL_HANDLER_PROVIDES_ADDRESS
static RETSIGTYPE segv_handler(int sig, int type,
			       SignalContext scp, char *addr);
#else
static RETSIGTYPE segv_handler(int sig);
#endif
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
STACK_SEPARATION defines the  space  between   the  stacks.  The maximum
discontinuity while writing the local stack  is determined by the number
of variables in the clause.  An example worst case is:

foo :-
	(   failing_goal,
	    bar(term(A, B, C, ....))
	;   hello(AnotherVar)
	).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define STACK_SEPARATION ROUND(MAXVARIABLES*sizeof(word), size_alignment)
#define STACK_SIGNAL	 (2 * size_alignment)
#define STACK_RESERVE	 (1 * size_alignment)
#define STACK_MINIMUM    (32 * 1024)

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			STACK MEMORY MANAGEMENT

In these days some operating systems allows the  user  to  map  physical
memory  anywhere  in  the  virtual  address  space.  For multiple stacks
machines such as Prolog, this is ideal.  The  stacks  can  be  allocated
very  far  appart  with  large  gaps  between  them.   Stack overflow is
detected by hardware and results (in  Unix)  in  a  segmentation  fault.
This fault is trapped and the stack is automatically expanded by mapping
more  memory.

In theory the stacks can be deallocated dynamically as  well,  returning
the  resources to the system.  Currently this can be done explicitely by
calling  trim_stacks/0  and  the  garbage  collector.    It   might   be
interesting  to  do  this  automatically  at  certain points to minimise
memory requirements.  How?

Currently this mechanism can use mmap() and munmap() of SunOs 4.0 or the
system-V shared memory primitives (if they meet certain criteria).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <errno.h>
#ifndef WIN32
extern int errno;
#endif /*WIN32*/

static int size_alignment;	/* Stack sizes must be aligned to this */
static int base_alignment;	/* Stack bases must be aligned to this */

#undef MB
#define MB * (1024L * 1024L)

static long
align_size(long int x)
{ return x % size_alignment ? (x / size_alignment + 1) * size_alignment : x;
}

static long
align_base(long int x)
{ return x % base_alignment ? (x / base_alignment + 1) * base_alignment : x;
}

static long
align_base_down(long int x)
{ return (x / base_alignment) * base_alignment;
}

#ifdef MMAP_STACK
#include <sys/mman.h>
#include <fcntl.h>

static int mapfd = -1;			/* File descriptor used for mapping */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Return a file descriptor to a file, open  for  reading  and  holding  at
least  one  page of 0's. On some systems /dev/zero is available for this
trick.  If not, a file of one page is created under the name /tmp/pl-map
if it does not already exists and this file is opened for  reading.   It
can  be  shared  by  many  SWI-Prolog  processes  and (therefore) is not
removed on exit.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifdef HAVE_MAP_ANON
#if !defined(MAP_ANON) && defined(MAP_ANONYMOUS)
#define MAP_ANON MAP_ANONYMOUS
#endif

#define get_map_fd() (-1)
#define STACK_MAP_TYPE MAP_ANON|MAP_PRIVATE|MAP_FIXED

#else /*HAVE_MAP_ANON*/

#define STACK_MAP_TYPE MAP_PRIVATE|MAP_FIXED

static int
get_map_fd()
{ int fd;
  static char *map = "/tmp/pl-map";

  if ( (fd = open("/dev/zero", O_RDONLY)) >= 0 )
    return fd;

  if ( (fd = open(map, O_RDONLY)) < 0 )
  { if ( errno == ENOENT )
    { char buf[1024];
      char *s;
      int n;
      int oldmask = umask(0);

      if ( (fd = open(map, O_RDWR|O_CREAT, 0666)) < 0 )
      { fatalError("Can't create map file %s: %s", map, OsError());
        return -1;
      }
      umask(oldmask);
      for(n=1024, s = buf; n > 0; n--)
        *s++ = EOS;
      for(n=size_alignment/1024; n > 0; n--)
      { if ( write(fd, buf, 1024) != 1024 )
          fatalError("Failed to create map file %s: %s\n", map, OsError());
      }

      return fd;
    }
    fatalError("Can't open map file %s: %s", map, OsError());
    return -1;
  }

  return fd;
}
#endif /*HAVE_MAP_ANON*/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Estimate the top if the heap. The default is to get the size of the heap
using getrlimit(), add this to the estimated  base and use the result as
top address.

This does not always appewar  to  work.   If  you  know the top, #define
TOPOFHEAP in config.h. Othewise #define  it  to   0,  in  which case the
system will allocate a default heap of 64 MB and the stacks above that.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifdef HAVE_GETRLIMIT
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef RLIMIT_DATA
#ifndef HAVE_RLIM_T
typedef unsigned long rlim_t;
#endif
static ulong
dataLimit()
{ struct rlimit limit;
 
  if ( getrlimit(RLIMIT_DATA, &limit) == 0 )
  { rlim_t datasize = limit.rlim_cur;
    rlim_t maxlong  = (rlim_t)(1L << (LONGBITSIZE-1)) - 1;
 
    if ( datasize > maxlong )
      datasize = (ulong)maxlong;
 
    return datasize;
  }
 
  return 0L;
}
#else
#define dataLimit() (0L)
#endif /*RLIMIT_DATA*/
#else
#define dataLimit() (0L)
#endif /*HAVE_GETRLIMIT*/


#ifdef TOPOFHEAP
#define topOfHeap() TOPOFHEAP
#else /*TOPOFHEAP*/
					/* __linux__ TASK_SIZE stuff */
					/* contributed by Roman Hodek */
#ifdef __linux__
#include <asm/page.h>
#include <asm/system.h>
#include <asm/ptrace.h>
#include <asm/segment.h>
#include <asm/processor.h>
#endif

ulong
topOfHeap()
{ ulong data = dataLimit();

  if ( data )
  { ulong top = heap_base + data;

#ifdef TASK_SIZE
    if ( top < heap_base || top > TASK_SIZE )
      top = TASK_SIZE;
#else
    if ( top < heap_base )
      return 0L;
#endif

    DEBUG(1, Sdprintf("Heap: %p ... %p\n", (void *)heap_base, (void *)top));
    return top;
  }
    
  return 0L;
}
#endif /*TOPOFHEAP*/


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Expand stack `s' by one page.  This might not be  enough,  but  in  this
(very  rare) case another segmentation fault will follow to get the next
page.  The memory is expanded by mapping the map-fd file onto  the  page
using  a  private  map.  This way the contents of the map-file is copied
into the page but all changes to the page are  kept  local.   Note  that
SunOs  4.0.0  on SUN-3 has a bug that causes the various mapped pages to
point to the same physical memory.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void
mapOrOutOf(Stack s)
{ ulong incr;
  long  newroom;

  if ( s->top > s->max )
    incr = ROUND(((ulong)s->top - (ulong)s->max), size_alignment);
  else
    incr = size_alignment;

  newroom = (ulong)s->limit - ((ulong)s->max + incr);
  if ( newroom < 0 )
    outOfStack(s, STACK_OVERFLOW_FATAL);

  if ( mmap(s->max, incr,
	    PROT_READ|PROT_WRITE, STACK_MAP_TYPE,
	    mapfd, 0L) != s->max )
    fatalError("Failed to map memory at 0x%x for %d bytes on fd=%d: %s\n",
	       s->max, incr, mapfd, OsError());

  DEBUG(1, Sdprintf("mapped %d bytes from 0x%x to 0x%x\n",
		    size_alignment, (unsigned) s->max, s->max + incr));
  s->max = addPointer(s->max, incr);

  if ( newroom <= STACK_SIGNAL )
  { if ( newroom <= STACK_RESERVE )
    {
#ifndef NO_SEGV_HANDLING
      LD->current_signal = SIGSEGV;
#endif
      outOfStack(s, STACK_OVERFLOW_SIGNAL_IMMEDIATELY);
    } else
      outOfStack(s, STACK_OVERFLOW_SIGNAL);
  }

  considerGarbageCollect(s);
}


#ifdef NO_SEGV_HANDLING
void
ensureRoomStack(Stack s, int bytes)
{ while((char *)s->max - (char *)s->top < (int)bytes)
    mapOrOutOf(s);
}
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unmap() returns all memory resources of a stack that are  no  longer  in
use to the OS.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void
unmap(Stack s)
{ caddress top  = (s->top > s->min ? s->top : s->min);
  caddress addr = (caddress) align_size((long) top + size_alignment);

  if ( addr < s->max )
  { if ( munmap(addr, (char *)s->max - (char *)addr) != 0 )
      fatalError("Failed to unmap memory: %s", OsError());
    s->max = addr;
  }
}


#ifdef O_SAVE

static void
deallocateStack(Stack s)
{ long len = (unsigned long)s->max - (unsigned long)s->base;

  if ( len > 0 && munmap(s->base, len) != 0 )
    fatalError("Failed to unmap memory: %s", OsError());
}


void
deallocateStacks(void)
{ deallocateStack((Stack) &LD->stacks.local);
  deallocateStack((Stack) &LD->stacks.global);
  deallocateStack((Stack) &LD->stacks.trail);
  deallocateStack((Stack) &LD->stacks.argument);
}


bool
restoreStack(Stack s)
{ caddress max;
  long len;
  struct stat statbuf;

  if ( mapfd < 0 || fstat(mapfd, &statbuf) == -1 )
  { mapfd = get_map_fd();
    base_alignment = size_alignment = getpagesize();
  }

  max = (caddress) align_size((long) s->top + 1);
  len = max - (caddress) s->base;

  if ( mmap(s->base, len,
	    PROT_READ|PROT_WRITE, STACK_MAP_TYPE,
	    mapfd, 0L) != s->base )
    fatalError("Failed to map memory at 0x%x for %d bytes on fd=%d: %s\n",
	       s->base, len, mapfd, OsError());

  s->max = max;
  DEBUG(0, Sdprintf("mapped %d bytes from 0x%x\n", len, (unsigned) s->base));
  succeed;
}
#endif /*O_SAVE*/

#endif /* MMAP_STACK */

#if O_SHARED_MEMORY
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#if gould
#define S_IRUSR SHM_R
#define S_IWUSR SHM_W
#endif
#if mips
struct pte { long pad };		/* where is the real one? */
#include <sys/param.h>
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Shared memory based MMU controlled stacks are a bit  more  tricky.   The
main  problem is that shared memory segments are scares resources.  Upto
a certain limit, each time the size of the stack is doubled.  Afterwards
the stack grows in fixed segments  of  size  s->segment_initial  *  2  ^
s->segment_double.   These  parameters  may  vary  from  stack to stack,
suiting the caracteristics of the stack and of the OS limits on  virtual
address space and number of shared memory segnments.  See pl-incl.h

The  shared  memory  segments  are  created,  mapped   and   immediately
afterwards  freed.   According  to  the documentation they actually will
live untill they are unmapped by the last process.  Immediately  freeing
them  avoids the burden to do this on exit() and ensures these resources
are freed, also if SWI-Prolog crashes.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if O_SHM_ALIGN_FAR_APART

#define min(a, b) ((a) < (b) ? (a) : (b))

static long
new_stack_size(s)
Stack s;
{ long size  = s->top - s->base;
  long free  = size / s->segment_initial;
  long limit = diffPointers(s->limit, s->base);

  if ( free > s->segment_double ) free = s->segment_double;
  else if ( free < 1 )            free = 1;
  
  size = align_size(size + free * s->segment_initial);  

  if ( size > limit )
    size = limit;

  return size;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
resize_segment(s, n, size)
  Resize segment n of stack s to get size size.  The base address of the
  segement is assumed to be correct.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void
resize_segment(s, n, size)
Stack s;
int n;
long size;
{ if ( s->segments[n].size != size )
  { int id = -1;
    char *addr;

    if ( size > 0 )
    { if ( (id=shmget(IPC_PRIVATE, size, S_IRUSR|S_IWUSR)) < 0 )
	fatalError("Failed to create shared memory object: %s", OsError());
      if ( (addr = shmat(id, 0, 0)) < 0 )
	fatalError("Failed to attach shared memory segment: %s", OsError());
      memcpy(s->segments[n].base, addr, min(size, s->segments[n].size));
      if ( shmdt(addr) < 0 )
      	fatalError("Failed to detach shared memory segment: %s", OsError());
    }    

    if ( s->segments[n].size > 0 )
      if ( shmdt(s->segments[n].base) < 0 )
      	fatalError("Failed to detach shared memory segment: %s", OsError());
    
    if ( id >= 0 )
    { DEBUG(0, Sdprintf("Attach segment of size %ld at 0x%x\n",
		      size, s->segments[n].base));
      if ( shmat(id, s->segments[n].base, 0) != s->segments[n].base )
      	fatalError("Failed to attach shared memory segment at 0x%x: %s",
		   s->segments[n].base, OsError());
      
      if ( shmctl(id, IPC_RMID, NULL) < 0 )
	fatalError("Failed to release shared memory object: %s", OsError());
    }

    s->segments[n].size = 0;
  }
}


void
mapOrOutOf(Stack s)
{ long new_size = new_stack_size(s);
  int  top_segment = new_size / base_alignment;
  int  n;

  DEBUG(1, Sdprintf("Expanding %s stack to %ld\n", s->name, new_size));

  for(n=0; n < top_segment; n++)
    resize_segment(s, n, base_alignment);

  resize_segment(s, n, new_size % base_alignment);

  for(n++; s->segments[n].size > 0; n++ )
    resize_segment(s, n, 0L);

  s->max = s->base + new_size;
  considerGarbageCollect(s);
}


static void
unmap(Stack s)
{ if ( new_stack_size(s) < s->max - s->base )
    mapOrOutOf(s);
}

#else /* O_SHM_ALIGN_FAR_APART */

void
mapOrOutOf(Stack s)
{ int id;
  char *rval;
  long len;
  caddress addr;

  len  = (s->segment_top <= s->segment_double
	  		? s->segment_initial << (s->segment_top)
	  		: s->segment_initial << s->segment_double);
  addr = s->segments[s->segment_top].base;

  if ( (id=shmget(IPC_PRIVATE, len, S_IRUSR|S_IWUSR)) < 0 )
  { if ( errno == EINVAL )
      fatalError("Kernel is not configured with option IPCSHMEM (contact a guru)");
    fatalError("Failed to create shared memory object: %s", OsError());
  }

  if ( (rval = shmat(id, addr, 0)) != (char *) addr )
    fatalError("Failed to map memory at %ld: %s\n", addr, OsError());

  if ( shmctl(id, IPC_RMID, NULL) < 0 )
    fatalError("Failed to release shared memory object: %s", OsError());

  s->segment_top++;
  s->max = s->segments[s->segment_top].base = addr+len;
  considerGarbageCollect(s);
}


static void
unmap(Stack s)
{ while( s->segment_top > 0 && s->segments[s->segment_top-1].base > s->top )
  { s->segment_top--;
    if ( shmdt(s->segments[s->segment_top].base) < 0 )
      fatalError("Failed to unmap: %s\n", OsError());
    s->max = s->segments[s->segment_top].base;
  }
}

#endif /* O_SHM_ALIGN_FAR_APART */
#endif /* O_SHARED_MEMORY */

#ifdef SIGNAL_HANDLER_PROVIDES_ADDRESS
static bool
expandStack(Stack s, caddress addr)
{ if ( addr < s->max || addr >= addPointer(s->limit, STACK_SEPARATION) )
    fail;				/* outside this area */

  if ( addr <= s->max + STACK_SEPARATION*2 )
  { mapOrOutOf(s);

    succeed;
  }

  fail;
}
#endif /*O_SHARED_MEMORY*/

#ifdef HAVE_VIRTUAL_ALLOC

#undef FD_ZERO
#undef FD_ISSET
#undef FD_SET
#include <windows.h>
#undef small

static void
mapOrOutOf(Stack s)
{ ulong incr;
  long  newroom;

  if ( s->top > s->max )
    incr = ROUND(((ulong)s->top - (ulong)s->max), size_alignment);
  else
    incr = size_alignment;

  newroom = (ulong)s->limit - ((ulong)s->max + incr);
  if ( newroom < 0 )
    outOfStack(s, STACK_OVERFLOW_FATAL);

  if ( VirtualAlloc(s->max, incr,
		    MEM_COMMIT, PAGE_READWRITE ) != s->max )
    fatalError("VirtualAlloc() failed at 0x%x for %d bytes: %d\n",
	       s->max, incr, GetLastError());

  DEBUG(1, Sdprintf("mapped %d bytes from 0x%x to 0x%x\n",
		    incr, (unsigned) s->max,
		    (ulong) s->max + size_alignment));

  s->max = addPointer(s->max, incr);

  if ( newroom <= STACK_SIGNAL )
  { if ( newroom <= STACK_RESERVE )
    {
#ifndef NO_SEGV_HANDLING
      LD->current_signal = SIGSEGV;
#endif
      outOfStack(s, STACK_OVERFLOW_SIGNAL_IMMEDIATELY);
    } else
      outOfStack(s, STACK_OVERFLOW_SIGNAL);
  }

  considerGarbageCollect(s);
}


#ifdef NO_SEGV_HANDLING
void
ensureRoomStack(Stack s, int bytes)
{ while((char *)s->max - (char *)s->top < (int)bytes)
    mapOrOutOf(s);
}
#endif

static void
unmap(Stack s)
{ caddress top  = (s->top > s->min ? s->top : s->min);
  caddress addr = (caddress) align_size((long) top + size_alignment);

  if ( addr < s->max )
  { if ( !VirtualFree(addr, (ulong)s->max - (ulong)addr, MEM_DECOMMIT) )
      fatalError("Failed to unmap memory: %d", GetLastError());
    s->max = addr;
  }
}


#define MAX_VIRTUAL_ALLOC (100 MB)
#define SPECIFIC_INIT_STACK 1

static void
initStacks(long local, long global, long trail, long argument)
{ SYSTEM_INFO info;
  int large = 0;			/* number of `large' stacks */
  ulong base;				/* allocation base */
  ulong totalsize;			/* total size to allocate */

  GetSystemInfo(&info);
  size_alignment = info.dwPageSize;
  base_alignment = size_alignment;
/*base_alignment = info.dwAllocationGranularity;*/

  local    = (long) align_size(local);	/* Round up to page boundary */
  global   = (long) align_size(global);
  trail    = (long) align_size(trail);
  argument = (long) align_size(argument);

  if ( local    == 0 ) large++;		/* find dynamic ones */
  if ( global   == 0 ) large++;
  if ( trail    == 0 ) large++;
  if ( argument == 0 ) large++;

  if ( large )
    totalsize = MAX_VIRTUAL_ALLOC;
  else
    totalsize = local + global + trail + argument + 4 * STACK_SEPARATION;

  if ( !(base = (ulong) VirtualAlloc(NULL, totalsize,
				     MEM_RESERVE, PAGE_READWRITE)) )
    fatalError("Failed to allocate stacks for %d bytes: %d",
	       totalsize, GetLastError());

  if ( large )
  { ulong space = totalsize -
           ( align_base(local + STACK_SEPARATION) +
	     align_base(global + STACK_SEPARATION) +
	     align_base(trail + STACK_SEPARATION) +
	     align_base(argument) );
    ulong large_size = ((space / large) / base_alignment) * base_alignment;

    if ( large_size < STACK_MINIMUM )
      fatalError("Can't fit requested stack sizes in address space");
    DEBUG(1, Sdprintf("Large stacks are %ld\n", large_size));

    if ( local    == 0 ) local    = large_size;
    if ( global   == 0 ) global   = large_size;
    if ( trail    == 0 ) trail    = large_size;
    if ( argument == 0 ) argument = large_size;
  }

#define INIT_STACK(name, print, limit, minsize) \
  DEBUG(1, Sdprintf("%s stack at 0x%x; size = %ld\n", print, base, limit)); \
  init_stack((Stack) &LD->stacks.name, print, (caddress) base, limit, minsize); \
  base += limit + STACK_SEPARATION; \
  base = align_base(base);
#define K * 1024

  INIT_STACK(global,   "global",   global,   16 K);
  INIT_STACK(local,    "local",    local,    8 K);
  INIT_STACK(trail,    "trail",    trail,    8 K);
  INIT_STACK(argument, "argument", argument, 1 K);

#ifndef NO_SEGV_HANDLING
  set_sighandler(SIGSEGV, (handler_t) segv_handler);
#endif
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Reset the stacks after an abort
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void
resetStacks()
{ emptyStacks();

  trimStacks();
}

#endif /*HAVE_VIRTUAL_ALLOC*/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This the the signal handler for segmentation faults if we are using  MMU
controlled  stacks.   The  only  argument  we  are  interested in is the
address of the segmentation fault.  SUN provides this via  an  argument.
If   your   system   does   not   provide   this  information,  set  the
SIGNAL_HANDLER_PROVIDES_ADDRESS flag.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef NO_SEGV_HANDLING
static RETSIGTYPE
#ifdef SIGNAL_HANDLER_PROVIDES_ADDRESS
segv_handler(int sig, int type, SignalContext scp, char *addr)
#else
segv_handler(int sig)
#endif
{ Stack stacka = (Stack) &LD->stacks;
  int i;

#ifndef SIGNAL_HANDLER_PROVIDES_ADDRESS
  int mapped = 0;

  DEBUG(1, Sdprintf("Page fault.  Free room (g+l+t) = %ld+%ld+%ld\n",
		    roomStack(global), roomStack(local), roomStack(trail)));

  for(i=0; i<N_STACKS; i++)
  { long r = (ulong)stacka[i].max - (ulong)stacka[i].top;

    if ( r < size_alignment )
    { DEBUG(1, Sdprintf("Mapped %s stack (free was %d)\n", stacka[i].name, r));
      mapOrOutOf(&stacka[i]);
      mapped++;
    }
  }

  if ( mapped )
    return;

#else /*SIGNAL_HANDLER_PROVIDES_ADDRESS*/

  DEBUG(1, Sdprintf("Page fault at %ld (%p)\n", (long) addr, addr));
  for(i=0; i<N_STACKS; i++)
  { if ( expandStack(&stacka[i], addr) )
      return;
  }
#endif /*SIGNAL_HANDLER_PROVIDES_ADDRESS*/

  pl_signal_handler(sig);
}

#endif /*NO_SEGV_HANDLING*/

static void
init_stack(Stack s, char *name, caddress base, long limit, long minsize)
{ s->name       = name;
  s->base       = s->max = s->top = base;
  s->limit	= addPointer(base, limit);
  s->min        = (caddress)((ulong)s->base + minsize);
  s->gced_size  = 0L;			/* size after last gc */
  gcPolicy(s, GC_FAST_POLICY);
#if O_SHARED_MEMORY
#if O_SHM_ALIGN_FAR_APART
{ int n;

  s->segment_initial = 32 * 1024;
  s->segment_double  = 20;
  for(n=0; n < MAX_STACK_SEGMENTS; n++)
  { s->segments[n].size = 0;
    s->segments[n].base = s->base + base_alignment * n;
  }
}
#else /* O_SHM_ALIGN_FAR_APART */
  s->segment_top     = 0;
  s->segment_initial = 32 * 1024;
  s->segment_double  = 5;
  s->segments[0].base = base;
#endif /* O_SHM_ALIGN_FAR_APART */
#endif /* O_SHARED_MEMORY */

  DEBUG(1, Sdprintf("%-8s stack from 0x%08x to 0x%08x\n",
		    s->name, (ulong)s->base, (ulong)s->limit));

  while(s->max < s->min)
    mapOrOutOf(s);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
initStacks() initialises the stacks structure,  thus  assigning  a  base
address,  a limit and a name to each of the stacks.  Finally it installs
a signal handler for handling  segmentation  faults.   The  segmentation
fault handler will actually create and expand the stacks on segmentation
faults.

The big problem is finding a safe   area  for the stacks. Currently, the
system tries to find an area as far   as possible from the heap, growing
downwards  if  it  can  determine  the    top  of  the  heap-area  using
topOfHeap(). If it cannot, it will work from the current top of the heap
as returned by sbrk(0).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifdef FORCED_MALLOC_BASE
#undef MMAP_MAX_ADDRESS
#undef MMAP_MIN_ADDRESS
#define MMAP_MAX_ADDRESS (FORCED_MALLOC_BASE + 64 MB)
#define MMAP_MIN_ADDRESS (FORCED_MALLOC_BASE + 16 MB)
#endif

#ifndef SPECIFIC_INIT_STACK

static void
initStacks(long local, long global, long trail, long argument)
{ int large = 0;
  ulong base, top, space, large_size, min_space;

  size_alignment = getpagesize();
#ifdef MMAP_STACK
  base_alignment = size_alignment;
  mapfd  = get_map_fd();
#endif
#if O_SHARED_MEMORY
  base_alignment = SHMLBA;
  DEBUG(0, Sdprintf("Shared memory must be aligned to %d (0x%x) bytes\n",
		  base_alignment, base_alignment));
#endif

  local    = (ulong) align_size(local);	/* Round up to page boundary */
  global   = (ulong) align_size(global);
  trail    = (ulong) align_size(trail);
  argument = (ulong) align_size(argument);

  min_space = align_base(1) +
	      align_base(local + STACK_SEPARATION) +
	      align_base(global + STACK_SEPARATION) +
	      align_base(trail + STACK_SEPARATION) +
	      align_base(argument);

  if ( local    == 0 ) large++;		/* find dynamic ones */
  if ( global   == 0 ) large++;
  if ( trail    == 0 ) large++;
  if ( argument == 0 ) large++;

  if ( (top = topOfHeap()) > 0L && !GD->options.heapSize )
  { if ( large > 0 )			/* we have dynamic stacks */
    { base = heap_base;
      space = top - base;
      space -= min_space;
      large++;				/* heap as well */
      large_size = ((space / large+1) / base_alignment) * base_alignment;
      if ( large_size > 64 MB )
	large_size = 64 MB;
      if ( large_size < STACK_MINIMUM )
	fatalError("Can't fit requested stack sizes in address space");
      DEBUG(1, Sdprintf("Large stacks are %ld\n", large_size));

      if ( local    == 0 ) local    = large_size;
      if ( global   == 0 ) global   = large_size;
      if ( trail    == 0 ) trail    = large_size;
      if ( argument == 0 ) argument = large_size;
    }

    base = top - (align_base(1) +
		  align_base(local + STACK_SEPARATION) +
		  align_base(global + STACK_SEPARATION) +
		  align_base(trail + STACK_SEPARATION) +
		  align_base(argument));
    base = align_base_down(base);
  } else				/* we don't know the top */
  { ulong maxdata = dataLimit();

    if ( !GD->options.heapSize )
    { if ( maxdata )
      { large_size = align_base_down((maxdata-min_space)/(large+1));
	large_size = max(large_size, 64 MB);
      } else
	large_size = 64 MB;
	
      GD->options.heapSize = large_size;
    } else
    { large_size = align_base_down((maxdata-min_space)/(large+1));
      large_size = max(large_size, 64 MB);
    }

#ifdef MMAP_MIN_ADDRESS
    base = MMAP_MIN_ADDRESS;
#else
    base = (ulong) align_base((ulong)sbrk(0) + GD->options.heapSize);
#endif

    if ( large > 0 )
    { DEBUG(1, Sdprintf("Large stacks are %ld\n", large_size));
  
      if ( local    == 0 ) local    = large_size;
      if ( global   == 0 ) global   = large_size;
      if ( trail    == 0 ) trail    = large_size;
      if ( argument == 0 ) argument = large_size;
    }
  }

#define INIT_STACK(name, print, limit, minsize) \
  DEBUG(1, Sdprintf("%s stack at 0x%x; size = %ld\n", print, base, limit)); \
  init_stack((Stack) &LD->stacks.name, print, (caddress) base, limit, minsize); \
  base += limit + STACK_SEPARATION; \
  base = align_base(base);
#define K * 1024

  INIT_STACK(global,   "global",   global,   16 K);
  INIT_STACK(local,    "local",    local,    8 K);
  INIT_STACK(trail,    "trail",    trail,    8 K);
  INIT_STACK(argument, "argument", argument, 1 K);

  assert(top == 0L || (ulong)aLimit <= top);

#ifndef NO_SEGV_HANDLING
  set_sighandler(SIGSEGV, (handler_t) segv_handler);
#endif
}

void
resetStacks()
{ emptyStacks();

  trimStacks();
}


#endif /*SPECIFIC_INIT_STACK*/

		/********************************
		*     STACK TRIMMING & LIMITS   *
		*********************************/

static void
gcPolicy(Stack s, int policy)
{ s->gc = ((s == (Stack) &LD->stacks.global ||
	    s == (Stack) &LD->stacks.trail) ? TRUE : FALSE);
  if ( s->gc )
  { s->small  = SMALLSTACK;
    s->factor = 3;
    s->policy = policy;
  } else
  { s->small  = 0;
    s->factor = 0;
    s->policy = 0;
  }
}


word
pl_trim_stacks()
{ trimStacks();

  gcPolicy((Stack) &LD->stacks.global, GC_FAST_POLICY);
  gcPolicy((Stack) &LD->stacks.trail,  GC_FAST_POLICY);

  succeed;
}


#else /* O_DYNAMIC_STACKS */

		/********************************
		*    SIMPLE STACK ALLOCATION    *
		*********************************/

forwards void init_stack(Stack, char *, long, long, long);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
On systems that do not allow us to get access to the MMU (or that do not
have an MMU)  the  stacks  have  fixed  size  and  overflow  checks  are
implemented  in  software.   The stacks are allocated using malloc(). If
you malloc() does not allow you to get more than 64K bytes in one go you
better start looking for another Prolog system (IBM-PC  is  an  example:
why does IBM bring computers on the marked that are 10 years out-of-date
at the moment of announcement?).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

word
pl_trim_stacks()
{ succeed;
}


word
pl_stack_parameter(term_t name, term_t key, term_t old, term_t new)
{ atom_t a, k;
  Stack stack = NULL;
  long *value = NULL;

  if ( PL_get_atom(name, &a) )
  { if ( a == ATOM_local )
      stack = (Stack) &LD->stacks.local;
    else if ( a == ATOM_global )
      stack = (Stack) &LD->stacks.global;
    else if ( a == ATOM_trail )
      stack = (Stack) &LD->stacks.trail;
    else if ( a == ATOM_argument )
      stack = (Stack) &LD->stacks.argument;
  }
  if ( !stack )
    return warning("stack_parameter/4: unknown stack");

  if ( PL_get_atom(key, &k) )
  { if ( k == ATOM_min_free )
      value = &stack->minfree;
  }
  if ( !value )
    return warning("stack_parameter/4: unknown key");

  return setLong(value, "stack_parameter/4", old, new);
}


static void
init_stack(Stack s, char *name, long size, long limit, long minfree)
{ if ( s->base == NULL )
  { fatalError("Not enough core to allocate stacks");
    return;
  }

  s->name 	= name;
  s->top	= s->base;
  s->limit	= addPointer(s->base, limit);
  s->minfree	= minfree;
  s->max	= (char *)s->base + size;
  s->gced_size = 0L;			/* size after last gc */
  s->gc	       = ((s == (Stack) &LD->stacks.global ||
		   s == (Stack) &LD->stacks.trail) ? TRUE : FALSE);
  s->small     = (s->gc ? SMALLSTACK : 0);
}

static void
initStacks(long local, long global, long trail, long arg)
{ long old_heap = GD->statistics.heap;
#if O_SHIFT_STACKS
  long itrail  = 32 K;
  long iglobal = 200 K;
  long ilocal  = 32 K;
#else
  long itrail  = trail;
  long iglobal = global;
  long ilocal  = local;
#endif

  gBase = (Word) malloc(iglobal + sizeof(word) +
			ilocal + sizeof(struct localFrame) +
			MAXARITY * sizeof(word));
  lBase = (LocalFrame)	addPointer(gBase, iglobal+sizeof(word));
  tBase = (TrailEntry)	malloc(itrail);
  aBase = (Word *)	malloc(arg);

  init_stack((Stack)&LD->stacks.global,	  "global",   iglobal, global, 100 K);
  init_stack((Stack)&LD->stacks.local,    "local",    ilocal,  local,   16 K);
  init_stack((Stack)&LD->stacks.trail,    "trail",    itrail,  trail,    8 K);
  init_stack((Stack)&LD->stacks.argument, "argument", arg,     arg,      0 K);

  GD->statistics.heap = old_heap;
}

void
resetStacks()
{ emptyStacks();
}

#endif /* O_DYNAMIC_STACKS */

void
trimStacks()
{
#ifdef O_DYNAMIC_STACKS
  unmap((Stack) &LD->stacks.local);
  unmap((Stack) &LD->stacks.global);
  unmap((Stack) &LD->stacks.trail);
  unmap((Stack) &LD->stacks.argument);
#endif /*O_DYNAMIC_STACKS*/
}
