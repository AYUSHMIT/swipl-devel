# Microsoft Visual C++ Generated NMAKE File, Format Version 2.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Win32 Release" && "$(CFG)" != "Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "libpl.mak" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
# PROP Target_Last_Scanned "Win32 Release"
MTL=MkTypLib.exe
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "c:\jan\lib"
# PROP Intermediate_Dir "c:\jan\objects\libpl"
OUTDIR=c:\jan\lib
INTDIR=c:\jan\objects\libpl

ALL : $(OUTDIR)/libpl.dll $(OUTDIR)/libpl.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

$(INTDIR) : 
    if not exist $(INTDIR)/nul mkdir $(INTDIR)

# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE CPP /nologo /MT /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /MT /W3 /GX /YX /O2 /I "c:\jan\pl\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__WIN32__" /D "MAKE_PL_DLL" /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MT /W3 /GX /YX /O2 /I "c:\jan\pl\include" /D "NDEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "__WIN32__" /D "MAKE_PL_DLL" /Fp$(OUTDIR)/"libpl.pch"\
 /Fo$(INTDIR)/ /c 
CPP_OBJS=c:\jan\objects\libpl/
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
BSC32_SBRS= \
	
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"libpl.bsc" 

$(OUTDIR)/libpl.bsc : $(OUTDIR)  $(BSC32_SBRS)
LINK32=link.exe
DEF_FILE=
LINK32_OBJS= \
	$(INTDIR)/"pl-dwim.obj" \
	$(INTDIR)/"pl-util.obj" \
	$(INTDIR)/"pl-rec.obj" \
	$(INTDIR)/"pl-comp.obj" \
	$(INTDIR)/"pl-write.obj" \
	$(INTDIR)/"pl-table.obj" \
	$(INTDIR)/"pl-pro.obj" \
	$(INTDIR)/"pl-itf.obj" \
	$(INTDIR)/"pl-term.obj" \
	$(INTDIR)/"pl-file.obj" \
	$(INTDIR)/"pl-prims.obj" \
	$(INTDIR)/"pl-dde.obj" \
	$(INTDIR)/"pl-stream.obj" \
	$(INTDIR)/"pl-bag.obj" \
	$(INTDIR)/"pl-flag.obj" \
	$(INTDIR)/"pl-nt.obj" \
	$(INTDIR)/"pl-ext.obj" \
	$(INTDIR)/"pl-prof.obj" \
	$(INTDIR)/"pl-wam.obj" \
	$(INTDIR)/"pl-sys.obj" \
	$(INTDIR)/"pl-main.obj" \
	$(INTDIR)/"pl-load.obj" \
	$(INTDIR)/"pl-os.obj" \
	$(INTDIR)/"pl-read.obj" \
	$(INTDIR)/"pl-glob.obj" \
	$(INTDIR)/"pl-modul.obj" \
	$(INTDIR)/"pl-trace.obj" \
	$(INTDIR)/"pl-store.obj" \
	$(INTDIR)/"pl-setup.obj" \
	$(INTDIR)/"pl-save.obj" \
	$(INTDIR)/"pl-proc.obj" \
	$(INTDIR)/"pl-dll.obj" \
	$(INTDIR)/"pl-dump.obj" \
	$(INTDIR)/"pl-op.obj" \
	$(INTDIR)/"pl-funct.obj" \
	$(INTDIR)/"pl-list.obj" \
	$(INTDIR)/"pl-fmt.obj" \
	$(INTDIR)/"pl-arith.obj" \
	$(INTDIR)/"pl-fli.obj" \
	$(INTDIR)/"pl-wic.obj" \
	$(INTDIR)/"pl-buffer.obj" \
	$(INTDIR)/"pl-gc.obj" \
	$(INTDIR)/"pl-atom.obj"
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:windows /DLL /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib c:\jan\pl\bin\uxnt.lib /NOLOGO /SUBSYSTEM:windows /DLL /MACHINE:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib c:\jan\pl\bin\uxnt.lib /NOLOGO /SUBSYSTEM:windows /DLL\
 /INCREMENTAL:no /PDB:$(OUTDIR)/"libpl.pdb" /MACHINE:I386\
 /OUT:$(OUTDIR)/"libpl.dll" /IMPLIB:$(OUTDIR)/"libpl.lib" 

$(OUTDIR)/libpl.dll : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "c:\jan\lib"
# PROP Intermediate_Dir "c:\jan\objects\libpl"
OUTDIR=c:\jan\lib
INTDIR=c:\jan\objects\libpl

ALL : $(OUTDIR)/libpl.dll $(OUTDIR)/libpl.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

$(INTDIR) : 
    if not exist $(INTDIR)/nul mkdir $(INTDIR)

# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /MT /W3 /GX /Zi /YX /Od /I "c:\jan\pl\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__WIN32__" /D "MAKE_PL_DLL" /c
# SUBTRACT CPP /Fr
CPP_PROJ=/nologo /MT /W3 /GX /Zi /YX /Od /I "c:\jan\pl\include" /D "_DEBUG" /D\
 "WIN32" /D "_WINDOWS" /D "__WIN32__" /D "MAKE_PL_DLL" /Fp$(OUTDIR)/"libpl.pch"\
 /Fo$(INTDIR)/ /Fd$(OUTDIR)/"libpl.pdb" /c 
CPP_OBJS=c:\jan\objects\libpl/
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
BSC32_SBRS= \
	
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"libpl.bsc" 

