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

#define O_DEBUG 1
#include "pl-incl.h"

#undef LD
#define LD LOCAL_LD

#ifdef O_PROFILE

#define current    (LD->profile.current) 	/* current call-node */
#define roots      (LD->profile.roots)		/* roots (<spontaneous>) */
#define nodes      (LD->profile.nodes)		/* # nodes created */
#define accounting (LD->profile.accounting)	/* We are accounting */

#define PROFNODE_MAGIC 0x7ae38f24

typedef struct call_node
{ long 		    magic;		/* PROFNODE_MAGIC */
  struct call_node *parent;
  Definition        def;
  ulong		    calls;		/* Calls from the parent */
  ulong		    redos;		/* redos while here */
  ulong		    exits;		/* exits to the parent */
  ulong		    recur;		/* recursive calls */
  ulong		    ticks;		/* time-statistics */
  ulong		    sibling_ticks;	/* ticks in a siblings */
  struct call_node *next;		/* next in siblings chain */
  struct call_node *siblings;		/* my offspring */
} call_node;

static void	freeProfileData(void);
static ulong	collectSiblingsTime(void);


#ifdef __WIN32__

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MS-Windows version
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void profile(long ticks, PL_local_data_t *ld);

static LARGE_INTEGER last_profile;
static HANDLE	     mythread;
static PL_local_data_t *my_LD;
static UINT	     timer;
static long	     virtual_events;
static long	     events;

static long
prof_new_ticks(HANDLE thread)
{ FILETIME created, exit, kernel, user;
  LARGE_INTEGER u;
  long ticks;

  if ( !GetThreadTimes(thread,
		       &created,
		       &exit,
		       &kernel,
		       &user) )
    return -1;				/* Error condition */

  u.LowPart  = user.dwLowDateTime;
  u.HighPart = user.dwHighDateTime;

  ticks = (long)((u.QuadPart - last_profile.QuadPart)/10240);
  last_profile = u;

  virtual_events += ticks;
  events++;

  return ticks;
}

static void CALLBACK
callTimer(UINT id, UINT msg, DWORD dwuser, DWORD dw1, DWORD dw2)
{ long newticks;

  SuspendThread(mythread);		/* stop thread to avoid trouble */
  if ( (newticks = prof_new_ticks(mythread)) )
  { if ( newticks < 0 )			/* Windows 95/98/... */
      newticks = 1;
    profile(newticks, my_LD);
  }
  ResumeThread(mythread);
}


static bool
startProfiler(int how)
{ MMRESULT rval;

  DuplicateHandle(GetCurrentProcess(),
		  GetCurrentThread(),
		  GetCurrentProcess(),
		  &mythread,
		  0,
		  FALSE,
		  DUPLICATE_SAME_ACCESS);

  my_LD = LD;

  if ( prof_new_ticks(mythread) < 0 )
  { printMessage(ATOM_informational,
		 ATOM_profile_no_cpu_time);
  }
  virtual_events = 0;
  events = 0;

  rval = timeSetEvent(10,
		      5,		/* resolution (milliseconds) */
		      callTimer,
		      (DWORD)0,
		      TIME_PERIODIC);
  if ( rval )
    timer = rval;
  else
    return PL_error(NULL, 0, NULL, ERR_SYSCALL, "timeSetEvent");

  LD->profile.profiling = how;

  succeed;
}


void
stopItimer(void)
{ if ( timer )
  { DEBUG(1, Sdprintf("%ld events, %ld virtual\n",
		      events, virtual_events));

    timeKillEvent(timer);
    timer = 0;
  }
  if ( mythread )
  { CloseHandle(mythread);
    mythread = 0;
    my_LD = NULL;
  }
}

#else /*__WIN32__*/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
POSIX version
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

static void profile(int sig);
static struct itimerval value, ovalue;	/* itimer controlling structures */

static bool
startProfiler(void)
{ GET_LD
  set_sighandler(SIGPROF, profile);

  value.it_interval.tv_sec  = 0;
  value.it_interval.tv_usec = 1;
  value.it_value.tv_sec  = 0;
  value.it_value.tv_usec = 1;
  
  if (setitimer(ITIMER_PROF, &value, &ovalue) != 0)
    return PL_error(NULL, 0, MSG_ERRNO, ERR_SYSCALL, setitimer);
  LD->profile.active = TRUE;

  succeed;
}

