/*  $Id$

    Part of XPCE
    Designed and implemented by Anjo Anjewierden and Jan Wielemaker
    E-mail: jan@swi.psy.uva.nl

    Copyright (C) 1992 University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>
#include <h/unix.h>			/* storeCharpFile() prototype */

		/********************************
		*         CREATE/CONVERT	*
		********************************/


status
initialiseCharArray(CharArray n, CharArray value)
{ str_cphdr(&n->data, &value->data);
  str_alloc(&n->data);
  if ( value->data.readonly )
    n->data.s_text8 = value->data.s_text8;
  else
    memcpy(n->data.s_text8, value->data.s_text8, str_datasize(&n->data));

  succeed;
}


static status
unlinkCharArray(CharArray n)
{ str_unalloc(&n->data);

  succeed;
}


static status
cloneCharArray(CharArray str, CharArray clone)
{ clonePceSlots(str, clone);
  clone->data = str->data;
  str_alloc(&clone->data);
  memcpy(clone->data.s_text8, str->data.s_text8, str_datasize(&str->data));

  succeed;
}


static status
storeCharArray(CharArray s, FileObj file)
{ TRY(storeSlotsObject(s, file));
  return storeCharpFile(file, s->data.s_text8); /* TBD: full store! */
}


static status
loadCharArray(CharArray s, FILE *fd, ClassDef def)
{ char *data;

  TRY(loadSlotsObject(s, fd, def));
  if ( (data = loadCharp(fd)) )
  { String str = &s->data;
    
    str_inithdr(str, ENC_ASCII);
    str->size     = strlen(data);
    str->s_text8  = data;
  }

  succeed;
}


static CharArray
getConvertCharArray(Any ctx, Any val)
{ string s;

  TRY(toString(val, &s));
  answer(stringToCharArray(&s));
}


Name
getValueCharArray(CharArray n)
{ answer(StringToName(&n->data));
}


		/********************************
		*             TESTS		*
		********************************/

status
equalCharArray(CharArray n1, CharArray n2)
{ if ( str_eq(&n1->data, &n2->data) )
    succeed;

  fail;
}


status
prefixCharArray(CharArray n1, CharArray n2) /* n2 is prefix of n1 */
{ if ( str_prefix(&n1->data, &n2->data) )
    succeed;

  fail;
}


status
suffixCharArray(CharArray n, CharArray s, Bool ign_case)
{ if ( ign_case == ON )
    return str_icase_suffix(&n->data, &s->data);
  else
    return str_suffix(&n->data, &s->data);
}


static status
largerCharArray(CharArray n1, CharArray n2)
{ if ( str_cmp(&n1->data, &n2->data) > 0 )
    succeed;
  fail;
}


static status
smallerCharArray(CharArray n1, CharArray n2)
{ if ( str_cmp(&n1->data, &n2->data) < 0 )
    succeed;
  fail;
}


static status
subCharArray(CharArray n1, CharArray n2, Bool ign_case)
{ if ( ign_case != ON )
  { if ( str_sub(&n1->data, &n2->data) )
      succeed;
  } /*else
  { if ( str_icasesub(&n1->data, &n2->data) )
      succeed;
  }*/

  fail;
}

		/********************************
		*         MODIFICATIONS		*
		********************************/

static CharArray
getModifyCharArray(CharArray n, CharArray n2)
{ answer(answerObject(classOfObject(n), n2, 0));
}


static CharArray
ModifiedCharArray(CharArray n, String buf)
{
  Class class = classOfObject(n);

  if ( class == ClassName )
    return (CharArray) StringToName(buf);
  else if ( class == ClassString)
    return (CharArray) StringToString(buf);	/* ??? */
  else
  { CharArray scratch = StringToScratchCharArray(buf);
    CharArray rval = get(n, NAME_modify, scratch, 0);

    doneScratchCharArray(scratch);
    answer(rval);
  }
}


