/*  $Id$

    Copyright (c) 1990 Jan Wielemaker. All rights reserved.
    See ../LICENCE to find out about your rights.
    jan@swi.psy.uva.nl

    Purpose: SWI-Prolog general include file
*/

#ifndef _PL_INCLUDE_H
#define _PL_INCLUDE_H

#ifdef __WIN32__
#define MD	     "config/win32.h"
#define PLHOME       "c:/pl"
#define DEFSTARTUP   ".plrc"
#define ARCH	     "i386-win32"
#define C_LIBS	     "-lreadline -lconsole"
#define C_STATICLIBS ""
#define C_CC	     "msvc++ 4.2"
#define C_LDFLAGS    ""
#else
#include <parms.h>			/* pick from the working dir */
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
First, include config.h or, if MD is  specified, this file.  This allows
for -DMD="md-mswin.h"
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CONFTEST
#ifdef MD
#include MD
#else
#include <config.h>
#endif
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		      PROLOG SYSTEM OPTIONS

These are not really options normally.  They are there because I use  to
add  new  features  conditional  using  #if ... #endif.  In many cases I
leave them in for ducumentation purposes.   Notably  O_STRING  might  be
handy for it someone wants to add a data type to the system.

  O_STRING
      Include data type string.  This  feature  does  not  rely  on  any
      system  feature.   It  hardly has any consequences for the system.
      Because of its experimental nature it is optional.  The definition
      of the predicates operating on strings might change.
      (NOTE: Currently some of the boot files rely on strings. It is NOT
      suggested to leave them out).
  O_COMPILE_OR
      Compile ->/2, ;/2 and |/2 into WAM.  This  no  longer  is  a  real
      option.   the mechanism to handle cuts without compiling ;/2, etc.
      has been taken out.
  O_COMPILE_ARITH
      Include arithmetic compiler (compiles is/2, >/2, etc. into WAM).
  O_PROLOG_FUNCTIONS
      Include evaluatable Prolog functions into the arithmetic module.
  O_BLOCK
      Include support for block/3, !/1, fail/1 and exit/2 in the VM.
  O_LABEL_ADDRESSES
      Means we can pick up the address of a label in  a function using
      the var  = `&&label' construct  and jump to  it using goto *var;
      This construct is known by the GNU-C compiler gcc version 2.  It
      is buggy in gcc-2.0, but seems to works properly in gcc-2.1.
  VMCODE_IS_ADDRESS
      Can only  be set when  O_LABEL_ADDRESSES is  set.  It causes the
      prolog  compiler  to put the  code  (=  label-) addresses in the
      compiled Prolog  code  rather than the  virtual-machine numbers.
      This speeds-up  the vm  instruction dispatching in  interpret().
      See also pl-comp.c
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define PL_KERNEL		1
#define O_COMPILE_OR		1
#define O_SOFTCUT		1
#define O_COMPILE_ARITH		1
#define O_STRING		1
#define O_PROLOG_FUNCTIONS	1
#define O_BLOCK			1
#define O_CATCHTHROW		1
#define O_INLINE_FOREIGNS	1
#define O_PCE			1	/* doesn't depend any longer */
#define O_DEBUGGER		1
#define O_INTERRUPT		1
#define O_DESTRUCTIVE_ASSIGNMENT 1
#define O_HASHTERM		1
#define O_LIMIT_DEPTH		1

#ifndef DOUBLE_TO_LONG_CAST_RAISES_SIGFPE
#ifdef __i386__
#define DOUBLE_TO_LONG_CAST_RAISES_SIGFPE 1
#endif
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
On the MIPS, whether or not alignment can   be used appears to depend on
the compilation flags. We play safe for   the  case the user changes the
flags after running configure.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if !defined(DOUBLE_ALIGNMENT) && defined(__mips__)
#define DOUBLE_ALIGNMENT sizeof(double)
#endif

#if MMAP_STACK || O_SHARED_MEMORY || HAVE_VIRTUAL_ALLOC
#define O_DYNAMIC_STACKS 1		/* sparse memory management */
#else
#ifndef O_SHIFT_STACKS
#define O_SHIFT_STACKS 1		/* use stack-shifter */
#endif
#endif

#ifndef O_LABEL_ADDRESSES
#if __GNUC__ == 2
#define O_LABEL_ADDRESSES	1
#ifndef VMCODE_IS_ADDRESS
#define VMCODE_IS_ADDRESS	1
#endif
#endif
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Runtime version.  Uses somewhat less memory and has no tracer.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifdef O_RUNTIME
#undef O_PROFILE			/* no profiling */
#undef O_DEBUGGER			/* no debugging */
#undef O_READLINE			/* no readline too */
#undef O_INTERRUPT			/* no interrupts too */
#endif


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
The macros below try to establish a common basis for various  compilers,
so  we  can  write  most  of the real code without having to worry about
compiler limits and differences.

The  current  version  has  prototypes  defined   for   all   functions.
Unfortunately  there  are  still a lot of old compilers around and it is
hard to write and maintain code that runs on both old and new compilers.
This has worked on TURBO_C not very long ago.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if (defined(__unix__) || defined(_AIX)) && !defined(unix)
#define unix 1
#endif

/* AIX requires this to be the first thing in the file.  */
#ifdef __GNUC__
# define alloca __builtin_alloca
#else
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
void *alloca ();
#   endif
#  endif
# endif
#endif


#if HAVE_XOS_H
#include <xos.h>
#endif
#ifdef HAVE_UXNT_H
#include <uxnt.h>
#endif

#ifndef CONFTEST
#include "pl-stream.h"
#else
#include <stdio.h>			/* for testing pl-save.c */
#endif

#include <sys/types.h>
#include <setjmp.h>
#ifdef ASSERT_H_REQUIRES_STDIO_H
#include <stdio.h>
#endif /*ASSERT_H_REQUIRES_STDIO_H*/
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>

#ifdef HAVE_SIGNAL
#include <signal.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
#include <string.h>
/* An ANSI string.h and pre-ANSI memory.h might conflict.  */
#if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#include <memory.h>
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */
#else /* not STDC_HEADERS and not HAVE_STRING_H */
#include <strings.h>
/* memory.h and strings.h conflict on some systems.  */
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#if OS2 && EMX
#include <process.h>
#include <io.h>
#endif /* OS2 */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
A common basis for C keywords.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if __GNUC__ && !__STRICT_ANSI__
#define HAVE_INLINE 1
#define HAVE_VOLATILE 1
#endif

#if !defined(HAVE_INLINE) && !defined(inline)
#define inline
#endif

#if defined(__GNUC__) && !defined(register)
#define register
#endif

#ifndef HAVE_VOLATILE
#define volatile
#endif


#if __STRICT_ANSI__
#undef TAGGED_LVALUE
#endif

#if defined(__STRICT_ANSI__) || defined(NO_ASM_NOP)
#define ASM_NOP { static int nop; nop++; }
#endif

#define forwards static		/* forwards function declarations */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Booleans,  addresses,  strings  and other   goodies.   Note that  ANSI
compilers have  `Void'.   This should  be  made part  of  the  general
platform as well.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef int			bool;
typedef double			real;
typedef void *			Void;

#if __GNUC__ && !__STRICT_ANSI__
#define LocalArray(t, n, s)	t n[s]
#else
#define LocalArray(t, n, s)	t *n = (t *) alloca((s)*sizeof(t))
#endif

#define TermVector(name, s)	LocalArray(Word, name, s)

#if HAVE_SIGNAL
#define MAXSIGNAL		32	/* highest system signal number */

typedef RETSIGTYPE (*handler_t)(int);
typedef void *SignalContext;		/* struct sigcontext on sun */