void
stopItimer(void)
{ GET_LD
  value.it_interval.tv_sec  = 0;
  value.it_interval.tv_usec = 0;
  value.it_value.tv_sec  = 0;
  value.it_value.tv_usec = 0;
  
  if ( !LD->profile.active )
    return;
  if (setitimer(ITIMER_PROF, &value, &ovalue) != 0)
  { warning("Failed to stop interval timer: %s", OsError());
    return;
  }
}

#endif /*__WIN32__*/

static bool
stopProfiler(void)
{ GET_LD
  if ( !LD->profile.active )
    succeed;

  stopItimer();
  LD->profile.active = FALSE;
#ifndef __WIN32__
  set_sighandler(SIGPROF, SIG_IGN);
#endif

  succeed;
}


static 
PRED_IMPL("profiler", 2, profiler, 0)
{ PRED_LD
  int val;

  if ( !PL_unify_bool_ex(A1, LD->profile.active) )
    fail;
  if ( !PL_get_bool_ex(A2, &val) )
    fail;

  if ( val == LD->profile.active )
    succeed;

  if ( val )
    return startProfiler();
  else
    return stopProfiler();
}
	

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Prolog query API:

$prof_sibling_of(?Child, ?Parent)
	Generate hierachy.  If Parent is '-', generate the roots

$prof_node(+Node, -Pred, -Calls, -Redos, -Exits, -TickFraction)

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int
get_node(term_t t, call_node **node ARG_LD)
{ if ( PL_is_functor(t, FUNCTOR_dprof_node1) )
  { term_t a = PL_new_term_ref();
    call_node *n;

    _PL_get_arg(1, t, a);
    if ( PL_get_pointer(a, (void**)&n) )
    { if ( n->magic == PROFNODE_MAGIC )
      { *node = n;
        succeed;
      }
    }
  }

  return PL_error(NULL, 0, NULL, ERR_TYPE, ATOM_profile_node, t);
}


static int
unify_node(term_t t, call_node *node)
{ return PL_unify_term(t,
		       PL_FUNCTOR, FUNCTOR_dprof_node1,
		         PL_POINTER, node);
}


static
PRED_IMPL("$prof_sibling_of", 2, prof_sibling_of, PL_FA_NONDETERMINISTIC)
{ PRED_LD
  call_node *parent;
  call_node *sibling;

  switch( CTX_CNTRL )
  { case FRG_FIRST_CALL:
    { atom_t a;

      if ( !PL_is_variable(A1) )
      { if ( get_node(A1, &sibling PASS_LD) )
	{ if ( sibling->parent )
	    return unify_node(A2, sibling->parent);
	}
	fail;
      } else
      { if ( PL_get_atom(A2, &a) && a == ATOM_minus )
	  sibling = roots;
	else if ( get_node(A2, &parent PASS_LD) )
	  sibling = parent->siblings;
	else 
	  fail;
      }

      if ( !sibling )
	fail;

      goto return_sibling;
    }
    case FRG_REDO:
    { sibling = CTX_PTR;

    return_sibling:
      if ( !unify_node(A1, sibling) )
	fail;
      if ( sibling->next )
	ForeignRedoPtr(sibling->next);
      succeed;
    }
    case FRG_CUTTED:
    default:
      succeed;
  }
}


static
PRED_IMPL("$prof_node", 7, prof_node, 0)
{ PRED_LD
  call_node *n;

  if ( !get_node(A1, &n PASS_LD) )
    fail;

  if ( unify_definition(A2, n->def, 0, 0) &&
       PL_unify_integer(A3, n->calls) &&
       PL_unify_integer(A4, n->redos) &&
       PL_unify_integer(A5, n->exits) &&
       PL_unify_integer(A6, n->recur) &&
       PL_unify_integer(A7, n->ticks) )
    succeed;

  fail;
}


		 /*******************************
		 *	    COLLECT DATA	*
		 *******************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
prof_procedure_data(+Head,
		  -TimeSelf, -TimeSiblings, -Parents, -Siblings)
    Where Parents  = list_of(reference(Head, Calls, Redos))
      And Siblings = list_of(reference(Head, Calls, Redos))
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef struct prof_ref
{ struct prof_ref *next;		/* next in chain */
  Definition def;			/* predicate */
  ulong ticks;
  ulong sibling_ticks;
  ulong calls;				/* calls to/from this predicate */
  ulong redos;				/* redos to/from this predicate */
} prof_ref;


