/*  $Id$

    Part of XPCE

    Author:  Jan Wielemaker and Anjo Anjewierden
    E-mail:  jan@swi.psy.uva.nl
    WWW:     http://www.swi.psy.uva.nl/projects/xpce/
    Copying: GPL-2.  See the file COPYING or http://www.gnu.org

    Copyright (C) 1990-2001 SWI, University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>

static status
initialiseOrv(Or or, int argc, Any *argv)
{ int n;

  initialiseCode((Code) or);
  assign(or, members,   newObject(ClassChain, 0));

  for(n=0; n<argc; n++)
    appendChain(or->members, argv[n]);

  succeed;
}


		/********************************
		*           EXECUTION		*
		********************************/

static status
ExecuteOr(Or or)
{ Cell cell;

  for_cell(cell, or->members)
  { if ( executeCode(cell->value) != FAIL )
      succeed;
  }

  fail;
}

		/********************************
		*      TERM REPRESENTATION	*
		********************************/

static Int
getArityOr(Or or)
{ answer(getArityChain(or->members));
}


static Any
getArgOr(Or or, Int n)
{ answer(getArgChain(or->members, n));
}


		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */


/* Instance Variables */

static vardecl var_or[] =
{ IV(NAME_members, "chain", IV_GET,
     NAME_statement, "One of these must succeed")
};

/* Send Methods */

static senddecl send_or[] =
{ SM(NAME_Execute, 0, NULL, ExecuteOr,
     DEFAULT, "Evaluate tests until one succeeds"),
  SM(NAME_initialise, 1, "test=code ...", initialiseOrv,
     DEFAULT, "Create from tests")
};

/* Get Methods */

static getdecl get_or[] =
{ GM(NAME_Arg, 1, "code", "int", getArgOr,
     DEFAULT, "Nth-1 argument for term description"),
  GM(NAME_Arity, 0, "int", NULL, getArityOr,
     DEFAULT, "Arity for term description")
};

/* Resources */

#define rc_or NULL
/*
static classvardecl rc_or[] =
{ 
};
*/

/* Class Declaration */

ClassDecl(or_decls,
          var_or, send_or, get_or, rc_or,
          ARGC_UNKNOWN, NULL,
          "$Rev$");

status
makeClassOr(Class class)
{ declareClass(class, &or_decls);
  delegateClass(class, NAME_members);

  succeed;
}