typedef struct
{ handler_t os;				/* Os signal handler */
  handler_t user;			/* User signal handler */
  bool catched;				/* Prolog catches this one */
} sig_handler;
#endif /* HAVE_SIGNAL */

#ifndef TRUE
#define TRUE			1
#define FALSE			0
#endif
#define succeed			return TRUE
#define fail			return FALSE
#define TRY(goal)		if ((goal) == FALSE) fail

#define CL_START		0	/* asserta */
#define CL_END			1	/* assertz */

typedef void *			caddress;

#define EOS			('\0')
#define ESC			((char) 27)
#define streq(s, q)		((strcmp((s), (q)) == 0))

#ifndef abs
#define abs(x)			((x) < 0 ? -(x) : (x))
#endif
				/* n is 2^m !!! */
#define ROUND(p, n)		((((p) + (n) - 1) & ~((n) - 1)))
#define addPointer(p, n)	((Void) ((char *)(p) + (long)(n)))
#define diffPointers(p1, p2)	((char *)(p1) - (char *)(p2))

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			     LIMITS

Below are some arbitrary limits on object sizes.  Feel free  to  enlarge
them.  Descriptions:

	* LINESIZ
	Buffer used to store textual info.  It is not concerned with
	critical things, just things like building an error message,
	reading a command for the tracer, etc.

	* MAXARITY
	Maximum arity of a predicate.  May be enarged further, but
	wastes stack (4 bytes for each argument) on machines that
	use malloc() for allocating the stack as the local and global
	stack need to be apart by this amount.  Also, an interrupt
	skips this amount of stack.
	
	* MAXVARIABLES
	Maximum number of variables in a clause.  May be increased
	further, but the global local stacks need to be separated
	by this amount for the segmentation-fault based stack expansion
	version (just costs virtual memory).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define BUFFER_RING_SIZE 	4	/* foreign buffer ring (pl-fli.c) */
#define LINESIZ			1024	/* size of a data line */
#define MAXARITY		1024	/* arity of predicate */
#define MAXVARIABLES		65536	/* number of variables/clause */
#define OP_MAXPRIORITY		1200	/* maximum operator priority */
#define SMALLSTACK		200 * 1024 /* GC policy */

#define WORDBITSIZE		(8 * sizeof(word))
#define LONGBITSIZE		(8 * sizeof(long))
#define INTBITSIZE		(8 * sizeof(int))

				/* Prolog's integer range */
#define PLMINTAGGEDINT		(-(long)(1L<<(WORDBITSIZE - LMASK_BITS - 1)))
#define PLMAXTAGGEDINT		(-PLMINTAGGEDINT - 1)
#define inTaggedNumRange(n)	(((n)&~PLMAXTAGGEDINT) == 0 || \
				 ((n)&~PLMAXTAGGEDINT) == ~PLMAXTAGGEDINT)
#define PLMININT		((long)(1L<<(WORDBITSIZE-1)))
#define PLMAXINT		(-(PLMININT+1))

#if vax
#define MAXREAL			(1.701411834604692293e+38)
#else					/* IEEE double */
#define MAXREAL			(1.79769313486231470e+308)
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Allocation Parameters
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define ALLOCSIZE	(1<<15)	/* size of allocation chunks (64K) */
#define ALLOCFAST	512	/* big enough for all structures */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Macros to handle hash tables.  See pl-table.c for  details.   First  the
sizes  of  the  hash  tables are defined.  Note that these should all be
2^N.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define ATOMHASHSIZE		1024	/* global atom table */
#define FUNCTORHASHSIZE		512	/* global functor table */
#define PROCEDUREHASHSIZE	256	/* predicates in module user */
#define MODULEPROCEDUREHASHSIZE 16	/* predicates in other modules */
#define RECORDHASHSIZE		16	/* global recorda/recordz table */
#define MODULEHASHSIZE		16	/* global module table */
#define PUBLICHASHSIZE		8	/* Module export table */
#define OPERATORHASHSIZE	32	/* global operator table */
#define FLAGHASHSIZE		16	/* global flag/3 table */
#define ARITHHASHSIZE		16	/* arithmetic function table */

#define stringHashValue(s, n) (unboundStringHashValue(s) & ((n)-1))
#define pointerHashValue(p, size) ((((long)(p) >> LMASK_BITS) ^ \
				    ((long)(p) >> (LMASK_BITS+5)) ^ \
				    ((long)(p))) & \
				   ((size)-1))

#define TABLE_REF_MASK		0x1L
#define isTableRef(p)		((unsigned long)(p) & TABLE_REF_MASK)
#define makeTableRef(p)		((void*)((unsigned long)(p) | TABLE_REF_MASK))
#define unTableRef(s, p)	(*((s*)((unsigned long)(p) & ~TABLE_REF_MASK)))

#define for_table(s, t) for(s = firstHTable(t); s; s = nextHTable(t, s))
#define return_next_table(t, v, clean) \
	{ for((v) = (v)->next; isTableRef(v) && (v); (v) = unTableRef(t, v)) \
	  if ( (v) == (t)NULL ) \
	  { clean; \
	    succeed; \
	  } \
	  ForeignRedoPtr(v); \
	}

#define NEED_REINDEX (1L << (LONGBITSIZE-1)) /* definition reindexing */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Foreign language interface definitions.  Note that these macros MUST  be
consistent  with  the  definitions  in  pl-itf.h, which is included with
users foreign language code.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define FRG_CONTROL_MASK	0x00000003L
#define FRG_CONTROL_BITS	2

#define FRG_FIRST_CALL	(0)		/* Initial call */
#define FRG_CUTTED	(1)		/* Context was cutted */
#define FRG_REDO	(2)		/* Normal redo */

#define FIRST_CALL	(0L)

#define ForeignRedoIntVal(v)	(((unsigned long)(v)<<FRG_CONTROL_BITS)|FRG_REDO)
#define ForeignRedoPtrVal(v)	(((unsigned long)(v))|FRG_REDO)

#define ForeignRedoInt(v)	return ForeignRedoIntVal(v)
#define ForeignRedoPtr(v)	return ForeignRedoPtrVal(v)

#define ForeignControl(h)	((h) & FRG_CONTROL_MASK)
#define ForeignContextInt(h)	((long)(h)>>FRG_CONTROL_BITS)
#define ForeignContextPtr(h)	((void*)((unsigned long)(h)&~FRG_CONTROL_MASK))

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Virtual machine instruction declarations.  Prefixes:

  I_	General instructions
  B_	Body specific version
  H_	Head specific versin
  A_	Arithmetic compilation specific
  C_	Control (compilation of ;/2, etc.)

Numbering these things is arbitrary,  but  for  fast  operation  of  the
switch  in  pl-wam.c,  numbering  should start at 0 and be without gaps.
I_HIGHEST must be made equal to the highest  value  of  the  instruction
codes.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define I_NOP		((code) 0)		/* nop */
#define I_ENTER		((code) 1)		/* enter body */
#define I_CALL		((code) 2)		/* call procedure */
#define I_DEPART	((code) 3)		/* last call of procedure */
#define I_EXIT		((code) 4)		/* exit procedure */
#define B_FUNCTOR	((code) 5)		/* start functor */
#define B_RFUNCTOR	((code) 6)		/* start functor */
#define H_FUNCTOR	((code) 7)
#define H_RFUNCTOR	((code) 8)
#define I_POPF		((code) 9)		/* end functor */
#define B_VAR		((code)10)		/* variable */
#define H_VAR		((code)11)
#define B_CONST		((code)12)		/* constant (atomic) */
#define H_CONST		((code)13)
#define H_INDIRECT	((code)14)		/* indirect in the head */
#define B_INTEGER	((code)15)		/* bignum in the head */
#define H_INTEGER	((code)16)		/* bignum in the body */
#define B_FLOAT		((code)17)		/* double in the head */
#define H_FLOAT		((code)18)		/* double in the body */

