/*  $Id$

    Copyright (c) 1990 Jan Wielemaker. All rights reserved.
    See ../LICENCE to find out about your rights.
    jan@swi.psy.uva.nl

    Purpose: Virtual machine instruction interpreter
*/

/*#define O_SECURE 1*/
/*#define O_DEBUG 1*/
#include "pl-incl.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SWI-Prolog  new-style  foreign-language  interface.   This  new  foreign
interface is a mix of the old  interface using the ideas on term-handles
from  Quintus  Prolog.  Term-handles  are    integers  (unsigned  long),
describing the offset of the term-location relative   to the base of the
local stack.

If a C-function has to  store  intermediate   results,  it  can do so by
creating a new term-reference using   PL_new_term_ref().  This functions
allocates a cell on the local stack and returns the offset.

While a foreign function is on top of  the stack, the local stacks looks
like this:

						      | <-- lTop
	-----------------------------------------------
	| Allocated term-refs using PL_new_term_ref() |
	-----------------------------------------------
	| reserved for #term-refs (1)		      |
	-----------------------------------------------
	| foreign-function arguments (term-refs)      |
	-----------------------------------------------
	| Local frame of foreign function             |
	-----------------------------------------------

On a call-back to Prolog using  PL_call(),  etc., (1) is filled with the
number of term-refs allocated. This  information   (stored  as  a tagged
Prolog int) is used by the garbage collector to update the stack frames.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if O_SECURE
#define setHandle(h, w)		{ assert(*valTermRef(h) != QID_MAGIC); \
				  (*valTermRef(h) = (w)); \
				}
#else
#define setHandle(h, w)		(*valTermRef(h) = (w))
#endif
#define valHandleP(h)		valTermRef(h)

#undef ulong
#define ulong unsigned long

static inline word
valHandle(term_t r)
{ Word p = valTermRef(r);

  deRef(p);
  return *p;
}


		 /*******************************
		 *	   CREATE/RESET		*
		 *******************************/

#undef PL_new_term_refs
#undef PL_new_term_ref
#undef PL_reset_term_refs

term_t
PL_new_term_refs(int n)
{ Word t = (Word)lTop;
  term_t r = consTermRef(t);

  lTop = (LocalFrame)(t+n);
  verifyStack(local);

  while(n-- > 0)
  { SECURE(assert(*t != QID_MAGIC));
    setVar(*t++);
  }
  
  return r;
}


term_t
PL_new_term_ref()
{ Word t = (Word)lTop;
  term_t r = consTermRef(t);

  lTop = (LocalFrame)(t+1);
  verifyStack(local);
  SECURE(assert(*t != QID_MAGIC));
  setVar(*t);
  
  return r;
}


void
PL_reset_term_refs(term_t r)
{ lTop = (LocalFrame) valTermRef(r);
}


term_t
PL_copy_term_ref(term_t from)
{ Word t   = (Word)lTop;
  term_t r = consTermRef(t);
  Word p2  = valHandleP(from);

  lTop = (LocalFrame)(t+1);
  verifyStack(local);
  deRef(p2);
  *t = isVar(*p2) ? makeRef(p2) : *p2;
  
  return r;
}


		 /*******************************
		 *	       ATOMS		*
		 *******************************/

atom_t
PL_new_atom(const char *s)
{ return (atom_t) lookupAtom((char *)s); /* hack */
}


const char *
PL_atom_chars(atom_t a)
{ return (const char *) stringAtom(a);
}


functor_t
PL_new_functor(atom_t f,  int a)
{ return lookupFunctorDef(f, a);
}


atom_t
PL_functor_name(functor_t f)
{ return nameFunctor(f);
}


int
PL_functor_arity(functor_t f)
{ return arityFunctor(f);
}


		 /*******************************
		 *    QUINTUS WRAPPER SUPPORT   *
		 *******************************/

bool
PL_cvt_i_integer(term_t p, long *c)
{ return PL_get_long(p, c);
}


bool
PL_cvt_i_float(term_t p, double *c)
{ return PL_get_float(p, c);
}


bool
PL_cvt_i_single(term_t p, float *c)
{ double f;

  if ( PL_get_float(p, &f) )
  { *c = (float)f;
    succeed;
  }

  fail;
}


bool
PL_cvt_i_string(term_t p, char **c)
{ return PL_get_chars(p, c, CVT_ATOM|CVT_STRING);
}


bool
PL_cvt_i_atom(term_t p, atom_t *c)
{ return PL_get_atom(p, c);
}


bool
PL_cvt_o_integer(long c, term_t p)
{ return PL_unify_integer(p, c);
}


bool
PL_cvt_o_float(double c, term_t p)
{ return PL_unify_float(p, c);
}


bool
PL_cvt_o_single(float c, term_t p)
{ return PL_unify_float(p, c);
}


bool
PL_cvt_o_string(const char *c, term_t p)
{ return PL_unify_atom_chars(p, c);
}


bool
PL_cvt_o_atom(atom_t c, term_t p)
{ return PL_unify_atom(p, c);
}


		 /*******************************
		 *	      COMPARE		*
		 *******************************/

int
PL_compare(term_t t1, term_t t2)
{ Word p1 = valHandleP(t1);
  Word p2 = valHandleP(t2);

  return compareStandard(p1, p2);	/* -1, 0, 1 */
}


		 /*******************************
		 *	      INTEGERS		*
		 *******************************/

word
makeNum(long i)
{ if ( inTaggedNumRange(i) )
    return consInt(i);

  return globalLong(i);
}


		 /*******************************
		 *	       CONS-*		*
		 *******************************/

void
PL_cons_functor(term_t h, functor_t fd, ...)
{ int arity = arityFunctor(fd);

  if ( arity == 0 )
  { setHandle(h, nameFunctor(fd));
  } else
  { Word a = allocGlobal(1 + arity);
    va_list args;

    va_start(args, fd);
    setHandle(h, consPtr(a, TAG_COMPOUND|STG_GLOBAL));
    *a++ = fd;
    while(arity-- > 0)
    { term_t r = va_arg(args, term_t);
      Word p = valHandleP(r);

      deRef(p);
      *a++ = (isVar(*p) ? makeRef(p) : *p);
    }
    va_end(args);
  }
}