$(OUTDIR)/libpl.bsc : $(OUTDIR)  $(BSC32_SBRS)
LINK32=link.exe
DEF_FILE=
LINK32_OBJS= \
	$(INTDIR)/"pl-dwim.obj" \
	$(INTDIR)/"pl-util.obj" \
	$(INTDIR)/"pl-rec.obj" \
	$(INTDIR)/"pl-comp.obj" \
	$(INTDIR)/"pl-write.obj" \
	$(INTDIR)/"pl-table.obj" \
	$(INTDIR)/"pl-pro.obj" \
	$(INTDIR)/"pl-itf.obj" \
	$(INTDIR)/"pl-term.obj" \
	$(INTDIR)/"pl-file.obj" \
	$(INTDIR)/"pl-prims.obj" \
	$(INTDIR)/"pl-dde.obj" \
	$(INTDIR)/"pl-stream.obj" \
	$(INTDIR)/"pl-bag.obj" \
	$(INTDIR)/"pl-flag.obj" \
	$(INTDIR)/"pl-nt.obj" \
	$(INTDIR)/"pl-ext.obj" \
	$(INTDIR)/"pl-prof.obj" \
	$(INTDIR)/"pl-wam.obj" \
	$(INTDIR)/"pl-sys.obj" \
	$(INTDIR)/"pl-main.obj" \
	$(INTDIR)/"pl-load.obj" \
	$(INTDIR)/"pl-os.obj" \
	$(INTDIR)/"pl-read.obj" \
	$(INTDIR)/"pl-glob.obj" \
	$(INTDIR)/"pl-modul.obj" \
	$(INTDIR)/"pl-trace.obj" \
	$(INTDIR)/"pl-store.obj" \
	$(INTDIR)/"pl-setup.obj" \
	$(INTDIR)/"pl-save.obj" \
	$(INTDIR)/"pl-proc.obj" \
	$(INTDIR)/"pl-dll.obj" \
	$(INTDIR)/"pl-dump.obj" \
	$(INTDIR)/"pl-op.obj" \
	$(INTDIR)/"pl-funct.obj" \
	$(INTDIR)/"pl-list.obj" \
	$(INTDIR)/"pl-fmt.obj" \
	$(INTDIR)/"pl-arith.obj" \
	$(INTDIR)/"pl-fli.obj" \
	$(INTDIR)/"pl-wic.obj" \
	$(INTDIR)/"pl-buffer.obj" \
	$(INTDIR)/"pl-gc.obj" \
	$(INTDIR)/"pl-atom.obj"
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:windows /DLL /DEBUG /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib c:\jan\pl\bin\uxnt.lib /NOLOGO /SUBSYSTEM:windows /DLL /DEBUG /MACHINE:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib c:\jan\pl\bin\uxnt.lib /NOLOGO /SUBSYSTEM:windows /DLL\
 /INCREMENTAL:yes /PDB:$(OUTDIR)/"libpl.pdb" /DEBUG /MACHINE:I386\
 /OUT:$(OUTDIR)/"libpl.dll" /IMPLIB:$(OUTDIR)/"libpl.lib" 

$(OUTDIR)/libpl.dll : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Group "Source Files"

################################################################################
# Begin Source File

SOURCE=".\pl-dwim.c"

$(INTDIR)/"pl-dwim.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-util.c"

$(INTDIR)/"pl-util.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-rec.c"

$(INTDIR)/"pl-rec.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-comp.c"

$(INTDIR)/"pl-comp.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-write.c"

$(INTDIR)/"pl-write.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-table.c"

$(INTDIR)/"pl-table.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-pro.c"

$(INTDIR)/"pl-pro.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-itf.c"

$(INTDIR)/"pl-itf.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-term.c"

$(INTDIR)/"pl-term.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-file.c"

$(INTDIR)/"pl-file.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-prims.c"

$(INTDIR)/"pl-prims.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-dde.c"

$(INTDIR)/"pl-dde.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-stream.c"

$(INTDIR)/"pl-stream.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-bag.c"

$(INTDIR)/"pl-bag.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-flag.c"

$(INTDIR)/"pl-flag.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-nt.c"

$(INTDIR)/"pl-nt.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-ext.c"

$(INTDIR)/"pl-ext.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-prof.c"

$(INTDIR)/"pl-prof.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-wam.c"

$(INTDIR)/"pl-wam.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-sys.c"

$(INTDIR)/"pl-sys.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-main.c"

$(INTDIR)/"pl-main.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-load.c"

$(INTDIR)/"pl-load.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-os.c"

$(INTDIR)/"pl-os.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-read.c"

$(INTDIR)/"pl-read.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-glob.c"

$(INTDIR)/"pl-glob.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-modul.c"

$(INTDIR)/"pl-modul.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-trace.c"

$(INTDIR)/"pl-trace.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-store.c"

$(INTDIR)/"pl-store.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-setup.c"

$(INTDIR)/"pl-setup.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-save.c"

$(INTDIR)/"pl-save.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-proc.c"

$(INTDIR)/"pl-proc.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-dll.c"

$(INTDIR)/"pl-dll.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-dump.c"

$(INTDIR)/"pl-dump.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-op.c"

$(INTDIR)/"pl-op.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-funct.c"

$(INTDIR)/"pl-funct.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-list.c"

$(INTDIR)/"pl-list.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-fmt.c"

$(INTDIR)/"pl-fmt.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-arith.c"

$(INTDIR)/"pl-arith.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-fli.c"

$(INTDIR)/"pl-fli.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-wic.c"

$(INTDIR)/"pl-wic.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-buffer.c"

$(INTDIR)/"pl-buffer.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-gc.c"

$(INTDIR)/"pl-gc.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=".\pl-atom.c"

$(INTDIR)/"pl-atom.obj" :  $(SOURCE)  $(INTDIR)

# End Source File
# End Group
# End Project
################################################################################