#define B_FIRSTVAR	((code)19)		/* first occurrence of var */
#define H_FIRSTVAR	((code)20)
#define B_VOID		((code)21)		/* anonimous variables */
#define H_VOID		((code)22)
#define B_ARGFIRSTVAR	((code)23)		/* body vars nested in funct */
#define B_ARGVAR	((code)24)

#define H_NIL		((code)25)		/* [] in the head */
#define B_NIL		((code)26)		/* [] in the body */
#define H_LIST		((code)27)		/* ./2 in the head */
#define H_RLIST		((code)28)		/* right-recursive list */
#define B_LIST		((code)29)		/* ./2 in the body */
#define B_RLIST		((code)30)		/* right-recursive list */

#define B_VAR0		((code)31)		/* B_VAR 0 */
#define B_VAR1		((code)32)		/* B_VAR 1 */
#define B_VAR2		((code)33)		/* B_VAR 2 */

#define I_USERCALL0	((code)34)		/* variable in body (call/1) */
#define I_USERCALLN	((code)35)		/* call/[2...] */
#define I_CUT		((code)36)		/* ! */
#define I_APPLY		((code)37)		/* apply/2 */

#if O_COMPILE_ARITH
#define A_ENTER		((code)38)		/* start arithmetic sequence */
#define A_INTEGER	((code)39)		/* 32-bit signed int */
#define A_DOUBLE	((code)40)		/* 64-bit double */
#define A_VAR0		((code)41)		/* variable-0 */
#define A_VAR1		((code)42)		/* variable-1 */
#define A_VAR2		((code)43)		/* variable-2 */
#define A_VAR		((code)44)		/* variable-n */
#define A_FUNC0		((code)45)		/* nullary arithmic function */
#define A_FUNC1		((code)46)		/* unary arithmic function */
#define A_FUNC2		((code)47)		/* binary arithmic function */
#define A_FUNC		((code)48)		/* n-ary arithmic function */
#define A_LT		((code)49)		/* < */
#define A_GT		((code)50)		/* > */
#define A_LE		((code)51)		/* =< */
#define A_GE		((code)52)		/* >= */
#define A_EQ		((code)53)		/* =:= */
#define A_NE		((code)54)		/* =\= */
#define A_IS		((code)55)		/* is */
#endif /* O_COMPILE_ARITH */

#if O_COMPILE_OR
#define C_OR		((code)56)		/* In-clause backtract point */
#define C_JMP		((code)57)		/* Jump over code */
#define C_MARK		((code)58)		/* Sub-clause cut mark */
#define C_CUT		((code)59)		/* cut to corresponding mark */
#define C_IFTHENELSE	((code)60)		/* if-then-else start */
#define C_VAR		((code)61)		/* make a variable */
#define C_END		((code)62)		/* dummy to help decompiler */
#define C_NOT		((code)63)		/* same as C_IFTHENELSE */
#define C_FAIL		((code)64)		/* fail */
#endif /* O_COMPILE_OR */

#define B_INDIRECT	((code)65)		/* INDIRECT in body */

#if O_BLOCK
#define I_CUT_BLOCK	((code)66)		/* !(block) */
#define B_EXIT		((code)67)		/* exit(block, rval) */
#endif /*O_BLOCK*/

#if O_INLINE_FOREIGNS
#define I_CALL_FV0	((code)68)		/* call foreign, no args */
#define I_CALL_FV1	((code)69)		/* call foreign, 1 var arg */
#define I_CALL_FV2	((code)70)		/* call foreign, 2 var args */
#endif /*O_INLINE_FOREIGNS*/

#define I_FAIL		((code)71)		/* fail */
#define I_TRUE		((code)72)		/* true */

#ifdef O_SOFTCUT
#define C_SOFTIF	((code)73)		/* Start A *-> B ; C */
#define C_SOFTCUT	((code)74)		/* `Cut' of A *-> B ; C */
#endif /*O_SOFTCUT*/

#define I_EXITFACT	((code)75)		/* exit from a fact */
#define D_BREAK		((code)76)		/* Debugger break-point */

#if O_CATCHTHROW
#define B_THROW		((code)77)		/* throw(Exception) */
#endif

#define I_HIGHEST	((code)77)		/* largest WAM code !!! */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Arithmetic comparison
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define LT 1
#define GT 2
#define LE 3
#define GE 4
#define NE 5
#define EQ 6

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Operator types
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define	OP_FX	0
#define OP_FY	1
#define OP_XF	2
#define OP_YF	3
#define OP_XFX	4
#define OP_XFY	5
#define OP_YFX	6
#define	OP_YFY	7

#define OP_PREFIX  1
#define OP_INFIX   2
#define OP_POSTFIX 3

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Files and streams.  Don't change the numbers, or change FOPENMODE below.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define F_CLOSED	0		/* closed entry */
#define F_READ		1		/* open for reading */
#define F_WRITE		2		/* open for writing */
#define F_APPEND	3		/* open for append writing */
#define F_WRNOTRUNC	4		/* open for write without truncate */
#define F_ANY		5		/* any mode (finding stream) */

#define FOPENMODE "-rwau"		/* Sopen_file() mode argument */

#define streamOutput(stream, goal) \
	{ int SOn = streamNo(stream, F_WRITE); \
	  int SOout = Output; \
	  word SOrval; \
	  if ( SOn < 0 ) fail; \
	  Output = SOn; \
	  LockStream(); \
	  SOrval = goal; \
	  UnlockStream(); \
	  Output = SOout; \
	  return SOrval; \
	}

#define streamInput(stream, goal) \
	{ int SOn = streamNo(stream, F_READ); \
	  int SOin = Input; \
	  word SOrval; \
	  if ( SOn < 0 ) fail; \
	  Input = SOn; \
	  SOrval = goal; \
	  Input = SOin; \
	  return SOrval; \
	}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Type fields.  These codes are  included  in  a  number  of  the  runtime
structures  at  a  fixed  point, so the runtime environment can tell the
difference.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define HeapMagic(n)	((n) | 0x25678000)
#define PROCEDURE_TYPE	HeapMagic(1)	/* a procedure */
#define RECORD_TYPE	HeapMagic(2)	/* a record list */
#define StackMagic(n)	((n) | 0x98765000)
#define QID_MAGIC	StackMagic(1)	/* Query frame */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			  PROLOG DATA REPRESENTATION

Prolog data objects live on various places:

	- In the variable and argument slots of environment frames.
	- As arguments to complex terms on the global stack.
	- In records (recorda/recorded database) in the heap.
	- In variables in foreign language functions.

All Prolog data is packed into a `word'.  A word is  a  32  bit  entity.
The top 3 bits are used to indicate the type; the bottom 2 bits are used
for  the  garbage  collector.   The  bits  for the garbage collector are
always 0 during normal execution.  This implies we do not have  to  care
about  them  for  pointers  and  as  pointers  always  point  to 4 bytes
entities, the range is not harmed by the garbage collection bits.

The remaining 27 bits can hold a  unique  representation  of  the  value
itself  or  can be a pointer to the global stack where the real value is
stored.  We call the latter type of data `indirect'.

Below is a description of the  representation  used  for  each  type  of
Prolog data:

INTEGER
    Integers are stored in the  27  remaining  bits  of  a  word.   This
    implies they are limited to +- 2^26.