void
PL_cons_list(term_t l, term_t head, term_t tail)
{ Word a = allocGlobal(3);
  Word p;
  
  a[0] = FUNCTOR_dot2;
  p = valHandleP(head);
  deRef(p);
  a[1] = (isVar(*p) ? makeRef(p) : *p);
  p = valHandleP(tail);
  deRef(p);
  a[2] = (isVar(*p) ? makeRef(p) : *p);

  setHandle(l, consPtr(a, TAG_COMPOUND|STG_GLOBAL));
}

		 /*******************************
		 *     POINTER <-> PROLOG INT	*
		 *******************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Pointers are not a special type in Prolog. Instead, they are represented
by an integer. The funtions below convert   integers  such that they can
normally be expressed as a tagged  integer: the heap_base is subtracted,
it is divided by 4 and the low 2   bits  are placed at the top (they are
normally 0). longToPointer() does the inverse operation.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static ulong
pointerToLong(void *ptr)
{ ulong p = (ulong) ptr;
  ulong low = p & 0x3L;

  p -= heap_base;
  p >>= 2;
  p |= low<<(sizeof(ulong)*8-2);
  
  return p;
}


static void *
longToPointer(ulong p)
{ ulong low = p >> (sizeof(ulong)*8-2);

  p <<= 2;
  p |= low;
  p += heap_base;

  return (void *) p;
}


		 /*******************************
		 *	      GET-*		*
		 *******************************/

int
PL_get_atom(term_t t, atom_t *a)
{ word w = valHandle(t);

  if ( isAtom(w) )
  { *a = (atom_t) w;
    succeed;
  }
  fail;
}


int
PL_get_atom_chars(term_t t, char **s)
{ word w = valHandle(t);

  if ( isAtom(w) )
  { *s = stringAtom(w);
    succeed;
  }
  fail;
}

#ifdef O_STRING
int
PL_get_string(term_t t, char **s, int *len)
{ word w = valHandle(t);

  if ( isString(w) )
  { *s = valString(w);
    *len = sizeString(w);
    succeed;
  }
  fail;
}
#endif

#define discardable_buffer 	(LD->fli._discardable_buffer)
#define buffer_ring		(LD->fli._buffer_ring)
#define current_buffer_id	(LD->fli._current_buffer_id)

static Buffer
findBuffer(int flags)
{ Buffer b;

  if ( flags & BUF_RING )
  { if ( ++current_buffer_id == BUFFER_RING_SIZE )
      current_buffer_id = 0;
    b = &buffer_ring[current_buffer_id];
  } else
    b = &discardable_buffer;

  if ( !b->base )
    initBuffer(b);

  emptyBuffer(b);
  return b;
}


char *
buffer_string(const char *s, int flags)
{ Buffer b = findBuffer(flags);
  int l = strlen(s) + 1;

  addMultipleBuffer(b, s, l, char);

  return baseBuffer(b, char);
}


static int
unfindBuffer(int flags)
{ if ( flags & BUF_RING )
  { if ( --current_buffer_id <= 0 )
      current_buffer_id = BUFFER_RING_SIZE-1;
  }

  fail;
}


static char *
malloc_string(const char *s)
{ char *c;
  int len = strlen(s)+1;

  if ( (c = malloc(len)) )
  { memcpy(c, s, len);
    return c;
  }

  outOfCore();
  return NULL;
}


int
PL_get_list_chars(term_t l, char **s, unsigned flags)
{ Buffer b = findBuffer(flags);
  word list = valHandle(l);
  Word arg, tail;
  int c;
  char *r;

  while( isList(list) && !isNil(list) )
  { arg = argTermP(list, 0);
    deRef(arg);
    if ( isTaggedInt(*arg) && (c=(int)valInt(*arg)) > 0 && c < 256)
    { addBuffer(b, c, char);
      tail = argTermP(list, 1);
      deRef(tail);
      list = *tail;
      continue;
    }
    return unfindBuffer(flags);
  }
  if (!isNil(list))
    return unfindBuffer(flags);

  addBuffer(b, EOS, char);
  r = baseBuffer(b, char);

  if ( flags & BUF_MALLOC )
    *s = malloc_string(r);
  else
    *s = r;

  succeed;
}


int
PL_get_chars(term_t l, char **s, unsigned flags)
{ word w = valHandle(l);
  char tmp[24];
  char *r;
  int type;

  if ( (flags & CVT_ATOM) && isAtom(w) )
  { type = PL_ATOM;
    r = stringAtom(w);
  } else if ( (flags & CVT_INTEGER) && isInteger(w) )
  { type = PL_INTEGER;
    Ssprintf(tmp, "%ld", valInteger(w) );
    r = tmp;
  } else if ( (flags & CVT_FLOAT) && isReal(w) )
  { type = PL_FLOAT;
    Ssprintf(tmp, "%f", valReal(w) );
    r = tmp;
#ifdef O_STRING
  } else if ( (flags & CVT_STRING) && isString(w) )
  { type = PL_STRING;
    r = valString(w);
#endif
  } else if ( (flags & CVT_LIST) )
  { return PL_get_list_chars(l, s, flags);
  } else if ( (flags & CVT_VARIABLE) )
  { type = PL_VARIABLE;
    r = varName(l, tmp);
  } else
    fail;
    
  if ( flags & BUF_MALLOC )
  { *s = malloc_string(r);
  } else if ( ((flags & BUF_RING) && type != PL_ATOM) || /* never atoms */
	      (type == PL_STRING) ||	/* always buffer strings */
	      r == tmp )		/* always buffer tmp */
  { Buffer b = findBuffer(flags);
    int l = strlen(r) + 1;

    addMultipleBuffer(b, r, l, char);
    *s = baseBuffer(b, char);
  } else
    *s = r;

  succeed;
}


