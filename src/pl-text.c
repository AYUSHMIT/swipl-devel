/*  $Id$

    Part of SWI-Prolog

    Author:        Jan Wielemaker and Anjo Anjewierden
    E-mail:        jan@swi.psy.uva.nl
    WWW:           http://www.swi-prolog.org
    Copyright (C): 1985-2002, University of Amsterdam

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "pl-incl.h"
#include "pl-ctype.h"
#include "pl-utf8.h"
#include <stdio.h>

#undef LD
#define LD LOCAL_LD

static inline word
valHandle__LD(term_t r ARG_LD)
{ Word p = valTermRef(r);

  deRef(p);
  return *p;
}

#define valHandle(r) valHandle__LD(r PASS_LD)
#define setHandle(h, w)		(*valTermRef(h) = (w))


		 /*******************************
		 *	UNIFIED TEXT STUFF	*
		 *******************************/

static inline int
bufsize_text(PL_chars_t *text, unsigned int len)
{ int unit;

  switch(text->encoding)
  { case ENC_ISO_LATIN_1:
    case ENC_ASCII:
    case ENC_UTF8:
      unit = sizeof(char);
      break;
    case ENC_WCHAR:
      unit = sizeof(pl_wchar_t);
      break;
    default:
      assert(0);
  }

  return len*unit;
}


void
PL_save_text(PL_chars_t *text, int flags)
{ if ( (flags & BUF_MALLOC) && text->storage != PL_CHARS_MALLOC )
  { int bl = bufsize_text(text, text->length+1);
    void *new = PL_malloc(bl);

    memcpy(new, text->text.t, bl);
    text->text.t = new;
    text->storage = PL_CHARS_MALLOC;
  } else if ( text->storage == PL_CHARS_LOCAL )
  { Buffer b = findBuffer(BUF_RING);
    int bl = bufsize_text(text, text->length+1);

    addMultipleBuffer(b, text->text.t, bl, char);
    text->text.t = baseBuffer(b, char);
    
    text->storage = PL_CHARS_RING;
  }
}


int
PL_get_text(term_t l, PL_chars_t *text, int flags)
{ GET_LD
  word w = valHandle(l);

  if ( (flags & CVT_ATOM) && isAtom(w) )
  { if ( !get_atom_text(w, text) )
      fail;
  } else if ( (flags & CVT_STRING) && isString(w) )
  { if ( !get_string_text(w, text PASS_LD) )
      fail;
  } else if ( (flags & CVT_INTEGER) && isInteger(w) )
  { sprintf(text->buf, INT64_FORMAT, valInteger(w) );
    text->text.t    = text->buf;
    text->length    = strlen(text->text.t);
    text->encoding  = ENC_ISO_LATIN_1;
    text->storage   = PL_CHARS_LOCAL;
    text->canonical = TRUE;
  } else if ( (flags & CVT_FLOAT) && isReal(w) )
  { char *q;

    Ssprintf(text->buf, LD->float_format, valReal(w) );
    text->text.t = text->buf;

    q = text->buf;			/* See also writePrimitive() */
    if ( *q == L'-' )
      q++;
    for(; *q; q++)
    { if ( !isDigit(*q) )
	break;
    }
    if ( !*q )
    { *q++ = '.';
      *q++ = '0';
      *q = EOS;
    }
    text->length    = strlen(text->text.t);
    text->encoding  = ENC_ISO_LATIN_1;
    text->storage   = PL_CHARS_LOCAL;
    text->canonical = TRUE;
  } else if ( (flags & CVT_LIST) &&
	      (isList(w) || isNil(w)) )
  { Buffer b;

    if ( (b = codes_or_chars_to_buffer(l, BUF_RING, FALSE)) )
    { text->length = entriesBuffer(b, char);
      addBuffer(b, EOS, char);
      text->text.t = baseBuffer(b, char);
      text->encoding = ENC_ISO_LATIN_1;
    } else if ( (b = codes_or_chars_to_buffer(l, BUF_RING, TRUE)) )
    { text->length = entriesBuffer(b, pl_wchar_t);
      addBuffer(b, EOS, pl_wchar_t);
      text->text.w = baseBuffer(b, pl_wchar_t);
      text->encoding = ENC_WCHAR;
    } else
      fail;

    text->storage   = PL_CHARS_RING;
    text->canonical = TRUE;
  } else if ( (flags & CVT_VARIABLE) && isVar(w) )
  { text->text.t   = varName(l, text->buf);
    text->length   = strlen(text->text.t);
    text->encoding = ENC_ISO_LATIN_1;
    text->storage  = PL_CHARS_LOCAL;
    text->canonical = TRUE;
  } else if ( (flags & CVT_WRITE) )
  { IOENC encodings[] = { ENC_ISO_LATIN_1, ENC_WCHAR, ENC_UNKNOWN };
    IOENC *enc;
    char *r;

    for(enc = encodings; *enc != ENC_UNKNOWN; enc++)
    { int size;
      IOSTREAM *fd;
    
      r = text->buf;
      size = sizeof(text->buf);
      fd = Sopenmem(&r, &size, "w");
      fd->encoding = *enc;
      if ( PL_write_term(fd, l, 1200, 0) &&
	   Sputcode(EOS, fd) >= 0 &&
	   Sflush(fd) >= 0 )
      { text->encoding = *enc;
	text->storage = (r == text->buf ? PL_CHARS_LOCAL : PL_CHARS_MALLOC);
	text->canonical = TRUE;

	if ( *enc == ENC_ISO_LATIN_1 )
	{ text->length = size-1;
	  text->text.t = r;
	} else
	{ text->length = (size/sizeof(pl_wchar_t))-1;
	  text->text.w = (pl_wchar_t *)r;
	}

	Sclose(fd);

	return TRUE;
      } else
      { Sclose(fd);
	if ( r != text->buf )
	  Sfree(r);
      }
    }

    fail;
  } else
  { fail;
  }

  succeed;
}