REAL
    For a real, the 27 bits are a pointer to a 8 byte unit on the global
    stack.  For both words of the 8 byte unit, the top 3  and  bottom  2
    bits  are  reserved  for identification and garbage collection.  The
    remaining bits hold the exponent and mantisse.  See pack_real()  and
    unpack_real() in pl-alloc.c for details.
ATOM
    For atoms, the 27 bits represent a pointer  to  an  atom  structure.
    Atom  structures are cells of a hash table.  Equality of the pointer
    implies equality of the atoms and visa versa.  Atom  structures  are
    not  collected by the garbage collector and thus live for the entire
    Prolog session.
STRING
    For a string, the 27 bits are a pointer to the  global  stack.   The
    first  word  of  the  string  again reserves  the top 3 and bottom 2
    bits.  The remaining bits indicate the lenght of the  string.   Next
    follows a 0 terminated character string.  Finally a word exactly the
    same  as the header word, to allow the garbage collector to traverse
    the stack downwards and identify the string.
TERM
    For a compound term, the 27 bits are a pointer to the global  stack.
    the  first  word there is a pointer to a functordef structure, which
    determines the name and arity of the  term.   functordef  structures
    are  cells  of  a hash table like atom structures.  They to live for
    the entire Prolog session.  Next, there are just as  many  words  as
    the  arity  of the term, each word representing a normal Prolog data
    object.
VARIABLES
    An unbound variable is represented by NULL.
REFERENCES
    References are the result of sharing variables.   If  two  variables
    must  share,  the one with the shortest livetime is made a reference
    pointer to the other.  This way a tree of reference pointers can  be
    constructed.   The root of the tree is the variable with the longest
    livetime.  To bind the entire tree of variables this root is  bound.
    The  others remain reference pointers.  This implies that ANY prolog
    data object might be a reference  pointer  to  another  Prolog  data
    object,  holding  the  real  value.  To find the real value, a macro
    called deRef() is available.

    The direction of reference pointers is critical.  It MUST  point  in
    the direction of the longest living variable.  If not, the reference
    pointer  will  point  into  the  dark  if  the other end dies.  This
    implies that if both cells are part of an environment frame, the one
    in the child function (closest to the top of the stack)  must  point
    to  the  one in the parent function.  If one is on the local and one
    on the global stack, the  pointer  must  point  towards  the  global
    stack.   Inside  the global stack it is irrelevant.  If backtracking
    destroys a variable, it also will reset the reference towards it  if
    there is one.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "pl-data.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common Prolog objects typedefs.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef unsigned long		term_t;		/* external term-reference */
typedef unsigned long		word;		/* Anonimous 4 byte object */
typedef word *			Word;		/* a pointer to anything */
typedef word			atom_t;		/* encoded atom */
typedef unsigned long		code;		/* bytes codes */
typedef code *			Code;		/* pointer to byte codes */
typedef int			Char;		/* char that can pass EOF */
typedef word			(*Func)();	/* foreign functions */

typedef struct atom *		Atom;		/* atom */
typedef struct functor *	Functor;	/* complex term */
typedef struct functorDef *	FunctorDef;	/* name/arity pair */
typedef struct procedure *	Procedure;	/* predicate */
typedef struct definition *	Definition;	/* predicate definition */
typedef struct clause *		Clause;		/* compiled clause */
typedef struct clause_ref *	ClauseRef;      /* reference to a clause */
typedef struct clause_index *	ClauseIndex;    /* Clause indexing table */
typedef struct clause_chain *	ClauseChain;    /* Chain of clauses in table */
typedef struct operator *	Operator;	/* see pl-op.c, pl-read.c */
typedef struct record *		Record;		/* recorda/3, etc. */
typedef struct recordList *	RecordList;	/* list of these */
typedef struct module *		Module;		/* predicate modules */
typedef struct sourceFile *	SourceFile;	/* file adminitration */
typedef struct list_cell *	ListCell;	/* Anonymous list */
typedef struct table *		Table;		/* (numeric) hash table */
typedef struct symbol *		Symbol;		/* symbol of hash table */
typedef struct localFrame *	LocalFrame;	/* environment frame */
typedef struct queryFrame *	QueryFrame;     /* toplevel query frame */
typedef struct fliFrame *	FliFrame; 	/* FLI interface frame */
typedef struct trail_entry *	TrailEntry;	/* Entry of train stack */
typedef struct data_mark	mark;		/* backtrack mark */
typedef struct index *		Index;		/* clause indexing */
typedef struct stack *		Stack;		/* machine stack */
typedef struct arithFunction * 	ArithFunction;  /* arithmetic function */
typedef struct assoc *		Assoc;		/* pl-bags.c */
typedef struct _varDef *	VarDef;		/* pl-comp.c */
typedef struct extension_cell *	ExtensionCell;  /* pl-ext.c */
typedef struct abort_handle *	AbortHandle;	/* PL_abort_hook() */
typedef struct initialise_handle * InitialiseHandle;
typedef struct tempfile *	TempFile; 	/* pl-os.c */
typedef struct canonical_dir *	CanonicalDir;	/* pl-os.c */
typedef struct on_halt *	OnHalt;		/* pl-os.c */
typedef struct find_data_tag *	FindData; 	/* pl-trace.c */
typedef struct feature *	Feature; 	/* pl-prims.c */


		 /*******************************
		 *	    ARITHMETIC		*
		 *******************************/

typedef struct
{ int	type;				/* type of number */
  union { real  f;			/* value as real */
	  long  i;			/* value as integer */
	  word  w[2];			/* for packing/unpacking the double */
	} value;
} number, *Number;

#define V_INTEGER	0		/* integer (long) value */
#define V_REAL		1		/* just a real */
#define V_EXPLICIT_REAL 3		/* explicely specified real */

#define intNumber(n)	((n)->type == V_INTEGER)
#define floatNumber(n)	((n)->type & V_REAL)

		 /*******************************
		 *	   GET-PROCEDURE	*
		 *******************************/

#define GP_FIND		0		/* find anywhere */
#define GP_FINDHERE	1		/* find in this module */
#define GP_CREATE	2		/* create (in this module) */
#define GP_DEFINE	4		/* define a procedure */
#define GP_RESOLVE	5		/* find defenition */

#define GP_HOW_MASK	0x0ff
#define GP_NAMEARITY	0x100		/* or'ed mask */

		 /*******************************
		 *	      FLAGS		*
		 *******************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Many of the structures have a large number of booleans  associated  with
them.   Early  versions defined these using `unsigned <name> : 1' in the
structure definition.  When I ported SWI-Prolog to a  machine  that  did
not  understand  this  construct  I  decided  to pack all the flags in a
short.  As this allows us to set, clear and test combinations  of  flags
with one operation, it turns out to be faster as well.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define true(s, a)		((s)->flags & (a))
#define false(s, a)		(!true((s), (a)))
#define set(s, a)		((s)->flags |= (a))
#define clear(s, a)		((s)->flags &= ~(a))
#define clearFlags(s)		((s)->flags = 0)

#define NONDETERMINISTIC	(0x00000001L) /* predicate */
#define DISCONTIGUOUS		(0x00000002L) /* predicate */
#define DYNAMIC			(0x00000004L) /* predicate */
#define FOREIGN			(0x00000008L) /* predicate */
#define HIDE_CHILDS		(0x00000010L) /* predicate */
#define MULTIFILE		(0x00000020L) /* predicate */
#define PROFILE_TICKED		(0x00000040L) /* predicate */
#define SPY_ME			(0x00000080L) /* predicate */
#define SYSTEM			(0x00000100L) /* predicate, module */
#define TRACE_ME		(0x00000200L) /* predicate */
#define TRANSPARENT		(0x00000400L) /* predicate */
#define GC_SAFE			(0x00000800L) /* predicate */
#define TRACE_CALL		(0x00001000L) /* predicate */
#define TRACE_REDO		(0x00002000L) /* predicate */
#define TRACE_EXIT		(0x00004000L) /* predicate */
#define TRACE_FAIL		(0x00008000L) /* predicate */
					/* This may be changed later ... */