int
PL_get_integer(term_t t, int *i)
{ word w = valHandle(t);
  
  if ( isTaggedInt(w) )
  { *i = valInt(w);
    succeed;
  }
  if ( isBignum(w) )
  { *i = valBignum(w);
    succeed;
  }
  if ( isReal(w) )
  { real f = valReal(w);
    long l;

#ifdef DOUBLE_TO_LONG_CAST_RAISES_SIGFPE
    if ( !((f >= PLMININT) && (f <= PLMAXINT)) )
      fail;
#endif

    l = (long)f;
    if ( (real)l == f )
    { *i = l;
      succeed;
    }
  }
  fail;
} 


int
PL_get_long(term_t t, long *i)
{ word w = valHandle(t);
  
  if ( isTaggedInt(w) )
  { *i = valInt(w);
    succeed;
  }
  if ( isBignum(w) )
  { *i = valBignum(w);
    succeed;
  }
  if ( isReal(w) )
  { real f = valReal(w);
    long l;
    
#ifdef DOUBLE_TO_LONG_CAST_RAISES_SIGFPE
    if ( !((f >= PLMININT) && (f <= PLMAXINT)) )
      fail;
#endif

    l = (long) f;
    if ( (real)l == f )
    { *i = l;
      succeed;
    }
  }
  fail;
} 


int
PL_get_float(term_t t, double *f)
{ word w = valHandle(t);
  
  if ( isReal(w) )
  { *f = valReal(w);
    succeed;
  }
  if ( isTaggedInt(w) )
  { *f = (double) valInt(w);
    succeed;
  }
  if ( isBignum(w) )
  { *f = (double) valBignum(w);
    succeed;
  }
  fail;
}


int
PL_get_pointer(term_t t, void **ptr)
{ long p;

  if ( PL_get_long(t, &p) )
  { *ptr = longToPointer((ulong)p);

    succeed;
  }

  fail;
} 



int
PL_get_name_arity(term_t t, atom_t *name, int *arity)
{ word w = valHandle(t);

  if ( isTerm(w) )
  { FunctorDef fd = valueFunctor(functorTerm(w));

    *name =  fd->name;
    *arity = fd->arity;
    succeed;
  }
  if ( isAtom(w) )
  { *name = (atom_t)w;
    *arity = 0;
    succeed;
  }

  fail;
}


int
_PL_get_name_arity(term_t t, atom_t *name, int *arity)
{ word w = valHandle(t);

  if ( isTerm(w) )
  { FunctorDef fd = valueFunctor(functorTerm(w));

    *name =  fd->name;
    *arity = fd->arity;
    succeed;
  }

  fail;
}


int
PL_get_functor(term_t t, functor_t *f)
{ word w = valHandle(t);

  if ( isTerm(w) )
  { *f = functorTerm(w);
    succeed;
  }
  if ( isAtom(w) )
  { *f = lookupFunctorDef(w, 0);
    succeed;
  }

  fail;
}


int
PL_get_module(term_t t, module_t *m)
{ atom_t a;

  if ( PL_get_atom(t, &a) )
  { *m = lookupModule(a);
    succeed;
  }

  fail;
}


void
_PL_get_arg(int index, term_t t, term_t a)
{ word w = valHandle(t);
  Functor f = (Functor)valPtr(w);
  Word p = &f->arguments[index-1];

  deRef(p);

  if ( isVar(*p) )
    w = consPtr(p, TAG_REFERENCE|storage(w)); /* makeRef() */
  else
    w = *p;

  setHandle(a, w);
}


int
PL_get_arg(int index, term_t t, term_t a)
{ word w = valHandle(t);

  if ( isTerm(w) && index > 0 )
  { Functor f = (Functor)valPtr(w);
    int arity = arityFunctor(f->definition);

    if ( --index < arity )
    { Word p = &f->arguments[index];

      deRef(p);

      if ( isVar(*p) )
	w = makeRef(p);
      else
	w = *p;

      setHandle(a, w);
      succeed;
    }
  }

  fail;
}


int
PL_get_list(term_t l, term_t h, term_t t)
{ word w = valHandle(l);

  if ( isList(w) )
  { Word p1, p2;
    
    p1 = argTermP(w, 0);
    p2 = argTermP(w, 1);
    deRef(p1);
    deRef(p2);
    setHandle(h, isVar(*p1) ? makeRef(p1) : *p1);
    setHandle(t, isVar(*p2) ? makeRef(p2) : *p2);
    succeed;
  }
  fail;
}


int
PL_get_head(term_t l, term_t h)
{ word w = valHandle(l);

  if ( isList(w) )
  { Word p;
    
    p = argTermP(w, 0);
    deRef(p);
    setHandle(h, *p ? *p : makeRef(p));
    succeed;
  }
  fail;
}


int
PL_get_tail(term_t l, term_t t)
{ word w = valHandle(l);

  if ( isList(w) )
  { Word p;
    
    p = argTermP(w, 1);
    deRef(p);
    setHandle(t, *p ? *p : makeRef(p));
    succeed;
  }
  fail;
}


int
PL_get_nil(term_t l)
{ word w = valHandle(l);

  if ( isNil(w) )
    succeed;

  fail;
}


int
_PL_get_xpce_reference(term_t t, xpceref_t *ref)
{ word w = valHandle(t);

  if ( hasFunctor(w, FUNCTOR_xpceref1) )
  { Word p = argTermP(w, 0);

    do
    { if ( isTaggedInt(*p) )
      { ref->type    = PL_INTEGER;
	ref->value.i = valInt(*p);

	succeed;
      } 
      if ( isAtom(*p) )
      { ref->type    = PL_ATOM;
	ref->value.a = (atom_t) *p;

	succeed;
      }
      if ( isBignum(*p) )
      { ref->type    = PL_INTEGER;
	ref->value.i = valBignum(*p);

	succeed;
      }
    } while(isRef(*p) && (p = unRef(*p)));

    return -1;				/* error! */
  }

  fail;
}


		 /*******************************
		 *		IS-*		*
		 *******************************/

int
PL_is_variable(term_t t)
{ word w = valHandle(t);

  return isVar(w) ? TRUE : FALSE;
}


