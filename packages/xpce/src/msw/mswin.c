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

#include "include.h"
#include "mswin.h"
#include <h/interface.h>

		 /*******************************
		 *	    DLL STUFF		*
		 *******************************/

HINSTANCE ThePceHInstance;		/* Global handle */

BOOL WINAPI
DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{ switch(reason)
  { case DLL_PROCESS_ATTACH:
      ThePceHInstance = instance;
      break;
    case DLL_THREAD_ATTACH:
      break;
    case DLL_THREAD_DETACH:
      break;
  }

  return TRUE;
}


int
pceMTdetach()
{ DEBUG(NAME_thread,
	Cprintf("Detached thread 0x%x\n", GetCurrentThreadId()));
  destroyThreadWindows(ClassFrame);
  destroyThreadWindows(ClassWindow);

  return TRUE;
}


		 /*******************************
		 *	      VERSIONS		*
		 *******************************/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Get Windows Version/Revision info
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int
ws_version(void)
{ DWORD dwv = GetVersion();

  return LOBYTE(LOWORD(dwv));
}


int
ws_revision(void)
{ DWORD dwv = GetVersion();

  return HIBYTE(LOWORD(dwv));
}


int
iswin32s()
{ if( GetVersion() & 0x80000000 && (GetVersion() & 0xFF) == 3 )
    return TRUE;
  return FALSE;
}


os_platform
ws_platform(void)
{ static int done = FALSE;
  os_platform platform = WINUNKNOWN;

  if ( !done )
  { OSVERSIONINFO info;

    info.dwOSVersionInfoSize = sizeof(info);
    if ( GetVersionEx(&info) )
    { switch( info.dwPlatformId )
      { case VER_PLATFORM_WIN32s:
	  platform = WIN32S;
	  break;
	case VER_PLATFORM_WIN32_WINDOWS:
	  switch( info.dwMinorVersion )
	  { case 0:
	      platform = WIN95;
	      break;
	    case 10:
	      platform = WIN98;
	      break;
	    case 90:
	      platform = WINME;
	      break;
	    default:
	      platform = WINUNKNOWN;
	  }
	  break;
	case VER_PLATFORM_WIN32_NT:
	  platform = NT;
	  break;
      }
    } else
      platform = WINUNKNOWN;
  }

  return platform;
}

char *
ws_os(void)
{ switch(ws_platform())
  { case WINUNKNOWN:
      return "win32";
    case WIN32S:
      return "win32s";
    case WIN95:
    case WIN98:
    case WINME:
      return "win95";			/* doesn't really make a difference */
    case NT:
      return "winnt";
    default:
      return "winunknown";
  }
}


HWND
HostConsoleHWND()
{ PceCValue val;

  if ( hostQuery(HOST_CONSOLE, &val) )
    return (HWND) val.pointer;

  return NULL;
}


status
ws_show_console(Name how)
{ HWND hwnd = HostConsoleHWND();

  if ( hwnd )
  { if ( how == NAME_open )
    { if ( IsIconic(hwnd) )
	ShowWindow(hwnd, SW_RESTORE);
      else
	ShowWindow(hwnd, SW_SHOW);
    } else if ( how == NAME_iconic )
      ShowWindow(hwnd, SW_SHOWMINIMIZED);
    else if ( how == NAME_hidden )
      ShowWindow(hwnd, SW_HIDE);
    else if ( how == NAME_fullScreen )
      ShowWindow(hwnd, SW_MAXIMIZE);

    succeed;
  }

  fail;
}


status
ws_console_label(CharArray label)
{ HWND hwnd = HostConsoleHWND();

  if ( hwnd )
    SetWindowText(hwnd, strName(label));

  succeed;
}


void
ws_check_intr()
{ hostAction(HOST_CHECK_INTERRUPT);
}


void
ws_msleep(int time)
{ Sleep((DWORD) time);
}


int
ws_getpid()
{ DEBUG(NAME_instance, Cprintf("HINSTANCE is %d\n", PceHInstance));

  return (int) GetCurrentProcessId();
} 


char *
ws_user()
{ char buf[256];
  char *user;
  DWORD len = sizeof(buf);

  if ( GetUserName(buf, &len) )
    return strName(CtoName(buf));	/* force static storage */
  else if ( (user = getenv("USER")) )
    return user;
  else
    return NULL;
}


int
ws_mousebuttons()
{ return GetSystemMetrics(SM_CMOUSEBUTTONS);
}


#ifdef O_IMGLIB
void
remove_ilerrout(int status)
{ unlink("ilerr.out");
}
#endif


void
ws_initialise(int argc, char **argv)
{ if ( ws_mousebuttons() == 2 )
    ws_emulate_three_buttons(100);
}


Int
ws_default_scrollbar_width()
{ int w = GetSystemMetrics(SM_CXHSCROLL);	/* Is this the right one? */

  return toInt(w);
}


#define MAXMESSAGE 1024

Name
WinStrError(int error, ...)
{ va_list args;
  char msg[MAXMESSAGE];

  va_start(args, error);
  if ( !FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
		      NULL,
		      error,
		      GetUserDefaultLangID(),
		      msg,
		      sizeof(msg),
#ifdef __CYGWIN32__
		      args) )		/* Cygwin is according to docs ... */
#else
		      (char **)args) )
#endif
  { sprintf(msg, "Unknown WINAPI error %d", error);
  }
  va_end(args);

  return CtoName(msg);
}


int
get_logical_drive_strings(int bufsize, char *buf)
{ return GetLogicalDriveStrings(bufsize, buf);
}

		 /*******************************
		 *      COMMON DIALOG STUFF	*
		 *******************************/

