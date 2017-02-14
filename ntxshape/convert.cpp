/**
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is NTXShape - NTX to Shapefile Converter.
 *
 * The Initial Developer of the Original Code is ESRI Canada Limited.
 * Portions created by ESRI Canada are Copyright (C) 1997-2003
 * ESRI Canada Limited.  All Rights Reserved.
 *
 * Contributor(s):
 *  Bruce Dodson <bdodson@esricanada.com>
**/


#include "ntxshape.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "resource.h"

// I was using strcmpi but that's not portable enough
// patmatchi is overkill but will do the trick
#include "patmatch.h"

#ifndef FALSE
#define FALSE (0)
#define TRUE (1)
#endif

#include <sys/stat.h>
#include <sys/types.h>

#if defined(__GNUC__) && !defined(__stdcall)
#define __stdcall __attribute__((stdcall))
#endif

#ifdef __CYGWIN__
#include <unistd.h>
#endif

#if defined(unix)

#include <unistd.h>

#define PATH_SEPARATOR '/'
#define DRIVE_SEPARATOR '\0'

#else

#include <io.h>
#include <errno.h>
#define PATH_SEPARATOR '\\'
#define DRIVE_SEPARATOR ':'

#ifdef MSDOS
#define REQUIRE_SHORT_FILENAME 1
#endif

#define F_OK 0

#ifdef _WIN32
#include "appmutex.h"
static AppMutex appMutex("NTXSHAPE_APP_MUTEX");
#endif

#endif




extern "C" char * stripext(char *fname);
extern "C" char * stripbase(char *fname);
/*returns the extension, or null if nothing was stripped.*/

int dotsWritten=0;
float nextDot=5.0;

extern "C" {
    void __stdcall StatusProc(float percentDone) {
        int pct = (int)percentDone;
        while (pct >= nextDot) {
            putchar('.'); fflush(stdout);
            ++dotsWritten;
            nextDot += 5;
        }
    }


    void FinishStatus(int err) {
        if (!err) {
            for (;dotsWritten<20;++dotsWritten) {
                putchar('.');
            }
        }

        dotsWritten = 0;
        nextDot = 5;
        putchar('\n');
        fflush(stdout);
    }
} //extern "C"






static bool skiparg(char *pArg) {
    return (
        (0==*pArg) 
     || (0==strcmp(pArg,"-"))
     || (0==strcmp(pArg,"#"))
    );
}


static bool make_output_filename(char *outfname, char *fnamebase, char *suffix, bool needSuffix, bool shapefile, bool check_exists = false) {
    
    strcpy(outfname,fnamebase);

    if (suffix && *suffix && needSuffix) {
        strcat(outfname, suffix);
    }

    if (check_exists) {
        // check shp first, because it's a friendlier error message for the user
        if (shapefile) {
            strcat(outfname, ".shp");
        
            if (access(outfname,(F_OK))==0) {
                fprintf(stderr,"ERROR: File \"%s\" already exists.\n",outfname);
                return false;
            }
            stripext(outfname);
        }
        
        // also check dbf even for shapefile, in case there was a non-spatial table
        strcat(outfname,".dbf");
        if (access(outfname,(F_OK))==0) {
            fprintf(stderr,"ERROR: File \"%s\" already exists.\n",outfname);
            return false;
        }
        
        // now change the extension back to shp if the output is spatial - for friendly progress message.
        if (shapefile) {
            stripext(outfname);
            strcat(outfname, ".shp");
        }
        
    } else {
        
        // just set the extension; no need to check it.
        strcat(outfname, shapefile ? ".shp" : ".dbf");
        
    }

    return true;
}



