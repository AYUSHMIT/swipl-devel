/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1992 University of Amsterdam. All rights reserved.
*/


#include <h/kernel.h>
#include <h/interface.h>		/* export for _markAnswerStack() */

static struct to_cell AnswerStackBaseCell;

void
pushAnswerObject(Any obj)
{ if ( isVirginObj(obj) )
  { ToCell c = alloc(sizeof(struct to_cell));

    DEBUG(NAME_gc, Cprintf("pushAnswerObject(%s)\n", pp(obj)));
    setAnswerObj(obj);
    c->value = obj;
    c->index = AnswerStack->index + 1;
    c->next  = AnswerStack;
    AnswerStack = c;
  }
}


void
deleteAnswerObject(Any obj)
{ if ( isAnswerObj(obj) )
  { ToCell c;

    DEBUG(NAME_gc, Cprintf("deleteAnswerObject(%s)\n", pp(obj)));

    c = AnswerStack;

    if ( c->value == obj )
    { AnswerStack = c->next;
      unalloc(sizeof(struct to_cell), c);
    } else
    { ToCell p = c;

      for( c = c->next; c; p = c, c = c->next)
      { if ( c->value == obj )
	{ p->next = c->next;
	  unalloc(sizeof(struct to_cell), c);
	  break;				/* can only be there once! */
	}
      }
    }

    clearAnswerObj(obj);
  }
}


export void
_markAnswerStack(AnswerMark *mark)
{ *mark = AnswerStack->index;
}


export void
_rewindAnswerStack(AnswerMark *mark, Any obj)
{ ToCell c, n;
  ToCell preserve = NULL;
  long index = *mark;

  for( c = AnswerStack; c->index > index; c = n )
  { n = c->next;
    DEBUG(NAME_gc, Cprintf("Cell at 0x%lx\n", (unsigned long)c));
    if ( c->value )
    { if ( c->value != obj )
      { Any o = c->value;

	if ( noRefsObj(o) && !onFlag(o, F_LOCKED|F_PROTECTED) )
	{ DEBUG(NAME_gc, 
		Cprintf("Removing %s from AnswerStack\n", pp(o)));
	  clearAnswerObj(c->value);
	  freeObject(o);
	}
	unalloc(sizeof(struct to_cell), c);
      } else
      	preserve = c;
    } else
      unalloc(sizeof(struct to_cell), c);
  }
  
  AnswerStack = c;

  if ( preserve )
  { preserve->next  = AnswerStack;
    preserve->index = AnswerStack->index + 1;
    AnswerStack     = preserve;
  }
}


void
initAnswerStack(void)
{ AnswerStack = &AnswerStackBaseCell;
  AnswerStack->index = 1;
  AnswerStack->value = 0;
  AnswerStack->next  = NULL;
}


void
resetAnswerStack(void)
{ ToCell c, n;

  for( c = AnswerStack; c != &AnswerStackBaseCell; c = n )
  { if ( c->value )
    { clearAnswerObj(c->value);
      /*freeableObj(c->value);		May be handy to leave the
					object for debugging */
    }
    n = c->next;
    unalloc(sizeof(struct to_cell), c);
  }

  initAnswerStack();
}


Int
countAnswerStack()
{ ToCell c;
  int n = 0;

  for( c = AnswerStack; c != &AnswerStackBaseCell; c = c->next )
    n++;

  answer(toInt(n));
}
