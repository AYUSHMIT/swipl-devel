/*  $Id$

    Copyright (c) 1990 Jan Wielemaker. All rights reserved.
    See ../LICENCE to find out about your rights.
    jan@swi.psy.uva.nl

    Purpose: write/1 and display/1 definition
*/

#include "pl-incl.h"
#include "pl-ctype.h"
extern int Output;

forwards char *	varName(Word);
forwards void	writePrimitive(Word, bool);
forwards bool	display(Word, bool);
forwards int	priorityOperator(Atom);
forwards bool	writeTerm(Word, int, bool, Word);
forwards word	displayStream(Word, Word, bool);
forwards word	writeStreamTerm(Word, Word, int, int, Word);

static char *
varName(Word adr)
{ static char name[10];

  if (adr > (Word) lBase)
    Ssprintf(name, "L%ld", (long)adr - (long)lBase);
  else
    Ssprintf(name, "G%ld", (long)adr - (long)gBase);

  return name;
}


#define AT_LOWER	0
#define AT_QUOTE	1
#define AT_FULLSTOP	2
#define AT_SYMBOL	3
#define AT_SOLO		4
#define AT_SPECIAL	5

static int
atomType(Atom a)
{ char *s = stringAtom(a);

  if ( isLower(*s) )
  { char *s2;

    for(s2 = s; *s2 && isAlpha(*s2); )
      s2++;
    return *s2 == EOS ? AT_LOWER : AT_QUOTE;
  }

  if ( streq(s, ".") )
    return AT_FULLSTOP;
  
  if (isSymbol(*s))
  { char *s2;

    for(s2 = s; *s2 && isSymbol(*s2); )
      s2++;
    return *s2 == EOS ? AT_SYMBOL : AT_QUOTE;
  }

  if ((isSolo(*s) || *s == ',') && s[1] == EOS)
    return AT_SOLO;

  if (streq(s, "[]") || streq(s, "{}") )
    return AT_SPECIAL;
  
  return AT_QUOTE;
}


static void
writePrimitive(Word w, bool quote)
{ char *s, c;

  DEBUG(9, Sdprintf("writing primitive at 0x%x: 0x%x\n", w, *w));

  if (isInteger(*w))
  { Putf("%ld", valNum(*w));
    return;
  }

  if (isReal(*w))
  { Putf("%f", valReal(*w));
    return;
  }

#if O_STRING
  if ( isString(*w) )
  { s = valString(*w);
    if ( quote == TRUE )
    { Put('\"');
      while( (c = *s++) != EOS )
      { if ( c == '"' )
          Put('"');
        Put(c);
      }
      Put('\"');
    } else
    { Putf("%s", s);
    }
    return;
  }
#endif /* O_STRING */

  if (isVar(*w))
  { Putf("%s", varName(w) );
    return;
  }    

  if (isAtom(*w))
  { if (quote == TRUE)
    { switch( atomType((Atom) *w) )
      { case AT_LOWER:
	case AT_SYMBOL:
	case AT_SOLO:
	case AT_SPECIAL:
	{ char *s = stringAtom(*w);

	  for(; *s; s++)
	    Put(*s);

          break;
	}
        case AT_QUOTE:
        case AT_FULLSTOP:
	default:
	{ char *s = stringAtom(*w);
	  char c;

	  Put('\'');
	  while( (c = *s++) != EOS )
	    if (c == '\'')
	      Putf("''");
	  else
	    Put(c);
	  Put('\'');
	}
      }
    } else
      Putf("%s", stringAtom(*w) );
  }
}

word
pl_nl(void)
{ return Put('\n');
}

word
pl_nl1(Word stream)
{ streamOutput(stream, pl_nl());
}