typedef struct
{ Definition def;
  ulong ticks;
  ulong sibling_ticks;
  ulong calls;
  ulong redos;
  ulong recur;
  prof_ref *callers;
  prof_ref *callees;
} node_sum;


static void
free_relatives(prof_ref *r ARG_LD)
{ prof_ref *n;

  for( ; r; r=n)
  { n = r->next;
    freeHeap(r, sizeof(*r));
  }
}


static void
add_parent_ref(node_sum *sum, call_node *self, call_node *parent ARG_LD)
{ prof_ref *r;
  Definition def = parent ? parent->def : NULL;

  sum->calls += self->calls;
  sum->redos += self->redos;

  for(r=sum->callers; r; r=r->next)
  { if ( r->def == def )
    { r->calls += self->calls;
      r->redos += self->redos;
      if ( parent )
      { r->ticks += self->ticks;
	r->sibling_ticks += self->sibling_ticks;
      }

      return;
    }
  }

  r = allocHeap(sizeof(*r));
  r->calls = self->calls;
  r->redos = self->redos;
  if ( parent )
  { r->ticks = self->ticks;
    r->sibling_ticks = self->sibling_ticks;
  }
  r->def = def;
  r->next = sum->callers;
  sum->callers = r;
}


static void
add_sibling_ref(node_sum *sum, call_node *self, call_node *sibling ARG_LD)
{ prof_ref *r;

  for(r=sum->callees; r; r=r->next)
  { if ( r->def == sibling->def )
    { r->calls += sibling->calls;
      r->redos += sibling->redos;
      r->ticks += sibling->ticks;
      r->sibling_ticks += sibling->sibling_ticks;

      return;
    }
  }

  r = allocHeap(sizeof(*r));
  r->calls = sibling->calls;
  r->redos = sibling->redos;
  r->ticks = sibling->ticks;
  r->sibling_ticks = sibling->sibling_ticks;
  r->def = sibling->def;
  r->next = sum->callees;
  sum->callees = r;
}




static int
sumProfile(call_node *n, Definition def, node_sum *sum ARG_LD)
{ call_node *s;
  int count = 0;

  if ( n->def == def )
  { count++;
    sum->ticks += n->ticks;
    sum->sibling_ticks += n->sibling_ticks;
    sum->recur += n->recur;
    add_parent_ref(sum, n, n->parent PASS_LD);
    for(s=n->siblings; s; s = s->next)
      add_sibling_ref(sum, n, s PASS_LD);
  }

  for(s=n->siblings; s; s = s->next)
    count += sumProfile(s, def, sum PASS_LD);

  return count;
}


static int
unify_relatives(term_t list, prof_ref *r ARG_LD)
{ term_t tail = PL_copy_term_ref(list);
  term_t head = PL_new_term_ref();
  term_t tmp = PL_new_term_ref();
  static functor_t FUNCTOR_node5;

  if ( !FUNCTOR_node5 )
    FUNCTOR_node5 = PL_new_functor(PL_new_atom("node"), 5);

  for( ; r; r=r->next)
  { if ( !PL_unify_list(tail, head, tail) )
      fail;

    PL_put_variable(tmp);
    if ( r->def )
      unify_definition(tmp, r->def, 0, 0);
    else
      PL_unify_atom_chars(tmp, "<spontaneous>");

    if ( !PL_unify_term(head, PL_FUNCTOR, FUNCTOR_node5,
			  PL_TERM, tmp,
			  PL_INTEGER, r->ticks,
			  PL_INTEGER, r->sibling_ticks,
			  PL_INTEGER, r->calls,
			  PL_INTEGER, r->redos) )
      fail;
  }

  return PL_unify_nil(tail);
}



