/*  $Id$

    Part of XPCE --- The SWI-Prolog GUI toolkit

    Author:        Jan Wielemaker and Anjo Anjewierden
    E-mail:        jan@swi.psy.uva.nl
    WWW:           http://www.swi.psy.uva.nl/projects/xpce/
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


#include <h/kernel.h>
#include <fcntl.h>
#include <h/interface.h>
#include <errno.h>

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Thread objects as SWI-Prolog streams. This module  is used by the Prolog
interface through pce_open/3. It should be merged with iostream.c, which
defines almost the same, providing XPCE with uniform access to a variety
of objects.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef struct pce_file_handle * PceFileHandle;

struct pce_file_handle
{ Any		object;			/* object `file-i-fied' */
  long		point;			/* current position */
  int		flags;			/* general flags field */
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Open flags recognised:

	PCE_RDONLY	Reading only
	PCE_WRONLY	Writing only
	PCE_RDWR	Reading and writing
	PCE_APPEND	Keep appending
	PCE_TRUNC	Tuncate object prior to writing
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static PceFileHandle *handles;		/* array of handles */
static int max_handles=0;		/* # handles allocated */

static int
allocFileHandle()
{ int handle;

  for(handle = 0; handle < max_handles; handle++)
  { if ( handles[handle] == NULL )
      return handle;
  }

  { PceFileHandle *newhandles;
    int n;

    if ( max_handles == 0 )
    { n = 16;
      newhandles = pceMalloc(sizeof(PceFileHandle) * n);
    } else
    { n = max_handles*2;
      newhandles = pceRealloc(handles, sizeof(PceFileHandle) * n);
    }

    if ( newhandles )
    { int rval = max_handles;

      memset(&newhandles[max_handles], 0,
	     sizeof(PceFileHandle) * (n-max_handles));
      max_handles = n;
      handles = newhandles;
      
      return rval;
    }
    
    errno = ENOMEM;
    return -1;
  }
}


int
pceOpen(Any obj, int flags)
{ int handle = allocFileHandle();
  PceFileHandle h;

  if ( handle < 0 )
    return handle;

  if ( !isProperObject(obj) )
  { errno = EINVAL;
    return -1;
  }

  if ( flags & PCE_WRONLY )
  { if ( !hasSendMethodObject(obj, NAME_writeAsFile) )
    { errno = EACCES;
      return -1;
    }

    if ( flags & PCE_TRUNC )
    { if ( !hasSendMethodObject(obj, NAME_truncateAsFile) ||
	   !send(obj, NAME_truncateAsFile, EAV) )
      { errno = EACCES;
	return -1;
      }
    }
  }
  if ( flags & PCE_RDONLY )
  { if ( !hasGetMethodObject(obj, NAME_readAsFile) )
    { errno = EACCES;
      return -1;
    }
  }

  h = alloc(sizeof(struct pce_file_handle));
  h->object = obj;
  addRefObj(obj);			/* so existence check is safe */
  h->flags = flags;
  h->point = 0L;
  handles[handle] = h;

  return handle;
}


int
pceClose(int handle)
{ PceFileHandle h;

  if ( handle >= 0 && handle < max_handles &&
       (h = handles[handle]) )
  { delRefObject(NIL, h->object);	/* handles deferred unalloc() */
    unalloc(sizeof(struct pce_file_handle), h);
    handles[handle] = NULL;

    return 0;
  }

  errno = EBADF;
  return -1;
}


int
pceWrite(int handle, const char *buf, int size)
{ PceFileHandle h;

  if ( handle >= 0 && handle < max_handles &&
       (h = handles[handle]) &&
       h->flags & (PCE_RDWR|PCE_WRONLY) )
  { string s;
    CharArray ca;
    status rval;
    Int where = (h->flags & PCE_APPEND ? (Int) DEFAULT : toInt(h->point));
    const wchar_t *wbuf = (const wchar_t*)buf;
    const wchar_t *end = (const wchar_t*)&buf[size];
    const wchar_t *f;

    if ( isFreedObj(h->object) )
    { errno = EIO;
      return -1;
    }

    assert(size%sizeof(wchar_t) == 0);
    
    for(f=wbuf; f<end; f++)
    { if ( *f > 0xff )
	break;
    }
  
    if ( f == end )
    { charA *asc = alloca(size);
      charA *t = asc;
  
      for(f=wbuf; f<end; )
	*t++ = (charA)*f++;
  
      str_set_n_ascii(&s, size/sizeof(wchar_t), asc);
    } else
    { str_set_n_wchar(&s, size/sizeof(wchar_t), (wchar_t*)wbuf);
    }

    ca = StringToScratchCharArray(&s);

    if ( (rval = send(h->object, NAME_writeAsFile, where, ca, EAV)) )
      h->point += size/sizeof(wchar_t);
    doneScratchCharArray(ca);

    if ( rval )
      return size;

    errno = EIO;
    return -1;
  } else
  { errno = EBADF;
    return -1;
  }
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Note: pos is measured  in  bytes.  If   we  use  wchar  encoding we must
compensate for this.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

long
pceSeek(int handle, long offset, int whence)
{ PceFileHandle h;

  offset /= sizeof(wchar_t);

  if ( handle >= 0 && handle < max_handles && (h = handles[handle]) )
  { Int size;

    if ( isFreedObj(h->object) )
    { errno = EIO;
      return -1;
    }

    switch(whence)
    { case PCE_SEEK_SET:
	h->point = offset;
        break;
      case PCE_SEEK_CUR:
        h->point += offset;
        break;
      case PCE_SEEK_END:
      { if ( hasGetMethodObject(h->object, NAME_sizeAsFile) &&
	     (size = get(h->object, NAME_sizeAsFile, EAV)) )
	{ h->point = valInt(size) - offset;
	  break;
	} else
	{ errno = EPIPE;		/* better idea? */
	  return -1;
	}
      }
      default:
      { errno = EINVAL;
	return -1;
      }
    }
    return h->point * sizeof(wchar_t);
  } else
  { errno = EBADF;
    return -1;
  }
}


/* see also Sread_object() */

int
pceRead(int handle, char *buf, int size)
{ PceFileHandle h;

  if ( handle >= 0 && handle < max_handles &&
       (h = handles[handle]) &&
       h->flags & (PCE_RDWR|PCE_RDONLY) )
  { Any argv[2];
    CharArray sub;
    int chread;

    if ( isFreedObj(h->object) )
    { errno = EIO;
      return -1;
    }

    argv[0] = toInt(h->point);
    argv[1] = toInt(size/sizeof(wchar_t));
  
    if ( (sub = getv(h->object, NAME_readAsFile, 2, argv)) &&
	 instanceOfObject(sub, ClassCharArray) )
    { String s = &sub->data;
  
      assert(s->size <= size/sizeof(wchar_t));
  
      if ( isstrA(s) )
      { charW *dest = (charW*)buf;
	const charA *f = s->s_textA;
	const charA *e = &f[s->size];
	
	while(f<e)
	  *dest++ = *f++;
      } else
      { memcpy(buf, s->s_textW, s->size*sizeof(charW));
      }
  
      chread = s->size * sizeof(wchar_t);
      h->point += s->size;
    } else
    { errno = EIO;
      chread = -1;
    }

    return chread;
  } else
  { errno = EBADF;
    return -1;
  }
}


const char *
pceOsError()
{ return strName(getOsErrorPce(PCE));
}