int
PL_is_atom(term_t t)
{ word w = valHandle(t);

  return isAtom(w) ? TRUE : FALSE;
}


int
PL_is_integer(term_t t)
{ word w = valHandle(t);

  return isInteger(w) ? TRUE : FALSE;
}


int
PL_is_float(term_t t)
{ word w = valHandle(t);

  return isReal(w) ? TRUE : FALSE;
}


int
PL_is_compound(term_t t)
{ word w = valHandle(t);

  return isTerm(w) ? TRUE : FALSE;
}


int
PL_is_functor(term_t t, functor_t f)
{ word w = valHandle(t);

  if ( hasFunctor(w, f) )
    succeed;

  fail;
}


int
PL_is_list(term_t t)
{ word w = valHandle(t);

  return (isList(w) || isNil(w)) ? TRUE : FALSE;
}


int
PL_is_atomic(term_t t)
{ word w = valHandle(t);

  return isAtomic(w) ? TRUE : FALSE;
}


int
PL_is_number(term_t t)
{ word w = valHandle(t);

  return isNumber(w) ? TRUE : FALSE;
}


#ifdef O_STRING
int
PL_is_string(term_t t)
{ word w = valHandle(t);

  return isString(w) ? TRUE : FALSE;
}

int
PL_unify_string_chars(term_t t, const char *s)
{ word str = globalString((char *)s);
  Word p = valHandleP(t);

  return unifyAtomic(p, str);
}

int
PL_unify_string_nchars(term_t t, int len, const char *s)
{ word str = globalNString(len, (char *)s);
  Word p = valHandleP(t);

  return unifyAtomic(p, str);
}

#endif

		 /*******************************
		 *             PUT-*  		*
		 *******************************/

void
PL_put_variable(term_t t)
{ Word p = allocGlobal(1);

  setVar(*p);
  setHandle(t, makeRef(p));
}


void
PL_put_atom(term_t t, atom_t a)
{ setHandle(t, a);
}


void
PL_put_atom_chars(term_t t, const char *s)
{ setHandle(t, lookupAtom(s));
}


void
PL_put_string_chars(term_t t, const char *s)
{ word w = globalString(s);

  setHandle(t, w);
}

void
PL_put_list_chars(term_t t, const char *chars)
{ int len = strlen(chars);
  
  if ( len == 0 )
  { setHandle(t, ATOM_nil);
  } else
  { Word p = allocGlobal(len*3);
    setHandle(t, consPtr(p, TAG_COMPOUND|STG_GLOBAL));

    for( ; *chars ; chars++)
    { *p++ = FUNCTOR_dot2;
      *p++ = consInt((long)*chars & 0xff);
      *p = consPtr(p+1, TAG_COMPOUND|STG_GLOBAL);
      p++;
    }
    p[-1] = ATOM_nil;
  }
}

void
PL_put_integer(term_t t, long i)
{ setHandle(t, makeNum(i));
}


void
_PL_put_number(term_t t, Number n)
{ if ( intNumber(n) )
    PL_put_integer(t, n->value.i);
  else
    PL_put_float(t, n->value.f);
}


void
PL_put_pointer(term_t t, void *ptr)
{ PL_put_integer(t, pointerToLong(ptr));
}


void
PL_put_float(term_t t, double f)
{ setHandle(t, globalReal(f));
}


void
PL_put_functor(term_t t, functor_t f)
{ int arity = arityFunctor(f);

  if ( arity == 0 )
  { setHandle(t, nameFunctor(f));
  } else
  { Word a = allocGlobal(1 + arity);

    setHandle(t, consPtr(a, TAG_COMPOUND|STG_GLOBAL));
    *a++ = f;
    while(arity-- > 0)
      setVar(*a++);
  }
}


void
PL_put_list(term_t l)
{ Word a = allocGlobal(3);

  setHandle(l, consPtr(a, TAG_COMPOUND|STG_GLOBAL));
  *a++ = FUNCTOR_dot2;
  setVar(*a++);
  setVar(*a);
}


void
PL_put_nil(term_t l)
{ setHandle(l, ATOM_nil);
}


void
PL_put_term(term_t t1, term_t t2)
{ Word p2 = valHandleP(t2);

  deRef(p2);
  setHandle(t1, isVar(*p2) ? makeRef(p2) : *p2);
}


void
_PL_put_xpce_reference_i(term_t t, unsigned long r)
{ Word a = allocGlobal(2);

  setHandle(t, consPtr(a, TAG_COMPOUND|STG_GLOBAL));
  *a++ = FUNCTOR_xpceref1;
  *a++ = makeNum(r);
}


void
_PL_put_xpce_reference_a(term_t t, atom_t name)
{ Word a = allocGlobal(2);

  setHandle(t, consPtr(a, TAG_COMPOUND|STG_GLOBAL));
  *a++ = FUNCTOR_xpceref1;
  *a++ = name;
}


		 /*******************************
		 *	       UNIFY		*
		 *******************************/

int
PL_unify_atom(term_t t, atom_t a)
{ Word p = valHandleP(t);

  return unifyAtomic(p, a);
}


int
PL_unify_functor(term_t t, functor_t f)
{ Word p = valHandleP(t);
  int arity = arityFunctor(f);

  deRef(p);
  if ( isVar(*p) )
  { if ( arity == 0 )
    { *p = nameFunctor(f);
    } else
    { 
#ifdef O_SHIFT_STACKS
      if ( !roomStack(global) > (1+arity) * sizeof(word) )
      { growStacks(environment_frame, NULL, FALSE, TRUE, FALSE);
	p = valHandleP(t);
	deRef(p);
      }
#else 
      requireStack(global, sizeof(word)*(1+arity));
#endif

      { Word a = gTop;
	gTop += 1+arity;

	*p = consPtr(a, TAG_COMPOUND|STG_GLOBAL);
	*a++ = f;
	for( ; arity > 0; a++, arity-- )
	  setVar(*a);
      }
    }

    DoTrail(p);
    succeed;
  } else
  { if ( arity == 0  )
    { if ( *p == nameFunctor(f) )
	succeed;
    } else
    { if ( hasFunctor(*p, f) )
	succeed;
    }

    fail;
  }
}


