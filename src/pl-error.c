/*  $Id$

    Part of SWI-Prolog
    Designed and implemented by Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1997 University of Amsterdam. All rights reserved.
*/


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
throw(error(<Formal>, <SWI-Prolog>))

<SWI-Prolog>	::= context(Name/Arity, Message)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "pl-incl.h"
#ifndef EACCES
#include <errno.h>
#endif

static void
put_name_arity(term_t t, functor_t f)
{ FunctorDef fdef = valueFunctor(f);

  if ( fdef->arity == 0 )
    PL_put_atom(t, fdef->name);
  else
  { term_t a = PL_new_term_refs(2);

    PL_put_atom(a+0, fdef->name);
    PL_put_integer(a+1, fdef->arity);
    PL_cons_functor(t, FUNCTOR_divide2, a+0, a+1);
  }
}


int
PL_error(const char *pred, int arity, const char *msg, int id, ...)
{ term_t except = PL_new_term_ref();
  term_t formal = PL_new_term_ref();
  term_t swi	= PL_new_term_ref();
  va_list args;
  int do_throw = FALSE;

  if ( msg == MSG_ERRNO )
    msg = OsError();

					/* build (ISO) formal part  */
  va_start(args, id);
  switch(id)
  { case ERR_INSTANTIATION:
      err_instantiation:
      PL_unify_atom(formal, ATOM_instantiation_error);
      break;
    case ERR_TYPE:			/* ERR_INSTANTIATION if var(actual) */
    { atom_t expected = va_arg(args, atom_t);
      term_t actual   = va_arg(args, term_t);

      if ( PL_is_variable(actual) && expected != ATOM_variable )
	goto err_instantiation;

      PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_type_error2,
		      PL_ATOM, expected,
		      PL_TERM, actual);
      break;
    }
    case ERR_AR_TYPE:			/* arithmetic type error */
    { atom_t expected = va_arg(args, atom_t);
      Number num      = va_arg(args, Number);
      term_t actual   = PL_new_term_ref();

      _PL_put_number(actual, num);
      PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_type_error2,
		      PL_ATOM, expected,
		      PL_TERM, actual);
      break;
    }
    case ERR_AR_UNDEF:
    { PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_evaluation_error1,
		      PL_ATOM, ATOM_undefined);
      break;
    }
    case ERR_AR_OVERFLOW:
    { PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_evaluation_error1,
		      PL_ATOM, ATOM_float_overflow);
      break;
    }
    case ERR_AR_UNDERFLOW:
    { PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_evaluation_error1,
		      PL_ATOM, ATOM_float_underflow);
      break;
    }
    case ERR_DOMAIN:			/*  ERR_INSTANTIATION if var(arg) */
    { atom_t domain = va_arg(args, atom_t);
      term_t arg    = va_arg(args, term_t);

      if ( PL_is_variable(arg) )
	goto err_instantiation;

      PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_domain_error2,
		      PL_ATOM, domain,
		      PL_TERM, arg);
      break;
    }
    case ERR_REPRESENTATION:
    { atom_t what = va_arg(args, atom_t);

      PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_representation_error1,
		      PL_ATOM, what);
      break;
    }
    case ERR_MODIFY_STATIC_PROC:
    { Procedure proc = va_arg(args, Procedure);
      term_t pred = PL_new_term_ref();

      unify_definition(pred, proc->definition, 0, GP_NAMEARITY);
      PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_permission_error3,
		      PL_ATOM, ATOM_modify,
		      PL_ATOM, ATOM_static_procedure,
		      PL_TERM, pred);
      break;
    }
    case ERR_UNDEFINED_PROC:
    { Definition def = va_arg(args, Definition);
      term_t pred = PL_new_term_ref();

      unify_definition(pred, def, 0, GP_NAMEARITY);
      PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_existence_error2,
		      PL_ATOM, ATOM_procedure,
		      PL_TERM, pred);
      break;
    }
    case ERR_FAILED:
    { Procedure proc = va_arg(args, Procedure);
      term_t pred = PL_new_term_ref();

      unify_definition(pred, proc->definition, 0, GP_NAMEARITY);
      PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_failure_error1,
		      PL_TERM, pred);

      break;
    }
    case ERR_EVALUATION:
    { atom_t what = va_arg(args, atom_t);

      PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_evaluation_error1,
		      PL_ATOM, what);
      break;
    }
    case ERR_NOT_EVALUABLE:
    { functor_t f = va_arg(args, functor_t);
      term_t actual = PL_new_term_ref();

      put_name_arity(actual, f);
      
      PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_type_error2,
		      PL_ATOM, ATOM_evaluable,
		      PL_TERM, actual);
      break;
    }
    case ERR_DIV_BY_ZERO:
    { PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_evaluation_error1,
		      PL_ATOM, ATOM_zero_divisor);
      break;
    }
    case ERR_PERMISSION:
    { atom_t type = va_arg(args, atom_t);
      atom_t op   = va_arg(args, atom_t);
      term_t obj  = va_arg(args, term_t);

      PL_unify_term(formal,
			PL_FUNCTOR, FUNCTOR_permission_error3,
			  PL_ATOM, type,
			  PL_ATOM, op,
			  PL_TERM, obj);

      break;
    }
    case ERR_EXISTENCE:
    { atom_t type = va_arg(args, atom_t);
      term_t obj  = va_arg(args, term_t);

      PL_unify_term(formal,
			PL_FUNCTOR, FUNCTOR_existence_error2,
			  PL_ATOM, type,
			  PL_TERM, obj);

      break;
    }
    case ERR_FILE_OPERATION:
    { atom_t action = va_arg(args, atom_t);
      atom_t type   = va_arg(args, atom_t);
      term_t file   = va_arg(args, term_t);

      switch(errno)
      { case EACCES:
	  PL_unify_term(formal,
			PL_FUNCTOR, FUNCTOR_permission_error3,
			  PL_ATOM, action,
			  PL_ATOM, type,
			  PL_TERM, file);
	  break;
	case EMFILE:
	case ENFILE:
	  PL_unify_term(formal,
			PL_FUNCTOR, FUNCTOR_representation_error1,
			  PL_ATOM, ATOM_max_files);
	  break;
#ifdef EPIPE
	case EPIPE:
	  if ( !msg )
	    msg = "Broken pipe";
	  /*FALLTHROUGH*/
#endif
	default:			/* what about the other cases? */
	  PL_unify_term(formal,
			PL_FUNCTOR, FUNCTOR_existence_error2,
			  PL_ATOM, type,
			  PL_TERM, file);
	  break;
      }

      break;
    }
    case ERR_STREAM_OP:
    { atom_t action = va_arg(args, atom_t);
      term_t stream = va_arg(args, term_t);
      
      PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_io_error2,
		      PL_ATOM, action,
		      PL_TERM, stream);
      break;
    }
    case ERR_NOTIMPLEMENTED:		/* non-ISO */
    { atom_t what = va_arg(args, atom_t);

      PL_unify_term(formal,
			PL_FUNCTOR, FUNCTOR_not_implemented_error1,
			  PL_ATOM, what);
      break;
    }
    case ERR_RESOURCE:
    { atom_t what = va_arg(args, atom_t);

      PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_not_implemented_error1,
		      PL_ATOM, what);
      break;
    }
    case ERR_NOMEM:
    { PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_resource_error1,
		      PL_ATOM, ATOM_no_memory);

      break;
    }
    case ERR_SYSCALL:
    { atom_t op = va_arg(args, atom_t);

      if ( !msg )
	msg = PL_atom_chars(op);

      switch(errno)
      { case ENOMEM:
	  PL_unify_term(formal,
			PL_FUNCTOR, FUNCTOR_resource_error1,
			  PL_ATOM, ATOM_no_memory);
	  break;
	default:
	  PL_unify_atom(formal, ATOM_system_error);
	  break;
      }

      break;
    }
    case ERR_SHELL_FAILED:
    { term_t cmd = va_arg(args, term_t);

      PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_shell2,
		      PL_ATOM, ATOM_execute,
		      PL_TERM, cmd);
      break;
    }
    case ERR_SHELL_SIGNALLED:
    { term_t cmd = va_arg(args, term_t);
      int sig = va_arg(args, int);

      PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_shell2,
		      PL_FUNCTOR, FUNCTOR_signal1,
		        PL_INTEGER, sig,
		      PL_TERM, cmd);
      break;
    }
    case ERR_SIGNALLED:
    { int   sig     = va_arg(args, int);
      char *signame = va_arg(args, char *);

      PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_signal2,
			PL_CHARS,   signame,
		        PL_INTEGER, sig);
      break;
    }
    case ERR_CLOSED_STREAM:
    { IOSTREAM *s = va_arg(args, IOSTREAM *);

      PL_unify_term(formal,
		    PL_FUNCTOR, FUNCTOR_existence_error2,
		    PL_ATOM, ATOM_stream,
		    PL_POINTER, s);
      do_throw = TRUE;
    }
    case ERR_BUSY:
    { atom_t type  = va_arg(args, atom_t);
      term_t mutex = va_arg(args, term_t);

      PL_unify_term(formal, PL_FUNCTOR, FUNCTOR_busy2, type, mutex);
    }
    default:
      assert(0);
  }
  va_end(args);

					/* build SWI-Prolog context term */
  if ( pred || msg )
  { term_t predterm = PL_new_term_ref();
    term_t msgterm  = PL_new_term_ref();

    if ( pred )
    { PL_unify_term(predterm,
		    PL_FUNCTOR, FUNCTOR_divide2,
		      PL_CHARS, pred,
		      PL_INTEGER, arity);
    }
    if ( msg )
    { PL_put_atom_chars(msgterm, msg);
    }

    PL_unify_term(swi,
		  PL_FUNCTOR, FUNCTOR_context2,
		    PL_TERM, predterm,
		    PL_TERM, msgterm);
  }

  PL_unify_term(except,
		PL_FUNCTOR, FUNCTOR_error2,
		  PL_TERM, formal,
		  PL_TERM, swi);


  if ( do_throw )
    return PL_throw(except);
  else
    return PL_raise_exception(except);
}


char *
tostr(char *buf, const char *fmt, ...)
{ va_list args;

  va_start(args, fmt);
  Svsprintf(buf, fmt, args);
  va_end(args);
  
  return buf;
}