int main(int argc,char**argv) {

    char version[] = "NTXSHAPE version " NTX_VERSION ", copyright (c) " NTX_COPYRIGHT_DATE " ESRI Canada Ltd.";

    char usage[] =
      "Usage: ntxshape <ntx_file> {out_file | out_path}\n"
      "                {ALL | POINT | LINE | POLYGON | DESC | NAME | LINEZ | datatype}\n"
      "                {SAFE | OVERWRITE | APPEND} {theme_number} {fcode_pattern}\n"
      "       ntxshape {-USAGE | -VERSION}";

    int err = 0;

    char * ntxfname = 0;
    char outputs[16] = "ALL";
    bool outputDesc=false;
    bool outputPoints=false;
    bool outputLines=false;
    bool outputLinesZ=false;
    bool outputNames=false;
    bool outputPolys=false;

    bool needSuffix=false;

    bool joinGLinked=true;
    bool delDeleted=true;

    bool overwrite=false;
    bool append=false;

    struct stat statbuf;


    char fnamebase[MAX_PATH];
    char * basename;

    char outfname[MAX_PATH];

    if (argc < 2 || skiparg(argv[1])) {
        puts(usage);
        return 1;
    } else if (patmatchi("-USAGE|-ADVANCED",argv[1])) {
        puts(usage);
        return 0;
    } else if (patmatchi("-VERSION",argv[1])) {
        puts(version);
        return 0;
    }

    ntxfname = argv[1];
    NTXConverter ntx(ntxfname);
    if (!ntx.IsValid()) {
        if (access(ntxfname,(F_OK))==-1) {
            (void)fprintf(stderr,"ERROR: NTX file \"%s\" does not exist.\n",ntxfname);
        } else {
            (void)fprintf(stderr,"ERROR: Could not open NTX file \"%s\"\n",ntxfname);
        }
        return 2;
    }

    if (argc < 3 || skiparg(argv[2])) {
        strcpy(fnamebase,argv[1]);
    } else {
        strcpy(fnamebase,argv[2]);

        if (stat(fnamebase,&statbuf)==0) {
            if (statbuf.st_mode & S_IFDIR) {
                int slen;
                slen = strlen(fnamebase);
                if (fnamebase[slen-1] != PATH_SEPARATOR) {
                    //if there are drive letters, see if this looks like
                    //a naked drive letter.
                    if (fnamebase[slen-1] != DRIVE_SEPARATOR) {
                        fnamebase[slen] = PATH_SEPARATOR;
                        fnamebase[slen+1] = '\0';
                    }
                }

                basename = strrchr(ntxfname,PATH_SEPARATOR);

                if (DRIVE_SEPARATOR && !basename) {
                    basename = strchr(ntxfname,DRIVE_SEPARATOR);
                }

                if (basename) {
                    ++basename;
                } else {
                    basename = ntxfname;
                }

                strcat(fnamebase,basename);
            }
        }
    }

    stripext(fnamebase);

    // make basename point at the correct file
    basename = strrchr(fnamebase,PATH_SEPARATOR);

    if (DRIVE_SEPARATOR && !basename) {
        basename = strchr(fnamebase, DRIVE_SEPARATOR);
    }

    if (basename) {
        ++basename;
    } else {
        basename = fnamebase;
    }



    if (argc > 3 && !skiparg(argv[3])) {
        strncpy(outputs,argv[3],sizeof(outputs)-2);
        outputs[ sizeof(outputs)-1 ] = '\0';
    }

    if (patmatchi("ALL", outputs)) {
        outputLines=outputPoints=true;
        
    } else if (patmatchi("POINT", outputs)) {
        outputPoints = true;

    } else if (patmatchi("LINE", outputs)) {
        outputLines = true;
        
    } else if (patmatchi("NAME", outputs)) {
        outputNames = true;

    } else if (patmatchi("DESCRIPTOR|DESC", outputs)) {
        outputDesc = true;

    } else if (patmatchi("POLYGON|POLY|BUILD", outputs)) {
        outputPolys = true;
        
    } else if (patmatchi("LINEZ", outputs)) {
        outputLinesZ = true;

    } else if (patmatchi("TOPOLOGICAL|TOPO", outputs)) {
        // The TOPOLOGICAL option is what used to be called POLYGON
        // before we supported real polygons.  It is no longer
        // documented in the main documentation or the usage, but
        // is still mentioned in the polygon technical notes.
        
        outputLines=outputPoints=true;
        
        ntx.SetDataType(7);
        ntx.AddDataType(1);
        ntx.AddDataType(3);
        ntx.AddDataType(4);
        ntx.SetDescFlags( (1<<10) | (1<<11) );
        
    } else {

        int datatype = atoi(outputs);
        outputPoints = NTXConverter::IsPoint(datatype);
        outputLines = NTXConverter::IsLine(datatype);

        if (outputPoints | outputLines) {
            ntx.SetDataType(datatype);
        } else if (datatype > 0 && datatype < 12) {
            outputDesc = true;
            ntx.SetDataType(datatype);
        } else {
            puts(usage);
            return 1;
        }
    }
    

    

    if (argc > 4 && !skiparg(argv[4])) {
        if (patmatchi("OVERWRITE|OVER",argv[4])) {
            overwrite = true;
        } else if (patmatchi("APPEND",argv[4])) {
            append = true;
        } else if (!patmatchi("SAFE", argv[4])) {
            puts(usage);
            return 1;
        }
    }

    int themeNum = NTX_NODATA;
    if (argc > 5 && !skiparg(argv[5]) && !patmatchi("ALL",argv[5]) ) {
        if (EOF == sscanf(argv[5],"%d", &themeNum)) {  // WAS || themeNum < 0
            fprintf(stderr, "ERROR: Invalid theme number \"%s\"\n",argv[5]);
            return 5;
        }
    }

    char *fcodePat = NULL;

    if (argc > 6 && !skiparg(argv[6])) {
        fcodePat = argv[6];
    }

    // joinGLinked and delDeleted start out as true
    if (argc > 7 && !skiparg(argv[7])) {
        // Unless the user specified the RAW flag, we will join features
        // that are graphically linked, and skip features that are
        // marked for deletion.
        
        // The NOJOIN, NODELETE, and RAW options are deprecated,
        // no longer documented, and will be removed at some point.

        if ( patmatchi("NOJOIN", argv[7])==0 ) {
            joinGLinked = false;
        } else if (patmatchi("NODELETE|NODEL", argv[7])) {
            delDeleted = false;
        } else if (patmatchi("RAW",argv[7])) {
            delDeleted = joinGLinked = false;
        }

    }

    ntx.SetHonourGraphicLink(joinGLinked ? 1 : 0);
    ntx.SetExcludeDescFlags(delDeleted ? 1<<5 : 0);

    needSuffix = (outputDesc + outputPoints + outputLines + outputLinesZ + outputPolys + outputNames) > 1;

#if (REQUIRE_SHORT_FILENAME)
    //8.3 compatibility - if more than one output type, truncate the
    //output name at 6 characters to make room for '_' and a suffix.
    basename[needSuffix ? 6 : 8]='\0';
#endif

    
    if (needSuffix) {
        strcat(basename,"_");
    }
    
    if (!(overwrite||append)) {
        // before writing anything, check whether any of our output is going to overwrite existing files.
        if (
            (outputDesc   && !make_output_filename(outfname, fnamebase, "d", needSuffix, false, true))
         || (outputPoints && !make_output_filename(outfname, fnamebase, "p", needSuffix, true, true))
         || (outputLines  && !make_output_filename(outfname, fnamebase, "l", needSuffix, true, true))
         || (outputLinesZ && !make_output_filename(outfname, fnamebase, "z", needSuffix, true, true))
         || (outputNames  && !make_output_filename(outfname, fnamebase, "n", needSuffix, true, true))
         || (outputPolys  && !make_output_filename(outfname, fnamebase, "r", needSuffix, true, true))
        ) {
            // error message has already been displayed
            return 3;
        }
    }

    
    printf("NTX file \"%s\" opened", ntxfname);

    if (themeNum != NTX_NODATA) {
        if (themeNum >= 0) {
            printf(" for theme %d",themeNum);
        } else {
            printf(" for all themes except %d",-themeNum);
        }
    }


    printf("...\n");

    fflush(stdout);



    ntx.SetThemeNumber(themeNum);
    ntx.SetFcodePattern(fcodePat);
    ntx.SetStatusCallback(StatusProc);



    if (outputDesc) {
        make_output_filename(outfname, fnamebase, "d", needSuffix, false);
        
        printf ("Writing descriptors to table \"%s\"",outfname);
        fflush(stdout);

        err = ntx.ConvertDescriptors(outfname, append);
        FinishStatus(err);
        if (err) {
            fprintf(stderr,"ERROR: NTX file \"%s\"\n",ntxfname);
            fprintf(stderr,"ERROR: Could not convert descriptors to \"%s\"\n",outfname);
            return 4;
        }
    }


    if (outputPoints) {
        make_output_filename(outfname, fnamebase, "p", needSuffix, true);

        printf("Writing points to shapefile \"%s\"",outfname);
        fflush(stdout);

        err = ntx.ConvertPoints(outfname, append);
        FinishStatus(err);
        if (err) {
            fprintf(stderr,"ERROR: NTX file \"%s\"\n",ntxfname);
            fprintf(stderr,"ERROR: Could not convert points to \"%s\"\n",outfname);
            return 4;
        }
    }

    if (outputLines) {
        make_output_filename(outfname, fnamebase, "l", needSuffix, true);
        
        printf("Writing lines to shapefile \"%s\"",outfname);
        fflush(stdout);
        err = ntx.ConvertLines(outfname, append);
        FinishStatus(err);
        if (err) {
            fprintf(stderr,"ERROR: NTX file \"%s\"\n",ntxfname);
            fprintf(stderr,"ERROR: Could not convert lines to \"%s\"\n",outfname);
            return 4;
        }
    }
    
    if (outputLinesZ) {
        make_output_filename(outfname, fnamebase, "z", needSuffix, true);

        printf("Writing 3D lines to shapefile \"%s\"",outfname);
        fflush(stdout);
        err = ntx.ConvertLinesZ(outfname, append);
        FinishStatus(err);
        if (err) {
            fprintf(stderr,"ERROR: NTX file \"%s\"\n",ntxfname);
            fprintf(stderr,"ERROR: Could not convert lines to \"%s\"\n",outfname);
            return 4;
        }
    }


    if (outputNames) {
        make_output_filename(outfname, fnamebase, "n", needSuffix, true);
        
        printf("Writing names to shapefile \"%s\"",outfname);
        fflush(stdout);

        err = ntx.ConvertNames(outfname, append);
        FinishStatus(err);
        if (err) {
            fprintf(stderr,"ERROR: NTX file \"%s\"\n",ntxfname);
            fprintf(stderr,"ERROR: Could not convert names to \"%s\"\n",outfname);
            return 4;
        }
    }


    if (outputPolys) {
        make_output_filename(outfname, fnamebase, "r", needSuffix, true);
        
        printf("Writing polygons to shapefile \"%s\"",outfname); //...
        fflush(stdout);

        err = ntx.ConvertPolygons(outfname, append);
        FinishStatus(err);
        if (err) {
            fprintf(stderr,"ERROR: NTX file \"%s\"\n",ntxfname);
            fprintf(stderr,"ERROR: Could not build polygons to \"%s\"\n",outfname);
            return 4;
        }
    }


    return 0;
}




