/*  $Id$

    Part of XPCE

    Author:  Jan Wielemaker and Anjo Anjewierden
    E-mail:  jan@swi.psy.uva.nl
    WWW:     http://www.swi.psy.uva.nl/projects/xpce/
    Copying: GPL-2.  See the file COPYING or http://www.gnu.org

    Copyright (C) 1990-2001 SWI, University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>
#include <h/graphics.h>

static EventTreeObj	getTreeEventNode(EventNodeObj n);

static status
initialiseEventNode(EventNodeObj n, Any value, EventNodeObj parent)
{ if ( isDefault(parent) )
    parent = NIL;

  assign(n, value,  value);
  assign(n, parent, parent);
  assign(n, sons,   NIL);

  succeed;
}


static status
sonEventNode(EventNodeObj n, EventNodeObj son)
{ if ( notNil(son->parent) )
    return errorPce(son, NAME_alreadyHasParent);

  if ( isNil(n->sons) )
    assign(n, sons, newObject(ClassChain, EAV));

  appendChain(n->sons, son);
  son->parent = n;
  addNodeEventTree(getTreeEventNode(n), son);
  
  succeed;
}


status
isAEventNode(EventNodeObj sb, EventNodeObj super)
{ do
  { if ( sb == super )
      succeed;
    sb = sb->parent;
  } while( isObject(sb) );

  fail;
}


static EventTreeObj
getTreeEventNode(EventNodeObj n)
{ while( instanceOfObject(n->parent, ClassEventNode) )
    n = n->parent;

  if ( instanceOfObject(n->parent, ClassEventTree) )
    answer((EventTreeObj) n->parent);

  fail;
}


		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */

static char *T_initialise[] =
        { "value=name", "parent=[event_node]*" };

/* Instance Variables */

static vardecl var_eventNode[] =
{ IV(NAME_value, "event_id", IV_GET,
     NAME_value, "Value of the node"),
  IV(NAME_parent, "event_node|event_tree", IV_GET,
     NAME_hierarchy, "Direct parent of the node"),
  IV(NAME_sons, "chain*", IV_GET,
     NAME_hierarchy, "Chain of direct sons")
};

/* Send Methods */

static senddecl send_eventNode[] =
{ SM(NAME_initialise, 2, T_initialise, initialiseEventNode,
     DEFAULT, "Create from value and parent"),
  SM(NAME_son, 1, "event_node", sonEventNode,
     NAME_edit, "Add a son to the node"),
  SM(NAME_isA, 1, "event_node", isAEventNode,
     NAME_test, "Test if node is a subnode of argument")
};

/* Get Methods */

static getdecl get_eventNode[] =
{ GM(NAME_tree, 0, "event_tree", NULL, getTreeEventNode,
     NAME_organisation, "The tree holding this event_node")
};

/* Resources */

#define rc_eventNode NULL
/*
static classvardecl rc_eventNode[] =
{ 
};
*/

/* Class Declaration */

static Name eventNode_termnames[] = { NAME_value, NAME_parent };

ClassDecl(eventNode_decls,
          var_eventNode, send_eventNode, get_eventNode, rc_eventNode,
          2, eventNode_termnames,
          "$Rev$");

status
makeClassEventNode(Class class)
{ return declareClass(class, &eventNode_decls);
}

