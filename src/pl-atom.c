/*  $Id$

    Copyright (c) 1990 Jan Wielemaker. All rights reserved.
    See ../LICENCE to find out about your rights.
    jan@swi.psy.uva.nl

    Purpose: atom management
*/

/*#define O_DEBUG 1*/
#include "pl-incl.h"
#include "pl-ctype.h"

static void	rehashAtoms();

static int  atom_buckets = ATOMHASHSIZE;
static int  atom_locked;
static Atom *atomTable;

#define lockAtoms() { atom_locked++; }
#define unlockAtoms() if ( --atom_locked == 0 && \
			   atom_buckets * 2 < statistics.atoms ) \
			rehashAtoms()

#if O_DEBUG
static int	lookups;
static int	cmps;
static void	exitAtoms(int status, void *arg);
#endif

Atom
lookupAtom(const char *s)
{ int v0 = unboundStringHashValue(s);
  int v = v0 & (atom_buckets-1);
  register Atom a;

  DEBUG(0, lookups++);

  for(a = atomTable[v]; a && !isRef((word)a); a = a->next)
  { DEBUG(0, cmps++);
    if (streq(s, a->name) )
      return a;
  }
  a = (Atom)allocHeap(sizeof(struct atom));
  a->next       = atomTable[v];
  a->type       = ATOM_TYPE;
#ifdef O_HASHTERM
  a->hash_value = v0;
#endif
  a->name       = store_string(s);
  atomTable[v]  = a;
  statistics.atoms++;

  if ( atom_buckets * 2 < statistics.atoms && !atom_locked )
    rehashAtoms();

  DEBUG(0, assert(isAtom((word)a) && !isInteger((word)a)));

  return a;
}


static void
makeAtomRefPointers()
{ Atom *a;
  int n;

  for(n=0, a=atomTable; n < (atom_buckets-1); n++, a++)
    *a = (Atom) makeRef(a+1);
  *a = NULL;
}


static void
rehashAtoms()
{ Atom *oldtab   = atomTable;
  int   oldbucks = atom_buckets;
  Atom a, n;

  startCritical;
  atom_buckets *= 2;
  atomTable = allocHeap(atom_buckets * sizeof(Atom));
  makeAtomRefPointers();
  
  DEBUG(0, Sdprintf("rehashing atoms (%d --> %d)\n", oldbucks, atom_buckets));

  for(a=oldtab[0]; a; a = n)
  { int v;

    while(isRef((word)a) )
    { a = *((Atom *)unRef(a));
      if ( a == NULL )
	goto out;
    }
    n = a->next;
    v = a->hash_value & (atom_buckets-1);
    a->next = atomTable[v];
    atomTable[v] = a;
  }

out:
  freeHeap(oldtab, oldbucks * sizeof(Atom));
  endCritical;
}


word
pl_atom_hashstat(term_t idx, term_t n)
{ int i, m;
  Atom a;
  
  if ( !PL_get_integer(idx, &i) || i < 0 || i >= atom_buckets )
    fail;
  for(m = 0, a = atomTable[i]; a && !isRef((word)a); a = a->next)
    m++;

  return PL_unify_integer(n, m);
}

#ifdef O_HASHTERM
#define ATOM(s) { (Atom)NULL, ATOM_TYPE, 0L, s }
#else
#define ATOM(s) { (Atom)NULL, ATOM_TYPE, s }
#endif

#ifdef COPY_ATOMS_TO_HEAP
Atom atoms;
#else
#define static_atoms atoms
#endif

struct atom static_atoms[] = {
#include "pl-atom.ic"
  ATOM((char *)NULL)
};
#undef ATOM

/* Note that the char * of the atoms is copied to the data segment.  This
   is done because some functions temporary change the char string associated
   with an atom (pl_concat_atom()) and GCC Sputs char constants in the text
   segment.
*/

#ifdef COPY_ATOMS_TO_HEAP
static void
copyAtomsToHeap()
{ atoms = allocHeap(sizeof(static_atoms));
  memcpy(atoms, static_atoms, sizeof(static_atoms));
}
#else
#define copyAtomsToHeap()
#endif

void
initAtoms(void)
{ atomTable = allocHeap(atom_buckets * sizeof(Atom));
  makeAtomRefPointers();
  copyAtomsToHeap();

  { Atom a;

    for( a = &atoms[0]; a->name; a++)
    { int v0 = unboundStringHashValue(a->name);
      int v = v0 & (atom_buckets-1);

      a->name = store_string(a->name);
      a->next       = atomTable[v];
#ifdef O_HASHTERM
      a->hash_value = v0;
#endif
      atomTable[v]  = a;
      DEBUG(0, assert(isAtom((word)a) && !isInteger((word)a)));
      statistics.atoms++;
    }
  }    

  DEBUG(0, PL_on_halt(exitAtoms, NULL));
}


#if O_DEBUG
static void
exitAtoms(int status, void *arg)
{ Sdprintf("hashstat: %d lookupAtom() calls used %d strcmp() calls\n",
	   lookups, cmps);
}
#endif