#define LOCKED			(SYSTEM)      /* predicate */
#define FILE_ASSIGNED		(0x00010000L) /* predicate */
#define VOLATILE		(0x00020000L) /* predicate */
#define AUTOINDEX		(0x00040000L) /* predicate */
#define NEEDSCLAUSEGC		(0x00080000L) /* predicate */
#define NEEDSREHASH		(0x00100000L) /* predicate */
#define ISCASE			(0x00200000L) /* predicate */

#define ERASED			(0x0001) /* clause */
#define UNIT_CLAUSE		(0x0002) /* clause */
#define HAS_BREAKPOINTS		(0x0004) /* clause */

#define UNKNOWN			(0x0002) /* module */

#define INLINE_F		(0x0001) /* functor */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Handling environment (or local stack) frames.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define FR_LEVEL		(~0xffL)
#define FR_CUT			(0x01L)
#define FR_NODEBUG		(0x02L)
#define FR_SKIPPED		(0x04L)
#define FR_MARKED		(0x08L)
#define FR_CHOICEPT		(0x10L)
#define FR_WATCHED		(0x20L)

#define ARGOFFSET		((int) sizeof(struct localFrame))

#define setLevelFrame(fr, l)	{ (fr)->flags &= ~FR_LEVEL;   \
				  (fr)->flags |= ((l) << 8); \
				}
#define levelFrame(fr)		(fr->flags >> 8)
#define incLevel(fr)		(fr->flags += 0x100)
#define argFrameP(f, n)		((Word)((f)+1) + (n))
#define argFrame(f, n)		(*argFrameP((f), (n)) )
#define varFrameP(f, n)		((Word)(f) + (n))
#define varFrame(f, n)		(*varFrameP((f), (n)) )
#define refFliP(f, n)		((Word)((f)+1) + (n))
#define parentFrame(f)		((f)->parent ? (f)->parent\
					     : (LocalFrame)varFrame((f), -1))
#define slotsFrame(f)		(true((f)->predicate, FOREIGN) ? \
				      (f)->predicate->functor->arity : \
				      (f)->clause->clause->prolog_vars)
#define contextModule(f)	((f)->context)

#if O_DEBUG
#define enterDefinition(def)	{ def->references++; }
#define leaveDefinition(def)	{ if ( --def->references == 0 && \
				       true(def, NEEDSCLAUSEGC) ) \
				    gcClausesDefinition(def); \
				  if ( def->references < 0 ) \
				  { Sdprintf("%s: reference count < 0\n", \
					     predicateName(def)); \
				    def->references = 0; \
				  } \
				}
#else /*O_DEBUG*/
#define enterDefinition(def)	{ def->references++; }
#define leaveDefinition(def)	{ if ( --def->references == 0 && \
				       true(def, NEEDSCLAUSEGC) ) \
				    gcClausesDefinition(def); \
				}
#endif /*O_DEBUG*/

#ifdef O_DEBUGGER
#define leaveFrame(fr) { if ( true(fr->predicate, FOREIGN) ) \
			 { if ( true(fr->predicate, NONDETERMINISTIC) ) \
			     leaveForeignFrame(fr); \
			 } else \
			   leaveDefinition(fr->predicate); \
			 if ( true(fr, FR_WATCHED) ) \
			   frameFinished(fr); \
		       }
#else /*O_DEBUGGER*/
#define leaveFrame(fr) { if ( true(fr->predicate, FOREIGN) ) \
			 { if ( true(fr->predicate, NONDETERMINISTIC) ) \
			     leaveForeignFrame(fr); \
			 } else \
			   leaveDefinition(fr->predicate); \
		       }
#endif /*O_DEBUGGER*/

#define INVALID_TRAILTOP  ((word)~0L)
#define INVALID_GLOBALTOP ((word)~1L)

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Heuristics functions to determine whether an integer reference passed to
erase and assert/2, clause/3, etc.  really points to a clause or record.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define inCore(a)	((char *)(a) >= hBase && (char *)(a) <= hTop)
#define isProcedure(w)	(((Procedure)(w))->type == PROCEDURE_TYPE)
#define isRecordList(w)	(((RecordList)(w))->type == RECORD_TYPE)
#define isClause(c)	(inCore(((Clause)(c))->procedure) && \
			  isProcedure(((Clause)(c))->procedure))
#define isRecord(r)	(inCore(((Record)(r))->list) && \
			  isRecordList(((Record)(r))->list))

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
At times an abort is not allowed because the heap  is  inconsistent  the
programmer  should  call  startCritical  to start such a code region and
endCritical to end it.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define startCritical (void)(GD->critical++)
#define endCritical   if ( --(GD->critical) == 0 && LD->aborted ) \
			pl_abort()

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LIST processing macros.

    isNil(w)		word is the nil list ([]).
    isList(w)		word is a './2' term.
    HeadList(p)		Pointer to the head of list *p (NOT dereferenced).
    TailList(p)		Pointer to the tail of list *p (NOT dereferenced).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define HeadList(p)	(argTermP(*(p), 0) )
#define TailList(p)	(argTermP(*(p), 1) )

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Structure declarations that must be shared across multiple files.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

struct atom
{ Atom		next;		/* next in chain */
  word		atom;		/* as appearing on the global stack */
#ifdef O_HASHTERM
  int		hash_value;	/* hash-key value */
#endif
  char *	name;		/* name associated with atom */
};

struct index
{ unsigned long key;		/* key of index */
  unsigned long varmask;	/* variable field mask */
};

struct functorDef
{ FunctorDef	next;		/* next in chain */
  word		functor;	/* as appearing on the global stack */
  word		name;		/* Name of functor */
  int		arity;		/* arity of functor */
  unsigned short	flags;	/* Flag field holding: */
		/* INLINE_F	   Inlined foreign (system) predicate */
};

struct clause
{ Procedure	procedure;	/* procedure we belong to */
  Code		codes;		/* byte codes of clause */
  struct index	index;		/* index key of clause */
  short		code_size;	/* size of byte code array */
  short		variables;		/* # of variables for frame */
  short		prolog_vars;		/* # real Prolog variables */
  unsigned short	line_no;	/* Source line-number */
  unsigned short	source_no;	/* Index of source-file */
  unsigned short	flags;		/* Flag field holding: */
		/* ERASED	   Clause is retracted, but referenced */
};

struct clause_ref
{ Clause	clause;
  ClauseRef	next;
};

#define CA1_PROC	1	/* code arg 1 is procedure */
#define CA1_FUNC	2	/* code arg 1 is functor */
#define CA1_DATA	3	/* code arg 2 is prolog data */
#define CA1_INTEGER	4	/* long value */
#define CA1_FLOAT	5	/* next 2 are double */
#define CA1_STRING	6	/* inlined string */

typedef struct
{ char		*name;		/* name of the code */
  code		code;		/* number of the code */
  char		arguments;	/* # arguments code takes */
  char		argtype;	/* # `external' arguments code takes */
} code_info;

struct data_mark
{ word		trailtop;	/* top of the trail stack */
  word		globaltop;	/* top of the global stack */
};

struct functor
{ word		definition;	/* Tagged definition pointer */
  word		arguments[1];	/* arguments vector */
};

struct operator
{ Operator	next;		/* next of chain */
  atom_t	name;		/* name of operator */
  short		type;		/* OP_FX, ... */
  short		priority;	/* priority of operator */
};