atom_t
textToAtom(PL_chars_t *text)
{ PL_canonise_text(text);

  if ( text->encoding == ENC_ISO_LATIN_1 )
  { return lookupAtom(text->text.t, text->length);
  } else
  { return lookupUCSAtom(text->text.w, text->length);
  }
}


word
textToString(PL_chars_t *text)
{ PL_canonise_text(text);

  if ( text->encoding == ENC_ISO_LATIN_1 )
  { return globalString(text->length, text->text.t);
  } else
  { return globalWString(text->length, text->text.w);
  }
}


int
PL_unify_text(term_t term, PL_chars_t *text, int type)
{ switch(type)
  { case PL_ATOM:
    { atom_t a = textToAtom(text);
      int rval = _PL_unify_atomic(term, a);
      
      PL_unregister_atom(a);
      return rval;
    }
    case PL_STRING:
    { word w = textToString(text);

      return _PL_unify_atomic(term, w);
    }
    case PL_CODE_LIST:
    case PL_CHAR_LIST:
    { if ( text->length == 0 )
      { return PL_unify_nil(term);
      } else
      { GET_LD
	term_t l = PL_new_term_ref();
	Word p0, p;
      
	switch(text->encoding)
	{ case ENC_ISO_LATIN_1:
	  { const unsigned char *s = (const unsigned char *)text->text.t;
	    const unsigned char *e = &s[text->length];

	    p0 = p = allocGlobal(text->length*3);
	    for( ; s < e; s++)
	    { *p++ = FUNCTOR_dot2;
	      if ( type == PL_CODE_LIST )
		*p++ = consInt(*s);
	      else
		*p++ = codeToAtom(*s);
	      *p = consPtr(p+1, TAG_COMPOUND|STG_GLOBAL);
	      p++;
	    }
	    break;
	  }
	  case ENC_WCHAR:
	  { const pl_wchar_t *s = (const pl_wchar_t *)text->text.t;
	    const pl_wchar_t *e = &s[text->length];
  
	    p0 = p = allocGlobal(text->length*3);
	    for( ; s < e; s++)
	    { *p++ = FUNCTOR_dot2;
	      if ( type == PL_CODE_LIST )
		*p++ = consInt(*s);
	      else
		*p++ = codeToAtom(*s);
	      *p = consPtr(p+1, TAG_COMPOUND|STG_GLOBAL);
	      p++;
	    }
	    break;
	  }
	  case ENC_UTF8:
	  { const char *s = text->text.t;
	    const char *e = &s[text->length];
	    unsigned int len = utf8_strlen(s, text->length);

	    p0 = p = allocGlobal(len*3);
	    while(s<e)
	    { int chr;

	      s = utf8_get_char(s, &chr);
	      *p++ = FUNCTOR_dot2;
	      if ( type == PL_CODE_LIST )
		*p++ = consInt(chr);
	      else
		*p++ = codeToAtom(chr);
	      *p = consPtr(p+1, TAG_COMPOUND|STG_GLOBAL);
	      p++;
	    }
	    break;
	  }
	  default:
	  { assert(0);

	    return FALSE;
	  }
	}
	p[-1] = ATOM_nil;

	setHandle(l, consPtr(p0, TAG_COMPOUND|STG_GLOBAL));

	return PL_unify(l, term);
      }
    }
    default:
    { assert(0);

      return FALSE;
    }
  }
}