word
pl_current_atom(term_t a, word h)
{ Atom atom;

  switch( ForeignControl(h) )
  { case FRG_FIRST_CALL:
      if ( PL_is_atom(a) )      succeed;
      if ( !PL_is_variable(a) ) fail;

      atom = atomTable[0];
      lockAtoms();
      break;
    case FRG_REDO:
      atom = (Atom) ForeignContextAddress(h);
      break;
    case FRG_CUTTED:
    default:
      unlockAtoms();
      succeed;
  }

  for(; atom; atom = atom->next)
  { while(isRef((word)atom) )
    { atom = *((Atom *)unRef(atom));
      if (atom == (Atom) NULL)
	goto out;
    }
    PL_unify_atom(a, atom);

    return_next_table(Atom, atom, unlockAtoms());
  }

out:
  unlockAtoms();
  fail;
}

		 /*******************************
		 *	 ATOM COMPLETION	*
		 *******************************/

#define ALT_SIZ 80		/* maximum length of one alternative */
#define ALT_MAX 256		/* maximum number of alternatives */
#define stringMatch(m)	(stringAtom((m)->name))

forwards bool 	allAlpha(char *);

typedef struct match
{ Atom	name;
  int	length;
} *Match;


static bool
allAlpha(register char *s)
{ for( ; *s; s++)
   if ( !isAlpha(*s) )
     fail;

  succeed;
}


static char *
extendAtom(char *prefix, bool *unique)
{ Atom a = atomTable[0];
  bool first = TRUE;
  static char common[LINESIZ];
  int lp = (int) strlen(prefix);

  *unique = TRUE;

  for(; a; a = a->next)
  { while( isRef((word)a) )
    { a = *((Atom *)unRef(a));
      if ( a == (Atom)NULL)
	goto out;
    }
    if ( strprefix(stringAtom(a), prefix) )
    { if ( strlen(stringAtom(a)) >= LINESIZ )
	continue;
      if ( first == TRUE )
      { strcpy(common, stringAtom(a)+lp);
	first = FALSE;
      } else
      { char *s = common;
	char *q = stringAtom(a)+lp;
	while( *s && *s == *q )
	  s++, q++;
	*s = EOS;
	*unique = FALSE;
      }
    }
  }

out:
  return first == TRUE ? (char *)NULL : common;
}


word
pl_complete_atom(term_t prefix, term_t common, term_t unique)
{ char *p, *s;
  bool u;
  char buf[LINESIZ];
    
  if ( !PL_get_chars(prefix, &p, CVT_ALL) )
    return warning("$complete_atom/3: instanstiation fault");
  strcpy(buf, p);

  if ( (s = extendAtom(p, &u)) )
  { strcat(buf, s);
    if ( PL_unify_list_chars(common, buf) &&
	 PL_unify_atom(unique, u ? ATOM_unique : ATOM_not_unique) )
      succeed;
  }

  fail;
}


static int
compareMatch(const void *m1, const void *m2)
{ return strcmp(stringMatch((Match)m1), stringMatch((Match)m2));
}


static bool
extend_alternatives(char *prefix, struct match *altv, int *altn)
{ Atom a = atomTable[0];
  char *as;
  int l;

  *altn = 0;
  for(; a; a=a->next)
  { while( a && isRef((word)a) )
      a = *((Atom *)unRef(a));
    if ( a == (Atom) NULL )
      break;
    
    as = stringAtom(a);
    if ( strprefix(as, prefix) &&
	 allAlpha(as) &&
	 (l = (int)strlen(as)) < ALT_SIZ )
    { Match m = &altv[(*altn)++];
      m->name = a;
      m->length = l;
      if ( *altn > ALT_MAX )
	break;
    }
  }
  
  qsort(altv, *altn, sizeof(struct match), compareMatch);

  succeed;
}


word
pl_atom_completions(term_t prefix, term_t alternatives)
{ char *p;
  char buf[LINESIZ];
  struct match altv[ALT_MAX];
  int altn;
  int i;
  term_t alts = PL_copy_term_ref(alternatives);
  term_t head = PL_new_term_ref();

  if ( !PL_get_chars(prefix, &p, CVT_ALL) )
    return warning("$atom_completions/2: instanstiation fault");
  strcpy(buf, p);

  extend_alternatives(buf, altv, &altn);
  
  for(i=0; i<altn; i++)
  { if ( !PL_unify_list(alts, head, alts) ||
	 !PL_unify_atom(head, altv[i].name) )
      fail;
  }

  return PL_unify_nil(alts);
} 


#define savestring(x) strcpy(xmalloc(1 + strlen(x)), (x))

char *
PL_atom_generator(char *prefix, int state)
{ static Atom a;

  if ( !state )
  { assert(!a);
    a = atomTable[0];
  } 

  for(; a; a=a->next)
  { char *as;
    int l;

    while( isRef((word)a) )
    { a = *((Atom *)unRef(a));
      if ( !a )
	return NULL;
    }
    
    assert(a->type == ATOM_TYPE);
    as = stringAtom(a);
    if ( strprefix(as, prefix) &&
	 allAlpha(as) &&
	 (l = strlen(as)) < ALT_SIZ )
    { a = a->next;
      return savestring(as);
    }
  }

  return NULL;
}