struct procedure
{ Definition	definition;	/* definition of procedure */
  int		type;		/* PROCEDURE_TYPE */
};

struct clause_index
{ int		buckets;		/* # entries */
  int		size;			/* # elements (clauses) */
  int		alldirty;		/* all chains need checked */
  ClauseChain 	entries;		/* chains holding the clauses */
};

struct clause_chain
{ ClauseRef	head;
  ClauseRef	tail;
  int		dirty;			/* # of garbage clauses */
};

struct definition
{ FunctorDef	functor;		/* Name/Arity of procedure */
  union
  { ClauseRef	clauses;		/* clause list of procedure */
    Func	function;		/* function pointer of procedure */
  } definition;
  ClauseRef	lastClause;		/* last clause of list */
  Module	module;			/* module of the predicate */
  int		references;		/* reference count */
#ifdef O_PROFILE
  int		profile_ticks;		/* profiler: call times active */
  int		profile_calls;		/* profiler: number of calls */
  int		profile_redos;		/* profiler: number of redos */
  int		profile_fails;		/* profiler: number of fails */
#endif /* O_PROFILE */
  ClauseIndex 	hash_info;		/* clause hash-tables */
  unsigned long indexPattern;		/* indexed argument pattern */
  unsigned long	flags;			/* booleans: */
		/*	FOREIGN		   foreign predicate? */
		/*	PROFILE_TICKED	   has been ticked this time? */
		/*	TRACE_ME	   is my call visible? */
		/*	HIDE_CHILDS	   hide childs for the debugger? */
		/*	SPY_ME		   spy point set? */
		/*	DYNAMIC		   dynamic predicate? */
		/*	MULTIFILE	   defined over more files? */
		/*	SYSTEM		   system predicate */
		/*	TRANSPARENT	   procedure transparent to modules */
		/*	DISCONTIGUOUS	   procedure might be discontiguous */
		/*	NONDETERMINISTIC   deterministic foreign (not used) */
		/*	GC_SAFE		   Save to perform GC while active */
		/*	TRACE_CALL	   Trace call-port */
		/*	TRACE_REDO	   Trace redo-port */
		/*	TRACE_EXIT	   Trace exit-port */
		/*	TRACE_FAIL	   Trace fail-port */
		/*	VOLATILE	   Don't save my clauses */
		/*	AUTOINDEX	   Automatically guess index */
		/*	NEEDSCLAUSEGC	   Clauses have been erased */
		/*	NEEDSREHASH	   Hash-table is out-of-date */
		/*	ISCASE		   Unused.  What is this? */
  unsigned	indexCardinality : 8;	/* cardinality of index pattern */
  short		number_of_clauses;	/* number of associated clauses */
};

struct localFrame
{ Code		programPointer;	/* pointer into program */
  LocalFrame	parent;		/* parent local frame */
  ClauseRef	clause;		/* Current clause of frame */
  LocalFrame	backtrackFrame;	/* Frame for backtracking */
  Definition	predicate;	/* Predicate we are running */
  mark		mark;		/* data backtrack mark */
  Module	context;	/* context module of frame */
  unsigned long flags;		/* packet long holding: */
		/*	LEVEL	   recursion level (28 bits) */
		/*	FR_CUT     has frame been cut ? */
		/*	FR_NODEBUG don't debug this frame ? */
};  


#define QF_NODEBUG		0x0001	/* debug-able query */
#define QF_DETERMINISTIC	0x0002	/* deterministic success */
#define	QF_INTERACTIVE		0x0004	/* interactive goal (prolog()) */

struct queryFrame
{ long		magic;			/* Magic code for security */
#if O_SHIFT_STACKS
  struct				/* Interpreter registers */
  { LocalFrame  fr;
    LocalFrame  bfr;
  } registers;
#endif
#ifdef O_LIMIT_DEPTH
  long		saved_depth_limit;	/* saved values of these */
  long		saved_depth_reached;
#endif
#if O_CATCHTHROW
  term_t	exception;		/* Exception term */
#endif
  unsigned long	flags;
  int		debugSave;		/* saved debugstatus.debugging */
  Word	       *aSave;			/* saved argument-stack */
  int		solutions;		/* # of solutions produced */
  LocalFrame	bfr;			/* BacktrackFrame */
  LocalFrame	saved_environment;	/* Parent local-frame */
  struct localFrame frame;		/* The local frame */
};


struct fliFrame
{ int		size;			/* # slots on it */
  FliFrame	parent;			/* parent FLI frame */
  mark		mark;			/* data-stack mark */
};

struct record
{ RecordList	list;			/* list I belong to */
  Record	next;			/* next of chain */
  int		nvars;			/* # variables in the term */
  int		gsize;			/* Stack space required (words) */
  int		size;			/* # bytes of the record */
  char 		buffer[1];		/* array holding codes */
};

struct recordList
{ RecordList	next;		/* next record chain with same key */
  int		type;		/* RECORD_TYPE */
  word		key;		/* key of record */
  Record	firstRecord;	/* first record associated with key */
  Record	lastRecord;	/* last record associated with key */
};

struct sourceFile
{ atom_t	name;		/* name of source file */
  SourceFile	next;		/* next of chain */
  int		count;		/* number of times loaded */
  long		time;		/* load time of file */
  ListCell	procedures;	/* List of associated procedures */
  Procedure	current_procedure;	/* currently loading one */
  int		index;		/* index number (1,2,...) */
  bool		system;		/* system sourcefile: do not reload */
};


struct list_cell
{ void *	value;		/* object in the cell */
  ListCell	next;		/* next in chain */
};


struct module
{ word		name;		/* name of module */
  SourceFile	file;		/* file from which module is loaded */
  Table		procedures;	/* predicates associated with module */
  Table		public;		/* public predicates associated */
  Module	super;		/* Import predicates from here */
  unsigned short flags;		/* booleans: */
		/*	SYSTEM	   system module */
		/*	UNKNOWN	   trap unknown predicates */
};

struct trail_entry
{ word		address;	/* Tagged address of the variable */
};

struct table
{ int		buckets;	/* size of hash table */
  int		size;		/* # symbols in the table */
  int		locked;		/* locked for resize */
  Symbol	*entries;	/* array of hash symbols */
};

struct symbol
{ Symbol	next;		/* next in chain */
  word		name;		/* name entry of symbol */
  word		value;		/* associated value with name */
};


		 /*******************************
		 *	     MARK/UNDO		*
		 *******************************/

#define setVar(w)		((w) = (word) NULL)

#ifdef O_DESTRUCTIVE_ASSIGNMENT

#define Undo(b)			do_undo(&b)

#else /*O_DESTRUCTIVE_ASSIGNMENT*/

#define Undo(b)		{ TrailEntry tt = tTop; \
			  TrailEntry mt = valPtr2((b).trailtop, STG_TRAIL); \
			  while(tt > mt) \
			  { tt--; \
			    setVar(*valPtr(tt->address)); \
			  } \
			  tTop = tt; \
			  gTop = valPtr2((b).globaltop, STG_GLOBAL); \
			}
#endif /*O_DESTRUCTIVE_ASSIGNMENT*/

#define Mark(b)		{ (b).trailtop  = consPtr(tTop, STG_TRAIL); \
			  (b).globaltop = consPtr(gTop, STG_GLOBAL); \
			}


		 /*******************************
		 *	   FLI INTERNALS	*
		 *******************************/

typedef unsigned long qid_t;		/* external query-id */
typedef unsigned long PL_fid_t;		/* external foreign context-id */

#define fid_t PL_fid_t			/* avoid AIX name-clash */