static bool
display(Word t, bool quote)
{ int n;
  int arity;
  Word arg;

  DEBUG(9, Sdprintf("display term at 0x%x; ", t));
  deRef(t);
  DEBUG(9, Sdprintf("after deRef() at 0x%x\n", t));

  if (isPrimitive(*t) )
  { DEBUG(9, Sdprintf("primitive\n"));
    writePrimitive(t, quote);
    succeed;
  }

  arity = functorTerm(*t)->arity;
  arg = argTermP(*t, 0);
  DEBUG(9, Sdprintf("Complex; arg0 at 0x%x, arity = %d\n", arg, arity));

  DEBUG(9, Sdprintf("functorTerm() = 0x%x, ->name = 0x%x\n",
				functorTerm(*t), functorTerm(*t)->name));
  if ( functorTerm(*t)->name == ATOM_comma )
    Putf("','");
  else
    writePrimitive((Word)&(functorTerm(*t)->name), quote);
  Putf("(");
  for(n=0; n<arity; n++, arg++)
  { if (n > 0)
      Putf(", ");
    display(arg, quote);
  }
  Putf(")");

  succeed;
}

word
pl_display(Word term)
{ return display(term, FALSE);
}

word
pl_displayq(Word term)
{ return display(term, TRUE);
}

static word
displayStream(Word stream, Word term, bool quote)
{ streamOutput(stream, display(term, quote));
}

word
pl_display2(Word stream, Word term)
{ return displayStream(stream, term, FALSE);
}

word
pl_displayq2(Word stream, Word term)
{ return displayStream(stream, term, TRUE);
}

static int
priorityOperator(Atom atom)
{ int type, priority;
  int result = 0;

  if (isPrefixOperator(atom, &type, &priority) && priority > result)
    result = priority;
  if (isPostfixOperator(atom, &type, &priority) && priority > result)
    result = priority;
  if (isInfixOperator(atom, &type, &priority) && priority > result)
    result = priority;

  return result;
}

/*  write a term. The 'enviroment' precedence is prec. 'style' askes
    for plain writing (write/1), quoting (writeq/1) or portray (print/1)

 ** Sun Apr 17 12:48:09 1988  jan@swivax.UUCP (Jan Wielemaker)  */

#define PLAIN		0
#define QUOTE_ATOMS	1
#define PORTRAY		2

/*  Call Prolog predicate $portray/1 on 'term'. Succeed or fail
    according to the result.

 ** Sun Jun  5 15:37:12 1988  jan@swivax.UUCP (Jan Wielemaker)  */

static bool
pl_call2(Word goal, Word arg)
{ Module mod = NULL;
  Atom name;
  int arity;
  word g;
  mark m;
  int n;
  bool rval;

  TRY(goal = stripModule(goal, &mod));
  deRef(goal);

  if ( isAtom(*goal) )
  { name = (Atom) *goal;
    arity = 0;
  } else if ( isTerm(*goal) )
  { name = functorTerm(*goal)->name;
    arity = functorTerm(*goal)->arity;
  } else
    return warning("call/2: instantiation fault");
  
  Mark(m);
  g = globalFunctor(lookupFunctorDef(name, arity+1));
  for(n = 0; n < arity; n++)
    pl_unify(argTermP(g, n), argTermP(goal, n));
  pl_unify(argTermP(g, n), arg);
  debugstatus.suspendTrace++;
  rval = callGoal(mod, g, FALSE);
  debugstatus.suspendTrace--;
  Undo(m);

  return rval;
}


static bool
needSpace(word w1, word w2)
{ if ( isAtom(w1) && atomType((Atom) w1) == AT_SYMBOL &&
       ((isAtom(w2) && atomType((Atom) w2) == AT_LOWER) ||
	isVar(w2)) )
    fail;

  succeed;
}