static
PRED_IMPL("$prof_procedure_data", 8, prof_procedure_data, PL_FA_TRANSPARENT)
{ PRED_LD
  Procedure proc;
  Definition def;
  node_sum sum;
  call_node *n;
  int rc;
  int count = 0;

  if ( !get_procedure(A1, &proc, 0, GP_FIND) )
    return PL_error(NULL, 0, NULL, ERR_EXISTENCE, ATOM_procedure, A1);
  def = proc->definition;

  memset(&sum, 0, sizeof(sum));
  for(n=roots; n; n=n->next)
    count += sumProfile(n, def, &sum PASS_LD);
  
  if ( count == 0 )
    fail;				/* nothing known about this one */

  rc = ( PL_unify_integer(A2, sum.ticks) &&
	 PL_unify_integer(A3, sum.sibling_ticks) &&
	 PL_unify_integer(A4, sum.calls) &&
	 PL_unify_integer(A5, sum.redos) &&
	 PL_unify_integer(A6, sum.recur) &&
	 unify_relatives(A7, sum.callers PASS_LD) &&
	 unify_relatives(A8, sum.callees PASS_LD)
       );

  free_relatives(sum.callers PASS_LD);
  free_relatives(sum.callees PASS_LD);

  return rc ? TRUE : FALSE;
}


static
PRED_IMPL("$prof_statistics", 4, prof_statistics, 0)
{ PRED_LD
  if ( PL_unify_integer(A1, LD->profile.ticks) &&
       PL_unify_integer(A2, LD->profile.accounting_ticks) &&
       PL_unify_float(A3, LD->profile.time) &&
       PL_unify_integer(A4, nodes) )
    succeed;

  fail;
}


		 /*******************************
		 *	       RESET		*
		 *******************************/

bool
resetProfiler()
{ GET_LD
  stopProfiler();

  freeProfileData();
  LD->profile.ticks = 0;
  LD->profile.accounting_ticks = 0;
  accounting = FALSE;

  succeed;
}


static
PRED_IMPL("reset_profiler", 0, reset_profiler, 0)
{ resetProfiler();

  succeed;
}


		 /*******************************
		 *	     TOPLEVEL		*
		 *******************************/

