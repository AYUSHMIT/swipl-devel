/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1992 University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>


static status
initialiseNotv(Not n, Code arg)
{ initialiseCode((Code) n);
  assign(n, argument, arg);

  succeed;
}


		/********************************
		*           EXECUTION		*
		********************************/

static status
ExecuteNot(Not n)
{ if ( executeCode(n->argument) != FAIL )
    fail;

  succeed;
}

		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */


/* Instance Variables */

static vardecl var_not[] =
{ IV(NAME_argument, "code", IV_BOTH,
     NAME_statement, "Test to negate")
};

/* Send Methods */

static senddecl send_not[] =
{ SM(NAME_Execute, 0, NULL, ExecuteNot,
     DEFAULT, "Evaluate argument test and negate result"),
  SM(NAME_initialise, 1, "test=code", initialiseNotv,
     DEFAULT, "Create from test")
};

/* Get Methods */

#define get_not NULL
/*
static getdecl get_not[] =
{ 
};
*/

/* Resources */

#define rc_not NULL
/*
static resourcedecl rc_not[] =
{ 
};
*/

/* Class Declaration */

static Name not_termnames[] = { NAME_argument };

ClassDecl(not_decls,
          var_not, send_not, get_not, rc_not,
          1, not_termnames,
          "$Rev$");


status
makeClassNot(Class class)
{ return declareClass(class, &not_decls);
}