CharArray
getCopyCharArray(CharArray n)
{ answer(ModifiedCharArray(n, &n->data));
}


CharArray
getCapitaliseCharArray(CharArray n)
{ if ( n->data.size == 0 )
    answer(n);
  else
  { String d = &n->data;
    int size = d->size;
    LocalString(buf, d, size);
    int i=1, o=1;

    str_store(buf, 0, toupper(str_fetch(d, 0)));

    for(; i < size; i++, o++)
    { wchar c = str_fetch(d, i);
      
      if ( iswordsep(c) )
      { if ( ++i < size )
	  str_store(buf, o, toupper(str_fetch(d, i)));
      } else
	str_store(buf, o, tolower(c));
    }

    buf->size = o;
    answer(ModifiedCharArray(n, buf));
  }
}


CharArray
getLabelNameCharArray(CharArray n)
{ String s = &n->data;
  int size = s->size;
  int i, l = 0, u = 0;

  if ( size == 0 )
    return n;

  for(i=0; i < size; i++)
  { l |= isupper(str_fetch(s, i));
    u |= islower(str_fetch(s, i));

    if ( (u && l) || (!u && !l) )		/* mixed case or no letters */
      return n;
  }

  { LocalString(buf, s, size);
    int o = 0;

    i = 0;
    str_store(buf, o, toupper(str_fetch(s, i)));
    i++, o++;

    for( ; i < size; i++, o++ )
    { if ( iswordsep(str_fetch(s, i)) )
      { str_store(buf, o, ' ');
	i++, o++;
	str_store(buf, o, toupper(str_fetch(s, i)));
      } else
	str_store(buf, o, tolower(str_fetch(s, i)));
    }

    answer(ModifiedCharArray(n, buf));
  }
}


CharArray
getDowncaseCharArray(CharArray n)
{ String s = &n->data;
  int size = s->size;
  LocalString(buf, s, size);
  int i;

  for(i=0; i<size; i++)
    str_store(buf, i, tolower(str_fetch(s, i)));
  buf->size = size;

  answer(ModifiedCharArray(n, buf));
}


static CharArray
getUpcaseCharArray(CharArray n)
{ String s = &n->data;
  int size = s->size;
  LocalString(buf, s, size);
  int i;

  for(i=0; i<size; i++)
    str_store(buf, i, toupper(str_fetch(s, i)));
  buf->size = size;

  answer(ModifiedCharArray(n, buf));
}


CharArray
getAppendCharArray(CharArray n1, CharArray n2)
{ String s1 = &n1->data;
  String s2 = &n2->data;
  LocalString(buf, s1, s1->size + s2->size);
  int n;

  buf->size = s1->size + s2->size;
  memcpy(buf->s_text8, s1->s_text8, (n=str_datasize(s1)));
  memcpy(&buf->s_text8[n], s2->s_text8, str_datasize(s2));

  answer(ModifiedCharArray(n1, buf));
}


static CharArray
getAppendCharArrayv(CharArray ca, int argc, CharArray *argv)
{ int l = ca->data.size;
  int i;

  for( i=0; i<argc; i++ )
    l += argv[i]->data.size;
       
  { LocalString(buf, &ca->data, l);

    if ( isstr8(&ca->data) )
    { char8 *d = buf->s_text8;
      
      memcpy(d, ca->data.s_text8, ca->data.size * sizeof(char8));
      d += ca->data.size;

      for( i=0; i<argc; i++ )
      { memcpy(d, argv[i]->data.s_text8, argv[i]->data.size*sizeof(char8));
	d += argv[i]->data.size;
      }
    } else
    { char16 *d = buf->s_text16;
      
      memcpy(d, ca->data.s_text16, ca->data.size * sizeof(char16));
      d += ca->data.size;

      for( i=0; i<argc; i++ )
      { memcpy(d, argv[i]->data.s_text16, argv[i]->data.size*sizeof(char16));
	d += argv[i]->data.size;
      }
    }

    buf->size = l;
    answer(ModifiedCharArray(ca, buf));
  }
}