#define consTermRef(p)	((Word)(p) - (Word)(lBase))
#define valTermRef(r)	(&((Word)(lBase))[r])

#if O_SHIFT_STACKS
#define SaveLocalPtr(s, ptr)	term_t s = consTermRef(ptr)
#define RestoreLocalPtr(s, ptr) (ptr) = (void *) valTermRef(s)
#else
#define SaveLocalPtr(s, ptr)	
#define RestoreLocalPtr(s, ptr) 
#endif

#define QueryFromQid(qid)	((QueryFrame) valTermRef(qid))
#define QidFromQuery(f)		(consTermRef(f))
#define QID_EXPORT_WAM_TABLE	(qid_t)(-1)

#include "pl-itf.h"

		 /*******************************
		 *	   OPTION LISTS		*
		 *******************************/

#define OPT_BOOL	(0)		/* types */
#define OPT_INT		(1)
#define OPT_STRING	(2)
#define OPT_ATOM	(3)
#define OPT_TERM	(4)		/* arbitrary term */

#define OPT_ALL		0x1		/* flags */

typedef struct
{ word		name;			/* Name of option */
  int		type;			/* Type of option */
} opt_spec, *OptSpec;

		 /*******************************
		 *	      EVENTS		*
		 *******************************/

#define PLEV_ERASED	   0 		/* clause or record was erased */
#define PLEV_DEBUGGING	   1		/* changed debugging mode */
#define PLEV_TRACING	   2		/* changed tracing mode */
#define PLEV_SPY	   3		/* changed spypoint */
#define PLEV_BREAK	   4		/* a break-point was set */
#define PLEV_NOBREAK	   5		/* a break-point was cleared */
#define PLEV_FRAMEFINISHED 6		/* A watched frame was discarded */


		/********************************
		*             STACKS            *
		*********************************/

#ifdef small				/* defined by MSVC++ 2.0 windows.h */
#undef small
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
If we have access to the virtual   memory management of the machine, use
this to enlarge the runtime stacks.  Otherwise use the stack-shifter.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define GC_FAST_POLICY 0x1		/* not really used yet */

#if O_SHARED_MEMORY
#define O_SHM_FREE_IMMEDIATE 1
#define MAX_STACK_SEGMENTS  20
#define STACK(type) \
	{ type		base;		/* base address of the stack */     \
	  type		top;		/* current top of the stack */      \
	  type		max;		/* allocated maximum */		    \
	  long		limit;		/* how big it is allowed to grow */ \
	  long		maxlimit;	/* maximum limit */                 \
	  char		*name;		/* Symbolic name of the stack */    \
	  long		segment_initial;/* initial size */		    \
	  int		segment_double;	/* times to double */		    \
	  int		segment_top;	/* Next segment to be allocated */  \
	  struct							    \
	  { caddress	base;		/* Base of this segment */	    \
	    long	size;		/* Size of this segment */	    \
	  } segments[MAX_STACK_SEGMENTS];				    \
	}
#else /* !O_SHARED_MEMORY */
#define STACK(type) \
	{ type		base;		/* base address of the stack */     \
	  type		top;		/* current top of the stack */      \
	  type		min;		/* donot shrink below this value */ \
	  type		max;		/* allocated maximum */		    \
	  type		limit;		/* top the the range (base+limit) */\
	  long		minfree;	/* minimum amount of free space */  \
	  bool		gc;		/* Can be GC'ed? */		    \
	  unsigned long	gced_size;	/* size after last GC */	    \
	  unsigned long	small;		/* Donot GC below this size */	    \
	  unsigned int	factor;		/* How eager we are */		    \
	  int		policy;		/* Time, memory optimization */	    \
	  char		*name;		/* Symbolic name of the stack */    \
	}
#endif

struct stack STACK(caddress);		/* Anonymous stack */

#define N_STACKS (4)

typedef struct
{ struct STACK(LocalFrame) local;	/* local (environment) stack */
  struct STACK(Word)	   global;	/* local (environment) stack */
  struct STACK(TrailEntry) trail;	/* trail stack */
  struct STACK(Word *)	   argument;	/* argument stack */
} pl_stacks_t;

#define tBase	(LD->stacks.trail.base)
#define tTop	(LD->stacks.trail.top)
#define tMax	(LD->stacks.trail.max)

#define lBase	(LD->stacks.local.base)
#define lTop	(LD->stacks.local.top)
#define lMax	(LD->stacks.local.max)
#define lLimit	(LD->stacks.local.limit)

#define gBase	(LD->stacks.global.base)
#define gTop	(LD->stacks.global.top)
#define gMax	(LD->stacks.global.max)
#define gLimit	(LD->stacks.global.limit)

#define aBase	(LD->stacks.argument.base)
#define aTop	(LD->stacks.argument.top)
#define aMax	(LD->stacks.argument.max)
#define aLimit	(LD->stacks.argument.limit)

#define SetHTop(val)	{ if ( (char *)(val) > hTop  ) hTop  = (char *)(val); }
#define SetHBase(val)	{ if ( (char *)(val) < hBase ) hBase = (char *)(val); }

#define onStack(name, addr) \
	((char *)(addr) >= (char *)LD->stacks.name.base && \
	 (char *)(addr) <  (char *)LD->stacks.name.top)
#ifdef O_SHIFT_STACKS
#define onStackArea(name, addr) \
	((char *)(addr) >= (char *)LD->stacks.name.base && \
	 (char *)(addr) <  (char *)LD->stacks.name.max)
#else
#define onStackArea(name, addr) \
	((char *)(addr) >= (char *)LD->stacks.name.base && \
	 (char *)(addr) <  (char *)LD->stacks.name.limit)
#endif
#define usedStack(name) ((char *)LD->stacks.name.top - \
			 (char *)LD->stacks.name.base)
#define sizeStack(name) ((char *)LD->stacks.name.max - \
			 (char *)LD->stacks.name.base)
#define roomStack(name) ((char *)LD->stacks.name.max - \
			 (char *)LD->stacks.name.top)
#define narrowStack(name) (roomStack(name) < LD->stacks.name.minfree)

#if O_DYNAMIC_STACKS
#ifdef NO_SEGV_HANDLING
#define requireStack(s, n) \
	{ if ( roomStack(s) < (int)(n) ) \
 	    ensureRoomStack((Stack)&LD->stacks.s, n); \
	}
#define verifyStack(s) requireStack(s, 64)
#else /*NO_SEGV_HANDLING*/
#define requireStack(s, n)
#define verifyStack(s)
#endif /*NO_SEGV_HANDLING*/
#else
#define requireStack(s, n) \
	{ if ( roomStack(s) < (n) ) \
 	    outOf((Stack)&LD->stacks.s); \
	}
#define verifyStack(s) requireStack(s, 64)
#endif


		/********************************
		*       READ WARNINGS           *
		*********************************/

#define ReadingSource (source_line_no > 0 && \
		       source_file_name != NULL_ATOM)


		/********************************
		*        FAST DISPATCHING	*
		********************************/

#if VMCODE_IS_ADDRESS
#define encode(wam) (wam_table[wam])		/* WAM --> internal */
						/* internal --> WAM */
#define decode(c)   ((code) (dewam_table[(unsigned long)(c) - \
					 dewam_table_offset]))
#else /* VMCODE_IS_ADDRESS */
#define encode(wam) (wam)
#define decode(wam) (wam)
#endif /* VMCODE_IS_ADDRESS */

		/********************************
		*            STATUS             *
		*********************************/

