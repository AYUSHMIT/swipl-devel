/*  $Id$

    Copyright (c) 1990 Jan Wielemaker. All rights reserved.
    See ../LICENCE to find out about your rights.
    jan@swi.psy.uva.nl

    Purpose: Support predicates for bagof
*/

/*#define O_SECURE 1*/
#include "pl-incl.h"

#undef LD
#define LD LOCAL_LD

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This module defines support  predicates  for  the  Prolog  all-solutions
predicates findall/3, bagof/3 and setof/3.  These predicates are:

	$record_bag(Key, Value)		Record a value under a key.
    	$collect_bag(Bindings, Values)	Retract all Solutions matching
					Bindings.

The (toplevel) remainder of the all-solutions predicates is  written  in
Prolog.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

struct assoc
{ Record	binding;
  Assoc		next;			/* next in chain */
};

#define alist LD->bags.bags		/* Each thread has its own */
					/* storage for this */

static void
freeAssoc(Assoc prev, Assoc a)
{ if ( prev == NULL )
  { GET_LD
    alist = a->next;
  } else
    prev->next = a->next;
  if ( a->binding )
    freeRecord(a->binding);
  freeHeap(a, sizeof(*a));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
$record_bag(Key-Value)

Record a solution of bagof.  Key is a term  v(V0,  ...Vn),  holding  the
variable binding for solution `Gen'.  Key is ATOM_mark for the mark.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

word
pl_record_bag(term_t t)
{ GET_LD
  Assoc a = (Assoc) allocHeap(sizeof(struct assoc));

  if ( PL_is_atom(t) )
    a->binding = NULL;
  else
    a->binding = compileTermToHeap(t);
  a->next    = alist;
  alist      = a;

  succeed;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This predicate will fail if no more records are left before the mark.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

word
pl_collect_bag(term_t bindings, term_t bag)
{ GET_LD
  term_t var_term = PL_new_term_ref();	/* v() term on global stack */
  term_t list     = PL_new_term_ref();	/* list to construct */
  term_t binding  = PL_new_term_ref();	/* current binding */
  term_t tmp      = PL_new_term_ref();
  Assoc a, next;
  Assoc prev = (Assoc) NULL;
  
  if ( !(a = alist) )
    fail;
  if ( !a->binding )
  { freeAssoc(prev, a);
    fail;				/* trapped the mark */
  }

  PL_put_nil(list);
					/* get variable term on global stack */
  copyRecordToGlobal(binding, a->binding PASS_LD);
  PL_get_arg(1, binding, var_term);
  PL_unify(bindings, var_term);
  PL_get_arg(2, binding, tmp);
  PL_cons_list(list, tmp, list);

  next = a->next;
  freeAssoc(prev, a);  

  if ( next != NULL )
  { for( a = next, next = a->next; next; a = next, next = a->next )
    { if ( !a->binding )
	break;

      if ( !structuralEqualArg1OfRecord(var_term, a->binding PASS_LD) )
      { prev = a;
	continue;
      }

      copyRecordToGlobal(binding, a->binding PASS_LD);
      PL_get_arg(1, binding, tmp);
      PL_unify(tmp, bindings);
      PL_get_arg(2, binding, tmp);
      PL_cons_list(list, tmp, list);
      SECURE(checkData(valTermRef(list)));
      freeAssoc(prev, a);
    }
  }

  SECURE(checkData(valTermRef(var_term)));

  return PL_unify(bag, list);
}