#include <h/unix.h>
#ifndef _MAX_PATH
#define _MAX_PATH 1024
#endif
#ifndef MAXPATHLEN
#define MAXPATHLEN _MAX_PATH
#endif
#define strapp(s, q) \
	{ int l = strlen(q); \
	  if ( s+l+2 > filter+sizeof(filter) ) \
	  { errorPce(filters, NAME_representation, \
		     CtoString("filter too long")); \
	    fail; \
	  } \
	  strcpy(s, q); \
	  s += l; \
	}

static int
allLetter(const char *s)
{ for(; *s && isalpha(*s); s++)
    ;

  return *s ? FALSE : TRUE;
}


Name
getWinFileNameDisplay(DisplayObj d,
		      Name mode,	/* open, save */
		      Chain filters,	/* tuple(Name, Pattern) */
		      CharArray title,
		      CharArray file,	/* default file */
		      Directory dir,	/* initial dir */
		      Any owner)	/* owner window */
{ OPENFILENAME ofn;
  HWND hwnd;
  Name rval = 0;
  EventObj ev = EVENT->value;
  char filter[1024], *ef = filter;
  char buffer[2048];
  char cwdbin[MAXPATHLEN];
  BOOL tmpb;

  memset(&ofn, 0, sizeof(OPENFILENAME));
  ofn.lStructSize = sizeof(OPENFILENAME);

  if ( isInteger(owner) )
    ofn.hwndOwner = (void *)valInt(owner);
  else if ( instanceOfObject(owner, ClassFrame) )
    ofn.hwndOwner = getHwndFrame(owner);
  else if ( instanceOfObject(ev, ClassEvent) &&
	    (hwnd = getHwndWindow(ev->window)) )
    ofn.hwndOwner = hwnd;

  if ( isDefault(filters) )
  { Name nm = get((Any)NAME_allFiles, NAME_labelName, EAV);
    strapp(ef, strName(nm));
    *ef++ = '\0';
    strapp(ef, "*.*");
    *ef++ = '\0';
  } else
  { Cell cell;
  
    for_cell(cell, filters)
    { if ( instanceOfObject(cell->value, ClassTuple) )
      { Tuple t = cell->value;
	CharArray s1 = t->first, s2 = t->second;
  
	if ( !instanceOfObject(s1, ClassCharArray) )
	{ errorPce(s1, NAME_unexpectedType, TypeCharArray);
	  fail;
	}
	if ( !instanceOfObject(s2, ClassCharArray) )
	{ errorPce(s2, NAME_unexpectedType, TypeCharArray);
	  fail;
	}
	strapp(ef, strName(s1));
	*ef++ = '\0';
	strapp(ef, strName(s2));
	*ef++ = '\0';
      } else if ( instanceOfObject(cell->value, ClassCharArray) )
      { StringObj s = cell->value;
  
	strapp(ef, strName(s));
	*ef++ = '\0';
	strapp(ef, strName(s));
	*ef++ = '\0';
      } else
      { errorPce(cell->value, NAME_unexpectedType, CtoType("char_array|tuple"));
	fail;
      }
    }
  }
  *ef = '\0';
  ofn.lpstrFilter  = filter;
  ofn.nFilterIndex = 0;

  if ( isDefault(file) )
    buffer[0] = '\0';
  else
  { if ( strlen(strName(file)) >= sizeof(buffer) )
    { errorPce(file, NAME_representation, CtoString("name_too_long"));
      fail;
    }
    strcpy(buffer, strName(file));
  }

  ofn.lpstrFile    = buffer;
  ofn.nMaxFile     = sizeof(buffer)-1;
  if ( notDefault(dir) )
  { 
#ifdef O_XOS				/* should always be true */
    char tmp[MAXPATHLEN];
    char *s;
  
    if ( (s = expandFileName(strName(dir->path), tmp)) )
    { if ( !(_xos_os_filename(s, cwdbin, sizeof(cwdbin))) )
      { errorPce(file, NAME_representation, CtoString("name_too_long"));
	fail;
      }
      ofn.lpstrInitialDir = cwdbin;
    }
#else
    ofn.lpstrInitialDir = expandFileName(strName(dir->path), cwdbin);
#endif

/*  Cprintf("InitialDir = '%s'\n", ofn.lpstrInitialDir); */
  }
  if ( notDefault(title) )
  ofn.lpstrTitle = strName(title);

  ofn.Flags = (OFN_HIDEREADONLY|
	       OFN_NOCHANGEDIR);
	       
  if ( mode == NAME_open )
  { ofn.Flags |= OFN_FILEMUSTEXIST;
    tmpb = GetOpenFileName(&ofn);
  } else
    tmpb = GetSaveFileName(&ofn);

  if ( !tmpb )
  { DWORD w;

    if ( !(w=CommDlgExtendedError()) )
      fail;				/* user canceled */

    Cprintf("Get{Open,Save}FileName() failed: %ld\n", w);
    fail;
  }

  if ( buffer[0] )
  { char *base = baseName(buffer);

    if ( !strchr(base, '.') && ofn.nFilterIndex > 0 )
    { char *pattern = filter;
      char *ext;
      int n;
	
      pattern = filter;
      pattern += strlen(pattern)+1;	/* first pattern */
      for(n=1; n<ofn.nFilterIndex; n++)
      { pattern += strlen(pattern)+1;
	pattern += strlen(pattern)+1;
      }
  
      if ( (ext = strrchr(pattern, '.')) && allLetter(ext+1) )
	strcat(buffer, ext);
    }

#ifdef O_XOS				/* should always be true */
    if ( !_xos_canonical_filename(buffer, filter, sizeof(filter), 0) )
    { errorPce(CtoString(buffer), NAME_representation, CtoName("name_too_long"));
      fail;
    }
    rval = CtoName(filter);
#else
    rval = CtoName(buffer);
#endif
  }

  return rval;
}