int
PL_unify_atom_chars(term_t t, const char *chars)
{ Word p = valHandleP(t);

  return unifyAtomic(p, lookupAtom((char *)chars));
}


int
PL_unify_list_chars(term_t l, const char *chars)
{ term_t head = PL_new_term_ref();
  term_t t    = PL_copy_term_ref(l);
  int rval;

  for( ; *chars; chars++ )
  { if ( !PL_unify_list(t, head, t) ||
	 !PL_unify_integer(head, (int)*chars & 0xff) )
      fail;
  }

  rval = PL_unify_nil(t);
  PL_reset_term_refs(head);

  return rval;
}


int
PL_unify_integer(term_t t, long i)
{ Word p = valHandleP(t);

  return unifyAtomic(p, makeNum(i));
}


int
_PL_unify_number(term_t t, Number n)
{ if ( intNumber(n) )
    return PL_unify_integer(t, n->value.i);
  else
    return PL_unify_float(t, n->value.f);
}


int
PL_unify_pointer(term_t t, void *ptr)
{ return PL_unify_integer(t, pointerToLong(ptr));
}


int
PL_unify_float(term_t t, double f)
{ word w = globalReal(f);
  Word p = valHandleP(t);

  return unifyAtomic(p, w);
}


int
PL_unify_arg(int index, term_t t, term_t a)
{ word w = valHandle(t);

  if ( isTerm(w) &&
       index > 0 &&
       index <= (int)arityFunctor(functorTerm(w)) )
  { Word p = argTermP(w, index-1);
    Word p2 = valHandleP(a);

    return unify_ptrs(p, p2);
  }

  fail;
}


int					/* can be faster! */
PL_unify_list(term_t l, term_t h, term_t t)
{ if ( PL_unify_functor(l, FUNCTOR_dot2) )
  { PL_get_list(l, h, t);

    succeed;
  }

  fail;
}