static
PRED_IMPL("$profile", 1, profile, PL_FA_TRANSPARENT)
{ PRED_LD
  int rc;
  ulong ticks;

  resetProfiler();
  startProfiler();
  LD->profile.time = CpuTime(CPU_USER);
  rc = callProlog(NULL, A1, PL_Q_PASS_EXCEPTION, NULL);
  LD->profile.time = CpuTime(CPU_USER) - LD->profile.time;
  stopProfiler();
  ticks = collectSiblingsTime();

  DEBUG(0, Sdprintf("Created %ld nodes (%ld bytes); %ld ticks\n",
		    nodes, nodes*sizeof(call_node), ticks));

  return rc;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This function is responsible for collection the profiling statistics  at
run time.  It is called by the UNIX interval timer on each clock tick of
the  machine  (every  20  milli seconds).  If profiling is plain we just
increment the profiling tick of the procedure on top of the stack.   For
cumulative  profiling  we  have  to  scan the entire local stack.  As we
don't want to increment each invokation of recursive  functions  on  the
stack  we  maintain a flag on each function.  This flag is set the first
time the function is found on the stack.  If is is found set the profile
counter will not be incremented.  We do a second pass over the frames to
clear the flags again.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#undef LD
#define LD LOCAL_LD

static void
#ifdef __WIN32__
profile(long count, PL_local_data_t *__PL_ld)
{ 
#else /*__WIN32__*/
profile(int sig)
{ GET_LD

#define count 1

#if _AIX
  if ( !LD->profile.active )
    return;
#endif

#if !defined(BSD_SIGNALS) && !defined(HAVE_SIGACTION)
  signal(SIGPROF, profile);
#endif

#endif /*__WIN32__*/
  LD->profile.ticks += count;
  
  if ( accounting )
  { LD->profile.accounting_ticks += count;
  } else if ( current )
  { assert(current->magic == PROFNODE_MAGIC);
    current->ticks += count;
  }

#undef count
}

		 /*******************************
		 *     HIERARCHY ACCOUNTING	*
		 *******************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
profCall(Definition def)
    Make a call from the current node to def.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

call_node *
profCall(Definition def ARG_LD)
{ call_node *node = current;

  accounting = TRUE;
  DEBUG(1, Sdprintf("profCall(%s)\n", predicateName(def)));

  if ( !node )				/* root-node of the profile */
  { for(node = roots; node; node=node->next)
    { if ( node->def == def )
      { node->calls++;
	current = node;
      }
    }

    node = allocHeap(sizeof(*node));
    memset(node, 0, sizeof(*node));
    nodes++;

    node->magic = PROFNODE_MAGIC;
    node->def = def;
    node->calls++;
    node->next = roots;
    roots = node;
    current = node;
    accounting = FALSE;

    return node;
  }

  for( ; node; node = node->parent)
  { if ( node->def == def )
    { node->recur++;

      current = node;
      accounting = FALSE;
      return node;
    }
  }

  for(node=current->siblings; node; node=node->next)
  { if ( node->def == def )
    { current = node;
      node->calls++;
      accounting = FALSE;
      return node;
    }
  }

  node = allocHeap(sizeof(*node));
  memset(node, 0, sizeof(*node));
  nodes++;
  node->magic = PROFNODE_MAGIC;
  node->def = def;
  node->parent = current;
  node->calls++;
  node->next = current->siblings;
  current->siblings = node;
  current = node;
  accounting = FALSE;

  return node;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Exit, resuming execution in node
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void
profExit(struct call_node *node ARG_LD)
{ call_node *n;

  if ( node == PROF_META_NODE )
    return;

  accounting = TRUE;
  assert(!node || node->magic == PROFNODE_MAGIC);

  DEBUG(1, Sdprintf("profExit(%s)\n",
		    node ? predicateName(node->def) : "NULL"));

  for(n=current; n && n != node; n=n->parent)
  { DEBUG(1, Sdprintf("\texit %s\n", predicateName(n->def)));
    n->exits++;
  }
  accounting = FALSE;

  current = node;
}


void
profRedo(struct call_node *node ARG_LD)
{ assert(!node || node->magic == PROFNODE_MAGIC);

  if ( node )
  { DEBUG(1, Sdprintf("profRedo(%s)\n", predicateName(node->def)));
    node->redos++;
  }
  current = node;
}


static ulong
collectSiblingsNode(call_node *n)
{ call_node *s;
  ulong count = 0;

  for(s=n->siblings; s; s=s->next)
  { count += collectSiblingsNode(s);
    n->sibling_ticks = count;
  }

  return count+n->ticks;
}


static ulong
collectSiblingsTime()
{ GET_LD
  ulong count = 0;
  call_node *n;
  
  for(n=roots; n; n=n->next)
  { count += collectSiblingsNode(n);
  }

  return count;
}


static void
freeProfileNode(call_node *node ARG_LD)
{ call_node *n;

  for(n=node->siblings; n; n=n->next)
  { freeProfileNode(n PASS_LD);
  }

  node->magic = 0;
  freeHeap(node, sizeof(*node));
  nodes--;
}


static void
freeProfileData()
{ GET_LD
  call_node *n;
  
  for(n=roots; n; n=n->next)
  { freeProfileNode(n PASS_LD);
  }

  roots = NULL;
  current = NULL;

  assert(nodes == 0);
}

#else /* O_PROFILE */

		 /*******************************
		 *	    NO PROFILER		*
		 *******************************/

void
stopItimer()
{
}

static 
PRED_IMPL("profiler", 2, profiler, 0)
{ return notImplemented("profile", 2);
}

static
PRED_IMPL("reset_profiler", 0, reset_profiler, 0)
{ return notImplemented("reset_profile", 0);
}

static
PRED_IMPL("profile_node", 7, profile_node, 0)
{ return notImplemented("profile_node", 7);
}

static
PRED_IMPL("profile_sibling_of", 2, profile_sibling_of, PL_FA_NONDETERMINISTIC)
{ return notImplemented("profile_sibling_of", 2);
} 

static
PRED_IMPL("$profile", 1, profile, PL_FA_TRANSPARENT)
{ return notImplemented("$profile", 1);
}

static
PRED_IMPL("$prof_procedure_data", 8, prof_procedure_data, PL_FA_TRANSPARENT)
{ return notImplemented("$prof_procedure_data", 7);
}

#endif /* O_PROFILE */

		 /*******************************
		 *      PUBLISH PREDICATES	*
		 *******************************/

BeginPredDefs(profile)
  PRED_DEF("$profile", 1, profile, PL_FA_TRANSPARENT)
  PRED_DEF("profiler", 2, profiler, 0)
  PRED_DEF("reset_profiler", 0, reset_profiler, 0)
  PRED_DEF("$prof_node", 7, prof_node, 0)
  PRED_DEF("$prof_sibling_of", 2, prof_sibling_of, PL_FA_NONDETERMINISTIC)
  PRED_DEF("$prof_procedure_data", 8, prof_procedure_data, PL_FA_TRANSPARENT)
  PRED_DEF("$prof_statistics", 4, prof_statistics, 0)
EndPredDefs