CharArray
getDeleteSuffixCharArray(CharArray n, CharArray s)
{ if ( suffixCharArray(n, s, OFF) )
  { string buf;

    str_cphdr(&buf, &n->data);
    buf.s_text = n->data.s_text;
    buf.size = n->data.size - s->data.size;

    answer(ModifiedCharArray(n, &buf));
  }

  fail;
}


CharArray
getEnsureSuffixCharArray(CharArray n, CharArray s)
{ if ( suffixCharArray(n, s, OFF) )
    answer(n);

  answer(getAppendCharArray(n, s));
}


CharArray
getDeletePrefixCharArray(CharArray n, CharArray s)
{ if ( prefixCharArray(n, s) )
  { string buf;

    str_cphdr(&buf, &n->data);
    buf.size = n->data.size - s->data.size;
    if ( isstr8(&buf) )
      buf.s_text8 = &n->data.s_text8[s->data.size];
    else
      buf.s_text16 = &n->data.s_text16[s->data.size];

    answer(ModifiedCharArray(n, &buf));
  }

  fail;
}


static CharArray
getSubCharArray(CharArray n, Int start, Int end)
{ string s;
  int x, y;
  int len = n->data.size;

  x = valInt(start);
  y = (isDefault(end) ? len : valInt(end));
  if ( x < 0 || y > len || x > y )
    fail;

  str_cphdr(&s, &n->data);
  s.size = y-x;
  if ( isstr8(&n->data) )
    s.s_text8 = &n->data.s_text8[x];
  else
    s.s_text16 = &n->data.s_text16[x];
  
  answer(ModifiedCharArray(n, &s));
}


		/********************************
		*          READING GETS		*
		********************************/

Int
getSizeCharArray(Any n)
{ CharArray c = n;

  answer(toInt(c->data.size));
}


static Int
getCharacterCharArray(CharArray n, Int idx)
{ int i = valInt(idx);

  if ( i < 0 || i >= n->data.size )	
    fail;

  answer(toInt(str_fetch(&n->data, i)));
}


static Int
getIndexCharArray(CharArray n, Int chr, Int here)
{ wchar c = valInt(chr);
  int h;

  h = (isDefault(here) ? 0 : valInt(here));
  if ( (h = str_next_index(&n->data, h, c)) >= 0 )
    answer(toInt(h));

  fail;
}


static Int
getRindexCharArray(CharArray n, Int chr, Int here)
{ wchar c = valInt(chr);
  int h, len = n->data.size;

  h = (isDefault(here) ? (len - 1) : valInt(here));
  if ( (h = str_next_rindex(&n->data, h, c)) >= 0 )
    answer(toInt(h));

  fail;
}


static Int
getLineNoCharArray(CharArray name, Int caret)
{ int here = (isDefault(caret) ? name->data.size : valInt(caret));

  answer(toInt(str_lineno(&name->data, here)));
}


static Vector
getScanCharArray(CharArray n, CharArray fmt)
{ if ( isstr8(&n->data) && isstr8(&fmt->data) )
  { Any argv[SCAN_MAX_ARGS];
    Int argc;

    TRY(argc = scanstr(n->data.s_text8, fmt->data.s_text8, argv));
    
    answer(answerObjectv(ClassVector, valInt(argc), argv));
  } else
  { errorPce(n, NAME_notSupportedForChar16);
    fail;
  }
}


static Name
getCompareCharArray(CharArray n1, CharArray n2, Bool ignore_case)
{ int rval;

  if ( ignore_case == ON )
    rval = str_icase_cmp(&n1->data, &n2->data);
  else
    rval = str_cmp(&n1->data, &n2->data);

  if ( rval < 0 )
    answer(NAME_smaller);
  else if ( rval == 0 )
    answer(NAME_equal);
  else
    answer(NAME_larger);

  assert(0);				/* should not get here! */
  fail;
}


		/********************************
		*          C-CONVERSIONS	*
		********************************/