typedef struct
{ bool		requested;		/* GC is requested by stack expander */
  int		blocked;		/* GC is blocked now */
  bool		active;			/* Currently running? */
  long		collections;		/* # garbage collections */
  long		global_gained;		/* global stack bytes collected */
  long		trail_gained;		/* trail stack bytes collected */
  real		time;			/* time spent in collections */
} pl_gc_status_t;


typedef struct
{ int		blocked;		/* No shifts allowed */
  real		time;			/* time spent in stack shifts */
  int		local_shifts;		/* Shifts of the local stack */
  int		global_shifts;		/* Shifts of the global stack */
  int		trail_shifts;		/* Shifts of the trail stack */
} pl_shift_status_t;


typedef struct
{ atom_t	symbolfile;		/* current symbol file */
  atom_t	orgsymbolfile;		/* symbol file we started with */
  atom_t	restored_state;		/* -r/restore state restored */
} pl_loaderstatus_t;

#define NO_PROFILING		0
#define CUMULATIVE_PROFILING	1
#define PLAIN_PROFILING		2


		/********************************
		*            MODULES            *
		*********************************/

#define MODULE_user	(GD->modules.user)
#define MODULE_system	(GD->modules.system)

		/********************************
		*         PREDICATES            *
		*********************************/

#define PROCEDURE_alt1			(GD->procedures.alt1)
#define PROCEDURE_garbage_collect0	(GD->procedures.garbage_collect0)
#define PROCEDURE_block3		(GD->procedures.block3)
#define PROCEDURE_catch3		(GD->procedures.catch3)
#define PROCEDURE_true0			(GD->procedures.true0)
#define PROCEDURE_fail0			(GD->procedures.fail0)
#define PROCEDURE_event_hook1		(GD->procedures.event_hook1)
#define PROCEDURE_print_message2	(GD->procedures.print_message2)

extern const code_info codeTable[]; /* Instruction info (read-only) */

		/********************************
		*            DEBUGGER           *
		*********************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Tracer communication declarations.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define ACTION_CONTINUE	0
#define ACTION_RETRY	1
#define ACTION_FAIL	2
#define ACTION_IGNORE	3
#define ACTION_AGAIN	4

#define CALL_PORT	0x001		/* port masks */
#define EXIT_PORT	0x002
#define FAIL_PORT	0x004
#define REDO_PORT	0x008
#define UNIFY_PORT	0x010
#define BREAK_PORT	0x020
#define CUT_CALL_PORT   0x040
#define CUT_EXIT_PORT   0x080
#define EXCEPTION_PORT	0x100
#define CUT_PORT	(CUT_CALL_PORT|CUT_EXIT_PORT)
#define VERY_DEEP	1000000000L	/* deep skiplevel */

#define LONGATOM_CHECK	    0x1		/* read/1: error on long atoms */
#define SINGLETON_CHECK	    0x2		/* read/1: check singleton vars */
#define DOLLAR_STYLE	    0x4		/* dollar is lower case */
#define DISCONTIGUOUS_STYLE 0x8		/* warn on discontiguous predicates */
#if O_STRING
#define STRING_STYLE	    0x10	/* read ".." as string object */
#endif /* O_STRING */
#define DYNAMIC_STYLE	    0x20	/* warn on assert/retract active */
#define MAXNEWLINES	    5		/* maximum number of newlines in atom */
#define SYSTEM_MODE	    (debugstatus.styleCheck & DOLLAR_STYLE)

typedef struct debuginfo
{ unsigned long	skiplevel;		/* current skip level */
  bool		tracing;		/* are we tracing? */
  bool		debugging;		/* are we debugging? */
  int		leashing;		/* ports we are leashing */
  int	        visible;		/* ports that are visible */
  int		style;			/* print style of tracer */
  bool		showContext;		/* tracer shows context module */
  int		styleCheck;		/* source style checking */
  int		suspendTrace;		/* tracing is suspended now */
  LocalFrame	retryFrame;		/* Frame to retry */
} pl_debugstatus_t;

#define FT_ATOM		0		/* atom feature */
#define FT_INTEGER	1		/* integer feature */

#define CHARESCAPE_FEATURE	0x0001	/* handle \ in atoms */
#define GC_FEATURE		0x0002	/* do GC */
#define TRACE_GC_FEATURE	0x0004	/* verbose gc */
#define TTY_CONTROL_FEATURE	0x0008	/* allow for tty control */
#define READLINE_FEATURE	0x0010	/* readline is loaded */
#define DEBUG_ON_ERROR_FEATURE	0x0020	/* start tracer on error */
#define REPORT_ERROR_FEATURE	0x0040	/* print error message */
#define FILE_CASE_FEATURE	0x0080	/* file names are case sensitive */
#define FILE_CASE_PRESERVING_FEATURE 0x0100 /* case preserving file names */
#define DOS_FILE_NAMES_FEATURE  0x0200	/* dos (8+3) file names */

typedef struct
{ unsigned long flags;			/* the feature flags */
} pl_features_t;

#define trueFeature(mask)	true(&features, mask)

#ifdef O_LIMIT_DEPTH
#define DEPTH_NO_LIMIT	(~0x0L)		/* Highest value */
#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Administration of loaded intermediate code files  (see  pl-wic.c).  Used
with the -c option to include all these if necessary.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef struct state * State;

struct state
{ char *	name;			/* name of state */
  State		next;			/* next state loaded */
};

#define QLF_TOPLEVEL 0x1		/* toplevel wic file */
#define QLF_OPTIONS  0x2		/* only load options */
#define QLF_EXESTATE 0x4		/* probe qlf exe state */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Sourcelocation information (should be used at more places).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef struct
{ atom_t	file;			/* name of the file */
  int		line;			/* line number */
} sourceloc, *SourceLoc;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Include debugging info to make it (very) verbose.  SECURE adds  code  to
check  consistency mainly in the WAM interpreter.  Prolog gets VERY slow
if SECURE is  used.   DEBUG  is  not  too  bad  (about  20%  performance
decrease).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define REL(a)		((Word)(a) - (Word)(lBase))

#if O_DEBUG
#define DEBUG(n, g) { if (GD->debug_level >= (n)) { g; } }
#else
#define DEBUG(a, b) 
#endif

#if O_SECURE
#define SECURE(g) {g;}
#else
#define SECURE(g)
#endif

#include "pl-os.h"			/* OS dependencies */

#ifdef SYSLIB_H
#include SYSLIB_H
#endif

#include "pl-funcs.h"			/* global functions */
#include "pl-main.h"			/* Declarations needed by pl-main.c */
#include "pl-error.h"			/* Exception generation */
#include "pl-global.h"			/* global data */

#define NULL_ATOM ((atom_t)0)
#define MK_ATOM(n)    		((n)<<7|TAG_ATOM|STG_STATIC)
#include "pl-atom.ih"
#include "pl-funct.ih"

#ifdef O_INLINE_FLI

#define PL_new_term_refs(n)	_PL_new_term_refs(n)
#define PL_new_term_ref()	_PL_new_term_ref()
#define PL_reset_term_refs(t)	_PL_reset_term_refs(t)

static inline term_t
_PL_new_term_refs(int n)
{ Word t = (Word)lTop;
  term_t r = consTermRef(t);

  while(n-- > 0)
    setVar(*t++);
  lTop = (LocalFrame)t;
  verifyStack(local);
  
  return r;
}


static inline term_t
_PL_new_term_ref()
{ Word t = (Word)lTop;
  term_t r = consTermRef(t);

  lTop = (LocalFrame)(t+1);
  verifyStack(local);
  setVar(*t);
  
  return r;
}


static inline void
_PL_reset_term_refs(term_t r)
{ lTop = (LocalFrame) valTermRef(r);
}

#endif /*O_INLINE_FLI*/

#endif /*_PL_INCLUDE_H*/