static bool
writeTerm(Word term, int prec, bool style, Word g)
{ Atom functor;
  int arity, n;
  int op_type, op_pri;
  Word arg;
  bool quote = (style != PLAIN);

  deRef(term);

  if ( !isVar(*term) && style == PORTRAY && pl_call2(g, term) )
    succeed;

  if (isPrimitive(*term) )
  { if (isAtom(*term) && priorityOperator((Atom)*term) > prec)
    { Put('(');
      writePrimitive(term, quote);
      Put(')');
    } else
      writePrimitive(term, quote);

    succeed;
  }

  functor = functorTerm(*term)->name;
  arity = functorTerm(*term)->arity;
  arg = argTermP(*term, 0);

  if (arity == 1)
  { if (functor == ATOM_curl)
    { Put('{');
      for(;;)
      { deRef(arg);
	if (!isTerm(*arg) || functorTerm(*arg) != FUNCTOR_comma2)
	  break;
	writeTerm(argTermP(*arg, 0), 999, style, g);
	Put(',');
	arg = argTermP(*arg, 1);
      }
      writeTerm(arg, 999, style, g);      
      Put('}');
      succeed;
    }
    if (isPrefixOperator(functor, &op_type, &op_pri) )
    { if (op_pri > prec)
	Put('(');
      writePrimitive((Word) &functor, quote);
      if ( needSpace((word) functor, *arg) )
	Put(' ');
      writeTerm(arg, op_type == OP_FX ? op_pri-1 : op_pri, style, g);
      if (op_pri > prec)
	Put(')');
      succeed;
    }
    if (isPostfixOperator(functor, &op_type, &op_pri) )
    { if (op_pri > prec)
	Put('(');
      writeTerm(arg, op_type == OP_XF ? op_pri-1 : op_pri, style, g);
      if ( needSpace((word) functor, *arg) )
	Put(' ');
      writePrimitive((Word)&functor, quote);
      if (op_pri > prec)
	Put(')');
      succeed;
    }
  } else if (arity == 2)
  { if (functor == ATOM_dot)
    { Put('[');
      for(;;)
      { writeTerm(arg++, 999, style, g);
	deRef(arg);
	if (*arg == (word) ATOM_nil)
	  break;
	if (!isList(*arg) )
	{ Put('|');
	  writeTerm(arg, 999, style, g);
	  break;
	}
	Put(',');
	arg = HeadList(arg);
      }
      Put(']');
      succeed;
    }
    if (isInfixOperator(functor, &op_type, &op_pri) )
    { if (op_pri > prec)
	Put('(');
      writeTerm(arg++, 
		op_type == OP_XFX || op_type == OP_XFY ? op_pri-1 : op_pri, 
		style, g);
      if (functor != ATOM_comma)
	Put(' ');
      writePrimitive((Word)&functor, quote);
      Put(' ');
      writeTerm(arg, 
		op_type == OP_XFX || op_type == OP_YFX ? op_pri-1 : op_pri, 
		style, g);
      if (op_pri > prec)
	Put(')');
      succeed;
    }
  }

  writePrimitive((Word)&functor, quote);
  Put('(');
  for(n=0; n<arity; n++, arg++)
  { if (n > 0)
      Putf(", ");
    writeTerm(arg, 999, style, g);
  }
  Put(')');

  succeed;
}

word
pl_write(Word term)
{ writeTerm(term, 1200, PLAIN, NULL);

  succeed;
}

word
pl_writeq(Word term)
{ writeTerm(term, 1200, QUOTE_ATOMS, NULL);

  succeed;
}

word
pl_print(Word term)
{ word g = (word) ATOM_portray;

  writeTerm(term, 1200, PORTRAY, &g);

  succeed;
}

word
pl_dprint(Word term, Word g)
{ writeTerm(term, 1200, PORTRAY, g);

  succeed;
}

static word
writeStreamTerm(Word stream, Word term, int prec, int style, Word g)
{ streamOutput(stream, writeTerm(term, prec, style, g));
}

word
pl_write2(Word stream, Word term)
{ return writeStreamTerm(stream, term, 1200, PLAIN, NULL);
}

word
pl_writeq2(Word stream, Word term)
{ return writeStreamTerm(stream, term, 1200, QUOTE_ATOMS, NULL);
}

word
pl_print2(Word stream, Word term)
{ word g = (word) ATOM_portray;

  return writeStreamTerm(stream, term, 1200, PORTRAY, &g);
}