#define SCRATCH_CHAR_ARRAYS	(10)

static CharArray scratch_char_arrays;

void
initCharArrays(void)
{ CharArray ca;
  int n;

  scratch_char_arrays = alloc(sizeof(struct char_array) * SCRATCH_CHAR_ARRAYS);

  for(ca=scratch_char_arrays, n = 0; n < SCRATCH_CHAR_ARRAYS; ca++, n++)
  { initHeaderObj(ca, ClassCharArray);
    setProtectedObj(ca);
    createdObject(ca, NAME_new);
  }
}


CharArray
CtoScratchCharArray(const char *s)
{ CharArray name = scratch_char_arrays;
  int n;

  for(n = 0; n < SCRATCH_CHAR_ARRAYS; n++, name++)
    if ( name->data.s_text8 == NULL )
    { str_inithdr(&name->data, ENC_ASCII);
      name->data.size = strlen(s);
      name->data.s_text8 = (char *) s;

      return name;
    }

  NOTREACHED;
  fail;
}


CharArray
StringToScratchCharArray(const String s)
{ CharArray name = scratch_char_arrays;
  int n;

  for(n = 0; n < SCRATCH_CHAR_ARRAYS; n++, name++)
    if ( name->data.s_text8 == NULL )
    { str_cphdr(&name->data, s);
      name->data.s_text = s->s_text;
      return name;
    }

  NOTREACHED;
  fail;
}


void
doneScratchCharArray(CharArray n)
{ n->data.s_text = NULL;
}


CharArray
CtoCharArray(char *s)
{ CharArray name = CtoScratchCharArray(s);
  CharArray rval = answerObject(ClassCharArray, name, 0);
  
  doneScratchCharArray(name);
  return rval;
}


CharArray
stringToCharArray(String s)
{ CharArray name = StringToScratchCharArray(s);
  CharArray rval = answerObject(ClassCharArray, name, 0);
  
  doneScratchCharArray(name);
  return rval;
}


		 /*******************************
		 *	 CLASS DECLARATION	*
		 *******************************/

/* Type declarations */

static char *T_compare[] =
        { "char_array", "ignore_case=[bool]" };
static char *T_ofAchar_fromADintD[] =
        { "of=char", "from=[int]" };
static char *T_gsub[] =
        { "start=int", "end=[int]" };
static char *T_cmpcase[] =
        { "text=char_array", "ignore_case=[bool]" };

/* Instance Variables */

static vardecl var_charArray[] =
{ IV(NAME_header, "alien:str_h", IV_NONE,
     NAME_internal, "Header info (packed)"),
  IV(NAME_text, "alien:wchar *", IV_NONE,
     NAME_internal, "Text represented (8- or 16-bits chars)")
};

/* Send Methods */

static senddecl send_charArray[] =
{ SM(NAME_initialise, 1, "text=char_array", initialiseCharArray,
     DEFAULT, "Create from other char_array"),
  SM(NAME_unlink, 0, NULL, unlinkCharArray,
     DEFAULT, "Free the char *"),
  SM(NAME_equal, 1, "char_array", equalCharArray,
     NAME_compare, "Test if names represent same text"),
  SM(NAME_larger, 1, "than=char_array", largerCharArray,
     NAME_compare, "Test if I'm alphabetically after arg"),
  SM(NAME_smaller, 1, "than=char_array", smallerCharArray,
     NAME_compare, "Test if I'm alphabetically before arg"),
  SM(NAME_prefix, 1, "prefix=char_array", prefixCharArray,
     NAME_test, "Test if receiver has prefix argument"),
  SM(NAME_sub, 2, T_cmpcase, subCharArray,
     NAME_test, "Test if argument is a substring"),
  SM(NAME_suffix, 2, T_cmpcase, suffixCharArray,
     NAME_test, "Test if receiver has suffix argument")
};