int
PL_unify_nil(term_t l)
{ Word p = valHandleP(l);

  return unifyAtomic(p, ATOM_nil);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Fixed by Franklin Chen <chen@adi.com> to   compile on MkLinux, where you
cannot assign to va_list as it is an array. Thanks!
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef struct va_list_rec {
  va_list v;
} va_list_rec;

#define args argsRec.v

static int
unify_termVP(term_t t, va_list_rec *argsRecP)
{ va_list_rec argsRec = *argsRecP;
  int rval;

  switch(va_arg(args, int))
  { case PL_VARIABLE:
      rval = TRUE;
      break;
    case PL_ATOM:
      rval = PL_unify_atom(t, va_arg(args, atom_t));
      break;
    case PL_INTEGER:
      rval = PL_unify_integer(t, va_arg(args, long));
      break;
    case PL_POINTER:
      rval = PL_unify_pointer(t, va_arg(args, void *));
      break;
    case PL_FLOAT:
      rval = PL_unify_float(t, va_arg(args, double));
      break;
    case PL_STRING:
      rval = PL_unify_string_chars(t, va_arg(args, const char *));
      break;
    case PL_TERM:
      rval = PL_unify(t, va_arg(args, term_t));
      break;
    case PL_CHARS:
      rval = PL_unify_atom_chars(t, va_arg(args, const char *));
      break;
    case PL_FUNCTOR:
    { functor_t ft = va_arg(args, functor_t);
      int arity = arityFunctor(ft);
      term_t tmp = PL_new_term_ref();
      int n;

      if ( !PL_unify_functor(t, ft) )
	goto failout;

      for(n=1; n<=arity; n++)
      {	_PL_get_arg(n, t, tmp);
	
	rval = unify_termVP(tmp, &argsRec);
	if ( !rval )
	  goto failout;
      }

      rval = TRUE;
      PL_reset_term_refs(tmp);
      break;
    failout:
      rval = FALSE;
      PL_reset_term_refs(tmp);
      break;
    }
    case PL_LIST:
    { int length = va_arg(args, int);
      term_t tmp = PL_copy_term_ref(t);
      term_t h   = PL_new_term_ref();

      for( ; length-- > 0; )
      { PL_unify_list(tmp, h, tmp);
	rval = unify_termVP(h, &argsRec);
	if ( !rval )
	  goto listfailout;
      }

      rval = PL_unify_nil(tmp);
      PL_reset_term_refs(tmp);
      break;
    listfailout:
      PL_reset_term_refs(tmp);
      break;
    }
    default:
      PL_warning("Format error in PL_unify_term()");
      rval = FALSE;
  }

  *argsRecP = argsRec;
  return rval;
}

int
PL_unify_term(term_t t, ...)
{
  va_list_rec argsRec;
  int rval;

  va_start(args, t);
  rval = unify_termVP(t, &argsRec);
  va_end(args);

  return rval;
}

#undef args

int
_PL_unify_xpce_reference(term_t t, xpceref_t *ref)
{ Word p = valHandleP(t);

  do
  { if ( isVar(*p) )
    { Word a = allocGlobal(2);
  
      *p = consPtr(a, TAG_COMPOUND|STG_GLOBAL);
      DoTrail(p);
      *a++ = FUNCTOR_xpceref1;
      if ( ref->type == PL_INTEGER )
	*a++ = makeNum(ref->value.i);
      else
	*a++ = ref->value.a;
  
      succeed;
    } 
    if ( hasFunctor(*p, FUNCTOR_xpceref1) )
    { Word a = argTermP(*p, 0);
      word v = (ref->type == PL_INTEGER ? makeNum(ref->value.i)
					: ref->value.a);
  
      deRef(a);
      return unifyAtomic(a, v);
    }
  } while ( isRef(*p) && (p = unRef(*p)));

  fail;
}


		 /*******************************
		 *       ATOMIC (INTERNAL)	*
		 *******************************/

atomic_t
_PL_get_atomic(term_t t)
{ return valHandle(t);
}


int
_PL_unify_atomic(term_t t, atomic_t a)
{ Word p = valHandleP(t);

  return unifyAtomic(p, a);
}


void
_PL_put_atomic(term_t t, atomic_t a)
{ setHandle(t, a);
}


void
_PL_copy_atomic(term_t t, atomic_t arg) /* internal one */
{ word a;

  if ( isIndirect(arg) )
    a = globalIndirect(arg);
  else
    a = arg;
  
  setHandle(t, a);
}


		 /*******************************
		 *	       TYPE		*
		 *******************************/


int
PL_term_type(term_t t)
{ word w = valHandle(t);

  if ( isVar(w) )		return PL_VARIABLE;
  if ( isInteger(w) )		return PL_INTEGER;
  if ( isReal(w) )		return PL_FLOAT;
#if O_STRING
  if ( isString(w) )		return PL_STRING;
#endif /* O_STRING */
  if ( isAtom(w) )		return PL_ATOM;

  assert(isTerm(w));
  				return PL_TERM;
}

		 /*******************************
		 *	      UNIFY		*
		 *******************************/

int
PL_unify(term_t t1, term_t t2)
{ Word p1 = valHandleP(t1);
  Word p2 = valHandleP(t2);
  mark m;
  int rval;

  Mark(m);
  if ( !(rval = unify(p1, p2, environment_frame)) )
    Undo(m);

  return rval;  
}


		 /*******************************
		 *	       MODULES		*
		 *******************************/

int
PL_strip_module(term_t raw, module_t *m, term_t plain)
{ Word r = valHandleP(raw);
  Word p;

  if ( (p = stripModule(r, m)) )
  { setHandle(plain, isVar(*p) ? makeRef(p) : *p);
    succeed;
  }

  fail;
}

		/********************************
		*            MODULES            *
		*********************************/

module_t
PL_context()
{ return environment_frame ? contextModule(environment_frame)
			   : MODULE_user;
}

atom_t
PL_module_name(Module m)
{ return (atom_t) m->name;
}

module_t
PL_new_module(atom_t name)
{ return lookupModule(name);
}


		 /*******************************
		 *	    PREDICATES		*
		 *******************************/

predicate_t
PL_pred(functor_t functor, module_t module)
{ if ( module == NULL )
    module = PL_context();

  return lookupProcedure(functor, module);
}


predicate_t
PL_predicate(const char *name, int arity, const char *module)
{ Module m = module ? lookupModule(lookupAtom(module)) : PL_context();
  functor_t f = lookupFunctorDef(lookupAtom(name), arity);

  return PL_pred(f, m);
}


predicate_t
_PL_predicate(const char *name, int arity, const char *module,
	      predicate_t *bin)
{ if ( !*bin )
    *bin = PL_predicate(name, arity, module);

  return *bin;
}


int
PL_predicate_info(predicate_t pred, atom_t *name, int *arity, module_t *m)
{ if ( pred->type == PROCEDURE_TYPE )
  { *name  = pred->definition->functor->name;
    *arity = pred->definition->functor->arity;
    *m     = pred->definition->module;

    succeed;
  }

  fail;
}

		 /*******************************
		 *	       CALLING		*
		 *******************************/

int
PL_call_predicate(Module ctx, int debug, predicate_t pred, term_t h0)
{ int rval;
  int flags = (debug ? PL_Q_NORMAL : PL_Q_NODEBUG);

  qid_t qid = PL_open_query(ctx, flags, pred, h0);
  rval = PL_next_solution(qid);
  PL_cut_query(qid);

  return rval;
}


bool
PL_call(term_t t, Module m)
{ return callProlog(m, t, TRUE);
}  


		/********************************
		*	 FOREIGNS RETURN        *
		********************************/

foreign_t
_PL_retry(long v)
{ ForeignRedoInt(v);
}


foreign_t
_PL_retry_address(void *v)
{ if ( (ulong)v & FRG_CONTROL_MASK )
    PL_fatal_error("PL_retry_address(0x%lx): bad alignment", (ulong)v);

  ForeignRedoPtr(v);
}


long
PL_foreign_context(control_t h)
{ return ForeignContextInt(h);
}

void *
PL_foreign_context_address(control_t h)
{ return ForeignContextPtr(h);
}


int
PL_foreign_control(control_t h)
{ return ForeignControl(h);
}


int
PL_throw(term_t exception)
{ PL_put_term(exception_bin, exception);

  exception_term = exception_bin;

  fail;
}

		/********************************
		*      REGISTERING FOREIGNS     *
		*********************************/

static void
notify_registered_foreign(functor_t fd, Module m)
{ if ( GD->initialised )
  { fid_t cid = PL_open_foreign_frame();
    term_t argv = PL_new_term_refs(2);
    predicate_t pred = _PL_predicate("$foreign_registered", 2, "system",
				     &GD->procedures.foreign_registered2);

    PL_put_atom(argv+0, m->name);
    PL_put_functor(argv+1, fd);
    PL_call_predicate(MODULE_system, FALSE, pred, argv);
    PL_discard_foreign_frame(cid);
  }
}


bool
PL_register_foreign(const char *name, int arity, Func f, int flags)
{ Procedure proc;
  Definition def;
  Module m;
  functor_t fdef = lookupFunctorDef(lookupAtom(name), arity);

  m = (environment_frame ? contextModule(environment_frame)
			 : MODULE_system);

  proc = lookupProcedure(lookupFunctorDef(lookupAtom(name), arity), m);
  def = proc->definition;

  if ( true(def, LOCKED) )
  { warning("PL_register_foreign(): Attempt to redefine a system predicate: %s",
	    procedureName(proc));
    fail;
  }

  if ( def->definition.function )
    warning("PL_register_foreign(): redefined %s", procedureName(proc));
  if ( false(def, FOREIGN) && def->definition.clauses != NULL )
    abolishProcedure(proc, m);

  def->definition.function = f;
  def->indexPattern = 0;
  def->indexCardinality = 0;
  def->flags = 0;
  set(def, FOREIGN|TRACE_ME);
  clear(def, NONDETERMINISTIC);

  if ( (flags & PL_FA_NOTRACE) )	  clear(def, TRACE_ME);
  if ( (flags & PL_FA_TRANSPARENT) )	  set(def, TRANSPARENT);
  if ( (flags & PL_FA_NONDETERMINISTIC) ) set(def, NONDETERMINISTIC);

  notify_registered_foreign(fdef, m);

  succeed;
}  


bool
PL_load_extensions(PL_extension *ext)
{ PL_extension *e;
  Module m;

  m = (environment_frame ? contextModule(environment_frame)
			 : MODULE_system);

  for(e = ext; e->predicate_name; e++)
  { short flags = TRACE_ME;
    register Definition def;
    register Procedure proc;

    if ( e->flags & PL_FA_NOTRACE )	     flags &= ~TRACE_ME;
    if ( e->flags & PL_FA_TRANSPARENT )	     flags |= TRANSPARENT;
    if ( e->flags & PL_FA_NONDETERMINISTIC ) flags |= NONDETERMINISTIC;

    proc = lookupProcedure(lookupFunctorDef(lookupAtom(e->predicate_name),
					    e->arity), 
			   m);
    def = proc->definition;
    if ( true(def, LOCKED) )
    { warning("PL_load_extensions(): Attempt to redefine system predicate: %s",
	      procedureName(proc));
      continue;
    }
    if ( def->definition.function )
      warning("PL_load_extensions(): redefined %s", procedureName(proc));
    if ( false(def, FOREIGN) && def->definition.clauses != NULL )
      abolishProcedure(proc, m);
    set(def, FOREIGN);
    set(def, flags);
    def->definition.function = e->function;
    def->indexPattern = 0;
    def->indexCardinality = 0;

    notify_registered_foreign(def->functor->functor, m);
  }    

  succeed;
}

		 /*******************************
		 *	 EMBEDDING PROLOG	*
		 *******************************/

int
PL_toplevel(void)
{ return prolog(lookupAtom("$toplevel"));
}


void
PL_halt(int status)
{ Halt(status);
}


		/********************************
		*            SIGNALS            *
		*********************************/

#if HAVE_SIGNAL
void
(*PL_signal(int sig, void (*func) (int)))(int)
{ void (*old)(int);

  if ( sig < 1 || sig > MAXSIGNAL )
  { fatalError("PL_signal(): illegal signal number: %d", sig);
    return NULL;
  }

  if ( LD_sig_handler(sig).catched == FALSE )
  { old = signal(sig, func);
    LD_sig_handler(sig).os = func;
    
    return old;
  }

  old = LD_sig_handler(sig).user;
  LD_sig_handler(sig).user = func;

  return old;
}
#endif

void
PL_raise(int sig)
{ if ( sig > 0 && sig <= MAXSIGNAL )
    signalled |= (1L << (sig-1));
}


		/********************************
		*         RESET (ABORTS)	*
		********************************/

struct abort_handle
{ AbortHandle	  next;			/* Next handle */
  PL_abort_hook_t function;		/* The handle itself */
};

#define abort_head (LD->fli._abort_head)
#define abort_tail (LD->fli._abort_tail)

void
PL_abort_hook(PL_abort_hook_t func)
{ AbortHandle h = (AbortHandle) allocHeap(sizeof(struct abort_handle));
  h->next = NULL;
  h->function = func;

  if ( abort_head == NULL )
  { abort_head = abort_tail = h;
  } else
  { abort_tail->next = h;
    abort_tail = h;
  }
}


int
PL_abort_unhook(PL_abort_hook_t func)
{ AbortHandle h = abort_head;

  for(; h; h = h->next)
  { if ( h->function == func )
    { h->function = NULL;
      return TRUE;
    }
  }

  return FALSE;
}


void
resetForeign(void)
{ AbortHandle h = abort_head;

  for(; h; h = h->next)
    if ( h->function )
      (*h->function)();
}


		/********************************
		*        FOREIGN INITIALISE	*
		********************************/

struct initialise_handle
{ InitialiseHandle	  next;			/* Next handle */
  PL_initialise_hook_t function;		/* The handle itself */
};

#define initialise_head (LD->fli._initialise_head)
#define initialise_tail (LD->fli._initialise_tail)

void
PL_initialise_hook(PL_initialise_hook_t func)
{ InitialiseHandle h = initialise_head;

  for(; h; h = h->next)
  { if ( h->function == func )
      return;				/* already there */
  }

  h = (InitialiseHandle) malloc(sizeof(struct initialise_handle));

  h->next = NULL;
  h->function = func;

  if ( initialise_head == NULL )
  { initialise_head = initialise_tail = h;
  } else
  { initialise_tail->next = h;
    initialise_tail = h;
  }
}


void
initialiseForeign(int argc, char **argv)
{ InitialiseHandle h = initialise_head;

  for(; h; h = h->next)
    (*h->function)(argc, argv);
}


		 /*******************************
		 *	      PROMPT		*
		 *******************************/

void
PL_prompt1(const char *s)
{ prompt1((char *) s);
}


int
PL_ttymode(int fd)
{ if ( fd == 0 )
  { if ( GD->cmdline.notty )		/* -tty in effect */
      return PL_NOTTY;
    if ( ttymode == TTY_RAW )		/* get_single_char/1 and friends */
      return PL_RAWTTY;
    return PL_COOKEDTTY;		/* cooked (readline) input */
  } else
    return PL_NOTTY;
}


void
PL_write_prompt(int fd, int dowrite)
{ if ( fd == 0 )
  { if ( dowrite )
    { extern int Output;
      int old = Output;
      Output = 1;
      Putf("%s", PrologPrompt());
      pl_flush();
      Output = old;
    }

    pl_ttyflush();
    GD->os.prompt_next = FALSE;
  }
}


void
PL_prompt_next(int fd)
{ if ( fd == 0 )
    GD->os.prompt_next = TRUE;
}


char *
PL_prompt_string(int fd)
{ if ( fd == 0 )
    return PrologPrompt();

  return "";
}


void
PL_add_to_protocol(const char *buf, int n)
{ protocol((char *)buf, n);
}


		 /*******************************
		 *	   DISPATCHING		*
		 *******************************/

#define dispatch_events (LD->fli._dispatch_events)

PL_dispatch_hook_t
PL_dispatch_hook(PL_dispatch_hook_t hook)
{ PL_dispatch_hook_t old = dispatch_events;

  dispatch_events = hook;
  return old;
}

int
PL_dispatch(int fd, int wait)
{ int rval;

  if ( wait == PL_DISPATCH_INSTALLED )
    return dispatch_events ? TRUE : FALSE;

  if ( dispatch_events )
  { do
    { rval = (*dispatch_events)(fd);
    } while( wait == PL_DISPATCH_WAIT && rval == PL_DISPATCH_TIMEOUT );
  } else
    rval = PL_DISPATCH_INPUT;

  return rval;
}


		 /*******************************
		 *	    FEATURES		*
		 *******************************/

int
PL_set_feature(const char *name, int type, ...)
{ va_list args;
  int rval = TRUE;

  va_start(args, type);
  switch(type)
  { case PL_ATOM:
    { char *v = va_arg(args, char *);
      setFeature(lookupAtom(name), FT_ATOM, lookupAtom(v));
      break;
    }
    case PL_INTEGER:
    { long v = va_arg(args, long);
      setFeature(lookupAtom(name), FT_INTEGER, v);
      break;
    }
    default:
      rval = FALSE;
  }

  va_end(args);
  return rval;
}


		/********************************
		*           WARNINGS            *
		*********************************/

bool
PL_warning(const char *fm, ...)
{ va_list args;

  va_start(args, fm);
  vwarning(fm, args);
  va_end(args);

  fail;
}

void
PL_fatal_error(const char *fm, ...)
{ va_list args;

  va_start(args, fm);
  vfatalError(fm, args);
  va_end(args);
}


		/********************************
		*            ACTIONS            *
		*********************************/

int
PL_action(int action, ...)
{ int rval;
  va_list args;

  va_start(args, action);

  switch(action)
  { case PL_ACTION_TRACE:
      rval = pl_trace();
      break;
    case PL_ACTION_DEBUG:
      rval = pl_debug();
      break;
    case PL_ACTION_BACKTRACE:
#ifdef O_DEBUGGER
    { int a = va_arg(args, int);

      if ( gc_status.active )
      { Sfprintf(Serror,
		 "\n[Cannot print stack while in %ld-th garbage collection]\n",
		 gc_status.collections);
	fail;
      }
      if ( GD->bootsession || !GD->initialised )
      { Sfprintf(Serror,
		 "\n[Cannot print stack while initialising]\n");
	fail;
      }
      backTrace(environment_frame, a);
      rval = TRUE;
    }
#else
      warning("No Prolog backtrace in runtime version");
      rval = FALSE;
#endif
      break;
    case PL_ACTION_BREAK:
      rval = pl_break();
      break;
    case PL_ACTION_HALT:
    { int a = va_arg(args, int);

      Halt(a);
      rval = FALSE;
      break;
    }
    case PL_ACTION_ABORT:
      rval = pl_abort();
      break;
    case PL_ACTION_SYMBOLFILE:
    { char *name = va_arg(args, char *);
      loaderstatus.symbolfile = lookupAtom(name);
      rval = TRUE;
      break;
    }
    case PL_ACTION_WRITE:
    { char *s = va_arg(args, char *);
      Putf("%s", (char *)s);
      rval = TRUE;
      break;
    }
    case PL_ACTION_FLUSH:
      rval = pl_flush();
      break;
    default:
      sysError("PL_action(): Illegal action: %d", action);
      /*NOTREACHED*/
      rval = FALSE;
  }

  va_end(args);

  return rval;
}

		/********************************
		*         QUERY PROLOG          *
		*********************************/

#define c_argc (GD->cmdline._c_argc)
#define c_argv (GD->cmdline._c_argv)

static void
init_c_args()
{ if ( c_argc == -1 )
  { int i;
    int argc    = GD->cmdline.argc;
    char **argv = GD->cmdline.argv;

    c_argv = allocHeap(argc * sizeof(char *));
    c_argv[0] = argv[0];
    c_argc = 1;

    for(i=1; i<argc; i++)
    { if ( argv[i][0] == '-' )
      { switch(argv[i][1])
	{ case 'x':
	  case 'g':
	  case 'd':
	  case 'f':
	  case 't':
	    i++;
	    continue;
	  case 'B':
	  case 'L':
	  case 'G':
	  case 'O':
	  case 'T':
	  case 'A':
	    continue;
	}
      }
      c_argv[c_argc++] = argv[i];
    }
  }
}


long
PL_query(int query)
{ switch(query)
  { case PL_QUERY_ARGC:
      init_c_args();
      return (long) c_argc;
    case PL_QUERY_ARGV:
      init_c_args();
      return (long) c_argv;
    case PL_QUERY_SYMBOLFILE:
      if ( !getSymbols() )
	return (long) NULL;
      return (long) stringAtom(loaderstatus.symbolfile);
    case PL_QUERY_ORGSYMBOLFILE:
      if ( getSymbols() == FALSE )
	return (long) NULL;
      return (long) stringAtom(loaderstatus.orgsymbolfile);
    case PL_QUERY_MAX_INTEGER:
      return PLMAXINT;
    case PL_QUERY_MIN_INTEGER:
      return PLMININT;
    case PL_QUERY_MAX_TAGGED_INT:
      return PLMAXTAGGEDINT;
    case PL_QUERY_MIN_TAGGED_INT:
      return PLMINTAGGEDINT;
    case PL_QUERY_GETC:
      PopTty(&ttytab);			/* restore terminal mode */
      return (long) Sgetchar();		/* normal reading */
    case PL_QUERY_VERSION:
      return PLVERSION;
    default:
      sysError("PL_query: Illegal query: %d", query);
      /*NOTREACHED*/
      fail;
  }
}