int
PL_unify_text_range(term_t term, PL_chars_t *text,
		    unsigned offset, unsigned len, int type)
{ if ( offset == 0 && len == text->length )
  { return PL_unify_text(term, text, type);
  } else
  { PL_chars_t sub;
    int rc;

    if ( offset > text->length || offset + len > text->length )
      return FALSE;

    sub.length = len;
    sub.storage = PL_CHARS_HEAP;
    if ( text->encoding == ENC_ISO_LATIN_1 )
    { sub.text.t   = text->text.t+offset;
      sub.encoding = ENC_ISO_LATIN_1;
      sub.canonical = TRUE;
    } else
    { sub.text.w   = text->text.w+offset;
      sub.encoding = ENC_WCHAR;
      sub.canonical = FALSE;
    }

    rc = PL_unify_text(term, &sub, type);

    PL_free_text(&sub);
    
    return rc;
  }
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int PL_promote_text(PL_chars_t *text)

Promote a text to USC if it is currently 8-bit text.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int
PL_promote_text(PL_chars_t *text)
{ if ( text->encoding != ENC_WCHAR )
  { if ( text->storage == PL_CHARS_MALLOC )
    { pl_wchar_t *new = PL_malloc(sizeof(pl_wchar_t)*(text->length+1));
      pl_wchar_t *t = new;
      const unsigned char *s = (const unsigned char *)text->text.t;
      const unsigned char *e = &s[text->length];

      while(s<e)
      { *t++ = *s++;
      }
      *t = EOS;

      PL_free(text->text.t);
      text->text.w = new;
      
      text->encoding = ENC_WCHAR;
    } else if ( text->storage == PL_CHARS_LOCAL &&
	        (text->length+1)*sizeof(pl_wchar_t) < sizeof(text->buf) )
    { unsigned char buf[sizeof(text->buf)];
      unsigned char *f = buf;
      unsigned char *e = &buf[text->length];
      pl_wchar_t *t = (pl_wchar_t*)text->buf;

      memcpy(buf, text->buf, text->length*sizeof(char));
      while(f<e)
      { *t++ = *f++;
      }
      *t = EOS;
      text->encoding = ENC_WCHAR;
    } else
    { Buffer b = findBuffer(BUF_RING);
      const unsigned char *s = (const unsigned char *)text->text.t;
      const unsigned char *e = &s[text->length];

      for( ; s<e; s++)
	addBuffer(b, *s, pl_wchar_t);
      addBuffer(b, EOS, pl_wchar_t);

      text->text.w   = baseBuffer(b, pl_wchar_t);
      text->encoding = ENC_WCHAR;
      text->storage  = PL_CHARS_RING;
    }
  }

  succeed;
}


int
PL_demote_text(PL_chars_t *text)
{ if ( text->encoding != ENC_ISO_LATIN_1 )
  { if ( text->storage == PL_CHARS_MALLOC )
    { char *new = PL_malloc(sizeof(char)*(text->length+1));
      char *t = new;
      const pl_wchar_t *s = (const pl_wchar_t *)text->text.t;
      const pl_wchar_t *e = &s[text->length];

      while(s<e)
      { if ( *s > 0xff )
	{ PL_free(new);
	  return FALSE;
	}
	*t++ = *s++ & 0xff;
      }
      *t = EOS;

      PL_free(text->text.t);
      text->text.t = new;
      
      text->encoding = ENC_ISO_LATIN_1;
    } else if ( text->storage == PL_CHARS_LOCAL )
    { pl_wchar_t buf[sizeof(text->buf)/sizeof(pl_wchar_t)];
      pl_wchar_t *f = buf;
      pl_wchar_t *e = &buf[text->length];
      char *t = text->buf;

      memcpy(buf, text->buf, text->length*sizeof(pl_wchar_t));
      while(f<e)
      { if ( *f > 0xff )
	  return FALSE;
	*t++ = *f++ & 0xff;
      }
      *t = EOS;
      text->encoding = ENC_ISO_LATIN_1;
    } else
    { Buffer b = findBuffer(BUF_RING);
      const pl_wchar_t *s = (const pl_wchar_t*)text->text.w;
      const pl_wchar_t *e = &s[text->length];

      for( ; s<e; s++)
      { if ( *s > 0xff )
	{ unfindBuffer(BUF_RING);
	  return FALSE;
	}
	addBuffer(b, *s&0xff, char);
      }
      addBuffer(b, EOS, char);

      text->text.t   = baseBuffer(b, char);
      text->storage  = PL_CHARS_RING;
      text->encoding = ENC_ISO_LATIN_1;
    }
  }

  succeed;
}


static int
can_demote(PL_chars_t *text)
{ if ( text->encoding != ENC_ISO_LATIN_1 )
  { const pl_wchar_t *w = (const pl_wchar_t*)text->text.w;
    const pl_wchar_t *e = &w[text->length];

    for(; w<e; w++)
    { if ( *w > 0xff )
	return FALSE;
    }
  }

  return TRUE;
}


int
PL_canonise_text(PL_chars_t *text)
{ if ( !text->canonical )
  { switch(text->encoding )
    { case ENC_ISO_LATIN_1:
	break;				/* nothing to do */
      case ENC_WCHAR:
      { const pl_wchar_t *w = (const pl_wchar_t*)text->text.w;
	const pl_wchar_t *e = &w[text->length];
      
	for(; w<e; w++)
	{ if ( *w > 0xff )
	    return FALSE;
	}

	return PL_demote_text(text);
      }
      case ENC_UTF8:
      { const char *s = text->text.t;
	const char *e = &s[text->length];

	while(s<e && !(*s & 0x80))
	  s++;
	if ( s == e )
	{ text->encoding  = ENC_ISO_LATIN_1;
	  text->canonical = TRUE;

	  succeed;
	} else
	{ int chr;
	  int wide = FALSE;
	  int len = s - text->text.t;

	  while(s<e)
	  { s = utf8_get_char(s, &chr);
	    if ( chr > 0xff )		/* requires wide characters */
	      wide = TRUE;
	    len++;
	  }

	  s = (const unsigned char *)text->text.t;
	  text->length = len;

	  if ( wide )
	  { pl_wchar_t *to = PL_malloc(sizeof(pl_wchar_t)*(len+1));

	    text->text.w = to;
	    while(s<e)
	    { s = utf8_get_char(s, &chr);
	      *to++ = chr;
	    }
	    *to = EOS;

	    text->encoding = ENC_WCHAR;
	    text->storage  = PL_CHARS_MALLOC;
	  } else
	  { char *to = PL_malloc(len+1);

	    text->text.t = to;
	    while(s<e)
	    { s = utf8_get_char(s, &chr);
	      *to++ = chr;
	    }
	    *to = EOS;

	    text->encoding = ENC_ISO_LATIN_1;
	    text->storage  = PL_CHARS_MALLOC;
	  }

	  text->canonical = TRUE;
	  succeed;
	}
      }
      default:
	assert(0);
    }
  }

  succeed;
}


void
PL_free_text(PL_chars_t *text)
{ if ( text->storage == PL_CHARS_MALLOC )
    PL_free(text->text.t);
}


void
PL_text_recode(PL_chars_t *text, IOENC encoding)
{ if ( text->encoding != encoding )
  { switch(encoding)
    { case ENC_UTF8:
      { switch(text->encoding)
	{ case ENC_ASCII:
	    text->encoding = ENC_UTF8;
	    break;
	  case ENC_ISO_LATIN_1:
	  { Buffer b = findBuffer(BUF_RING);
	    const unsigned char *s = (const unsigned char *)text->text.t;
	    const unsigned char *e = &s[text->length];
	    char tmp[8];

	    for( ; s<e; s++)
	    { if ( *s&0x80 )
	      { const char *end = utf8_put_char(tmp, *s);
		const char *q = tmp;

		for(q=tmp; q<end; q++)
		  addBuffer(b, *q, char);
	      } else
	      { addBuffer(b, *s, char);
	      }
	    }
	    PL_free_text(text);
            text->length   = entriesBuffer(b, char);
	    addBuffer(b, EOS, char);
	    text->text.t   = baseBuffer(b, char);
	    text->encoding = ENC_UTF8;
	    text->storage  = PL_CHARS_RING;

	    break;
	  }
	  case ENC_WCHAR:
	  { Buffer b = findBuffer(BUF_RING);
	    const pl_wchar_t *s = text->text.w;
	    const pl_wchar_t *e = &s[text->length];
	    char tmp[8];

	    for( ; s<e; s++)
	    { if ( *s > 0x7f )
	      { const char *end = utf8_put_char(tmp, (int)*s);
		const char *q = tmp;

		for(q=tmp; q<end; q++)
		  addBuffer(b, *q&0xff, char);
	      } else
	      { addBuffer(b, *s&0xff, char);
	      }
	    }
	    PL_free_text(text);
            text->length   = entriesBuffer(b, char);
	    addBuffer(b, EOS, char);
	    text->text.t   = baseBuffer(b, char);
	    text->encoding = ENC_UTF8;
	    text->storage  = PL_CHARS_RING;

	    break;
	  }
	  default:
	    assert(0);
	}
	break;
	default:
	  assert(0);
      }
    }
  }
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PL_cmp_text(PL_chars_t *t1, unsigned o1,
	    PL_chars_t *t2, unsigned o2,
	    unsigned len)

Compares two substrings of two text representations.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int
PL_cmp_text(PL_chars_t *t1, unsigned o1, PL_chars_t *t2, unsigned o2,
	    unsigned len)
{ int l = len;
  int ifeq = 0;

  if ( l > (int)(t1->length - o1) )
  { l = t1->length - o1;
    ifeq = -1;				/* first is short */
  }
  if ( l > (int)(t2->length - o2) )
  { l = t2->length - o2;
    if ( ifeq == 0 )
      ifeq = 1;
  }

  if ( l < 0 )				/* too long offsets */
    return ifeq;

  if ( t1->encoding == ENC_ISO_LATIN_1 && t2->encoding == ENC_ISO_LATIN_1 )
  { const unsigned char *s = t1->text.t+o1;
    const unsigned char *q = t2->text.t+o2;

    for(; l-- > 0 && *s == *q; s++, q++ )
      ;
    if ( l < 0 )
      return ifeq;
    else
      return *s > *q ? 1 : -1;
  } else if ( t1->encoding == ENC_WCHAR && t2->encoding == ENC_WCHAR )
  { const pl_wchar_t *s = t1->text.w+o1;
    const pl_wchar_t *q = t2->text.w+o2;

    for(; l-- > 0 && *s == *q; s++, q++ )
      ;
    if ( l < 0 )
      return ifeq;
    else
      return *s > *q ? 1 : -1;
  } else if ( t1->encoding == ENC_ISO_LATIN_1 && t2->encoding == ENC_WCHAR )
  { const unsigned char *s = t1->text.t+o1;
    const pl_wchar_t *q = t2->text.w+o2;

    for(; l-- > 0 && *s == *q; s++, q++ )
      ;
    if ( l < 0 )
      return ifeq;
    else
      return *s > *q ? 1 : -1;
  } else
  { const pl_wchar_t *s = t1->text.w+o1;
    const unsigned char *q = t2->text.t+o2;

    for(; l-- > 0 && *s == *q; s++, q++ )
      ;
    if ( l < 0 )
      return ifeq;
    else
      return *s > *q ? 1 : -1;
  }  
}


int
PL_concat_text(int n, PL_chars_t **text, PL_chars_t *result)
{ int total_length = 0;
  int latin = TRUE;
  int i;

  for(i=0; i<n; i++)
  { if ( latin && !can_demote(text[i]) )
      latin = FALSE;
    total_length += text[i]->length;
  }

  result->canonical = TRUE;
  result->length = total_length;

  if ( latin )
  { char *to;

    result->encoding = ENC_ISO_LATIN_1;
    if ( total_length+1 < sizeof(result->buf) )
    { result->text.t = result->buf;
      result->storage = PL_CHARS_LOCAL;
    } else
    { result->text.t = PL_malloc(total_length+1);
      result->storage = PL_CHARS_MALLOC;
    }

    for(to=result->text.t, i=0; i<n; i++)
    { memcpy(to, text[i]->text.t, text[i]->length);
      to += text[i]->length;
    }
    *to = EOS;
  } else
  { pl_wchar_t *to;

    result->encoding = ENC_WCHAR;
    if ( total_length+1 < sizeof(result->buf)/sizeof(pl_wchar_t) )
    { result->text.w = (pl_wchar_t*)result->buf;
      result->storage = PL_CHARS_LOCAL;
    } else
    { result->text.w = PL_malloc((total_length+1)*sizeof(pl_wchar_t));
      result->storage = PL_CHARS_MALLOC;
    }

    for(to=result->text.w, i=0; i<n; i++)
    { if ( text[i]->encoding == ENC_WCHAR )
      { memcpy(to, text[i]->text.w, text[i]->length*sizeof(pl_wchar_t));
	to += text[i]->length;
      } else
      { const unsigned char *f = text[i]->text.t;
	const unsigned char *e = &f[text[i]->length];

	while(f<e)
	  *to++ = *f++;
      }
    }
    assert(to-result->text.w == total_length);
    *to = EOS;
  }

  return TRUE;
}


IOSTREAM *
Sopen_text(PL_chars_t *txt, const char *mode)
{ IOSTREAM *stream;

  stream = Sopen_string(NULL,
			txt->text.t,
			bufsize_text(txt, txt->length),
			"r");
  stream->encoding = txt->encoding;

  return stream;
}