/* Get Methods */

static getdecl get_charArray[] =
{ GM(NAME_printName, 0, "char_array", NULL, getSelfObject,
     DEFAULT, "Equivalent to <-self"),
  GM(NAME_size, 0, "int", NULL, getSizeCharArray,
     NAME_cardinality, "Number of characters in the text"),
  GM(NAME_capitalise, 0, "char_array", NULL, getCapitaliseCharArray,
     NAME_case, "Capitalised version of name"),
  GM(NAME_downcase, 0, "char_array", NULL, getDowncaseCharArray,
     NAME_case, "Map all uppercase letters to lowercase"),
  GM(NAME_labelName, 0, "char_array", NULL, getLabelNameCharArray,
     NAME_case, "Default name used for labels"),
  GM(NAME_upcase, 0, "char_array", NULL, getUpcaseCharArray,
     NAME_case, "Map all lowercase letters to uppercase"),
  GM(NAME_compare, 2, "{smaller,equal,larger}", T_compare, getCompareCharArray,
     NAME_compare, "Alphabetical comparison"),
  GM(NAME_append, 1, "char_array", "char_array ...", getAppendCharArrayv,
     NAME_content, "Concatenation of me and the argument(s)"),
  GM(NAME_character, 1, "char", "int", getCharacterCharArray,
     NAME_content, "ASCII value of 0-based nth character"),
  GM(NAME_deletePrefix, 1, "char_array", "prefix=char_array", getDeletePrefixCharArray,
     NAME_content, "Delete specified prefix"),
  GM(NAME_deleteSuffix, 1, "char_array", "suffix=char_array", getDeleteSuffixCharArray,
     NAME_content, "Delete specified suffix"),
  GM(NAME_ensureSuffix, 1, "char_array", "suffix=char_array", getEnsureSuffixCharArray,
     NAME_content, "Append suffix if not already there"),
  GM(NAME_sub, 2, "char_array", T_gsub, getSubCharArray,
     NAME_content, "Get substring from 0-based start and end"),
  GM(NAME_convert, 1, "char_array", "any", getConvertCharArray,
     NAME_conversion, "Convert `text-convertible'"),
  GM(NAME_value, 0, "name", NULL, getValueCharArray,
     NAME_conversion, "Value as a name"),
  GM(NAME_copy, 0, "char_array", NULL, getCopyCharArray,
     NAME_copy, "Copy representing the same text"),
  GM(NAME_modify, 1, "char_array", "char_array", getModifyCharArray,
     NAME_internal, "New instance of my class"),
  GM(NAME_lineNo, 1, "line=int", "index=[int]", getLineNoCharArray,
     NAME_line, "Get 1-based line number at which index is"),
  GM(NAME_index, 2, "int", T_ofAchar_fromADintD, getIndexCharArray,
     NAME_parse, "Get 0-based index starting at pos (forwards)"),
  GM(NAME_rindex, 2, "int", T_ofAchar_fromADintD, getRindexCharArray,
     NAME_parse, "Get 0-based index starting at pos (backwards)"),
  GM(NAME_scan, 1, "vector", "format=char_array", getScanCharArray,
     NAME_parse, "C-scanf like parsing of string")
};

/* Resources */

#define rc_charArray NULL
/*
static resourcedecl rc_charArray[] =
{ 
};
*/

/* Class Declaration */

static Name charArray_termnames[] = { NAME_value };

ClassDecl(charArray_decls,
          var_charArray, send_charArray, get_charArray, rc_charArray,
          1, charArray_termnames,
          "$Rev$");

status
makeClassCharArray(Class class)
{ declareClass(class, &charArray_decls);

  setCloneFunctionClass(class, cloneCharArray);
  setLoadStoreFunctionClass(class, loadCharArray, storeCharArray);

  initCharArrays();

  succeed;
}
