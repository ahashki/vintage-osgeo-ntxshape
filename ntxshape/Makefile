# * The contents of this file are subject to the Mozilla Public License 
# * Version 1.1 (the "License"); you may not use this file except in 
# * compliance with the License. You may obtain a copy of the License at
# * http://www.mozilla.org/MPL/
# * 
# * Software distributed under the License is distributed on an "AS IS" 
# * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See 
# * the License for the specific language governing rights and limitations 
# * under the License.
# * 
# * The Original Code is NTXShape - NTX to Shapefile Converter.
# * 
# * The Initial Developer of the Original Code is ESRI Canada Limited.
# * Portions created by ESRI Canada are Copyright (C) 1997-2003 
# * ESRI Canada Limited.  All Rights Reserved.
# * 
# * Contributor(s):
# *  Bruce Dodson <bdodson@esricanada.com>


CC = gcc

LIB_OBJS = intxfile.o ntx.o ntxrecs.o ntxshape.o ntxdesc.o ntxline.o \
   descattr.o dbfout.o dbfopen.o shpopen.o shpout.o \
   lineout.o pntout.o nameout.o patmatch.o \
   ntxbuild.o ntxsearch.o polyout.o

EXE_OBJS = ntxshape.ro stripext.o convert.o

DLL_OBJS = avntxshp.ro ntxshp.o avexec.o dllmain.o statusdlg.o

ALL_TARGETS = libntxshape.a ntxshape.exe ntxapi14.dll


.PHONY: all clean clean_objects clean_targets
.IGNORE: clean_objects clean_targets
.SILENT: clean_objects clean_targets

CFLAGS += -mtune=i686 -malign-double -Wall

ifdef DEBUG
CPPFLAGS += -DDEBUG
CFLAGS += -g
else
CFLAGS += -O3 -funroll-loops -ffast-math
LDFLAGS += -Wl,--strip-all
endif


ifdef PROFILE
CFLAGS += -pg
LDFLAGS += -pg
endif

CXXFLAGS = $(CFLAGS) 

%.o : %.cpp
	g++ -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@

%.ro: %.rc
	rc /i "%MSSdk%\include" $<
	windres -O coff $*.res $@
	rm $*.res

%.tlb: %.idl
	echo Generating typelib.  This may fail if CL.EXE is not found...
	midl /win32 /I "%MSSdk%\include" $<

#Someone please show me how to use the /cpp_cmd and /cpp_opt
#flags to invoke cpp.exe as the preprocessor for midl.

all: $(ALL_TARGETS)

libntxshape.a: $(LIB_OBJS)
	ar -rs libntxshape.a $^

ntxshape.exe: $(EXE_OBJS) libntxshape.a
	g++  $(LDFLAGS) -o $@ $^

ntxapi14.dll: $(DLL_OBJS) libntxshape.a
	g++ --shared $(LDFLAGS) -Wl,--add-stdcall-alias -Wl,--enable-auto-image-base -mthreads --mwindows -o $@ $^ -ladvapi32 -lcomctl32


clean: clean_objects clean_targets

clean_objects: 
	echo Removing intermediate files
	rm $(EXE_OBJS) $(DLL_OBJS) $(LIB_OBJS)

clean_targets:
	echo Removing target files
	rm $(ALL_TARGETS)

include dependencies
avntxshp.ro: ntxapi.tlb
