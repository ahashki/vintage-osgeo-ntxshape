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
 *  Frank Warmerdam <warmerdam@pobox.com>
**/



/**
 * The original source code contained the following notices:
 *      "Copyright (c) 1995 Frank Warmerdam"
 *      "This code is in the public domain."
 *
 * I took the public domain version and used it in NTXShape.
 * Error reports from customers exposed bugs in shapelib.
 * Since shapelib was not well maintained at the time, I
 * fixed the problems myself.
 *
 * Since then Frank Warmerdam has resumed development of 
 * the library, asserted a copyright on the new version(s), 
 * and is now releasing his new version(s) under LGPL/MIT 
 * dual license.  Frank's version now appears to be well
 * maintained, with regular updates, a different API, and 
 * more features than the version included in NTXShape.
 *
 * I am not using Frank's updated releases; I don't know 
 * whether the new releases fix the bugs I found over the
 * years through NTXShape, and I don't know to what degree
 * NTXShape relies upon changes that I made, and I don't
 * know to what degree the new API would break my code.
 * I see no reason to find out any of these things.
 *
 * However, it is not my intention to start a "competing"
 * version of shapelib.  For new projects, I suggest you
 * use Frank's version:
 *      http://gdal.velocet.ca/projects/shapelib/
 *
 *
 * The last modification to the public domain source code
 * attributed to Frank Warmerdam was dated 1995/10/21.
 *
 * Changes from the public domain version of 1995/10/21:
 *
 * Revision 1.10  2002/05/12 23:47:45  bdodson
 * Frank Warmerdam's final release in the 1.1.x series had one bug fix
 * that I had not noticed / fixed in my version, a memory leak in SHPOpen.
 * That was fixed in Frank's shpopen.c, revision 1.9, 1998/02/24
 *
 * Revision 1.9  2002/05/11 02:20:00  bdodson
 * cleaner either-endian handling replaced ifdefs with macros
 * fixed indentation and formatting, added Mozilla license notice
 *
 * Revision 1.8  2000-04-24 14:32:37-03  bdodson
 * Removed some unused variables
 * Commented out the "if" clause on final case in SHPWriteVertices, so -Wall
 *  would not complain about uninitialized vars.  This would bomb on 3d
 *  or measured shapefile, but it would have bombed before anyway.
 *
 * Revision 1.7  2000-03-27 19:18:32-04  bdodson
 * dropped rcsid[]
 *
 * Revision 1.6  2000-03-27 16:08:22-04  bdodson
 * Replace "if (bBigendian)" with an #ifdef wherever it occurs.
 * The endian-ness is not going to change between compile time and run-time.
 *
 * Revision 1.5  1999-12-13 19:30:51-04  bdodson
 * <>
 *
 * Revision 1.5  1999-12-13 19:29:54-04  bdodson
 * Outdated (renamed to 'E:\Projects\NTXShape\rcs\src\shpopen.cpp,v').
 *
 * Revision 1.4  1999-12-13 19:29:44-04  bdodson
 * C++, performance tweaks
 *
 * Revision 1.3  1999-03-31 18:59:49-04  bdodson
 * Fixed record-num and record-len values in record headers.  They were in the wrong byte order.
 *
 * Revision 1.2  1999-01-29 12:45:25-04  bdodson
 * include string.h
 *
 * Revision 1.1  1998/06/10 18:43:27  bdodson
 * Initial revision
 *
**/

#include "geom.h"

extern "C" {




#include "shpfile.h"

#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>


#if UINT_MAX == 65535
typedef long	      int32;
#else
typedef int	      int32;
#endif

#ifndef FALSE
#  define FALSE		0
#  define TRUE		1
#endif

#define ByteCopy( a, b, c )	memcpy( (void*)(b), (void*)(a), (c) )

#ifndef MAX
#  define MIN(a,b)      ((a<b) ? a : b)
#  define MAX(a,b)      ((a>b) ? a : b)
#endif


/************************************************************************/
/*                              SwapWord()                              */
/*                                                                      */
/*      Swap a 2, 4 or 8 byte word.                                     */
/************************************************************************/

static void SwapWord( int length, void * wordP ) {
     int i;
     unsigned char temp;
 
     for ( i=0; i < length/2; i++ ) {
         temp = ((unsigned char *) wordP)[i];
         ((unsigned char *)wordP)[i] = ((unsigned char *) wordP)[length-i-1];
         ((unsigned char *) wordP)[length-i-1] = temp;
     }
}

#ifdef _BIGENDIAN_MACHINE
//sparc
#define NativeToBigEndian(l,p)
#define NativeToLittleEndian(l,p) SwapWord(l,p)
#else
//intel
#define NativeToBigEndian(l,p) SwapWord(l,p)
#define NativeToLittleEndian(l,p)
#endif
/************************************************************************/
/*                          SHPWriteHeader()                            */
/*                                                                      */
/*      Write out a header for the .shp and .shx files as well as the	*/
/*	contents of the index (.shx) file.				*/
/************************************************************************/

static void SHPWriteHeader( SHPHandle psSHP) {
    unsigned char abyHeader[100];
    int i;
    int32 i32;
    double dValue;
    int32 * panSHX;
 
    /* -------------------------------------------------------------------- */
    /*      Prepare header block for .shp file.                             */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < 100; i++) {
        abyHeader[i] = 0;
    }
 
    abyHeader[2] = 0x27;				/* magic cookie */
    abyHeader[3] = 0x0a;
 
    i32 = psSHP->nFileSize/2;				/* file size */
    ByteCopy( &i32, abyHeader+24, 4 );
    NativeToBigEndian(4, abyHeader+24 );

    i32 = 1000;						/* version */
    ByteCopy( &i32, abyHeader+28, 4 );
    NativeToLittleEndian(4, abyHeader+28 );

    i32 = psSHP->nShapeType;				/* shape type */
    ByteCopy( &i32, abyHeader+32, 4 );
    NativeToLittleEndian(4, abyHeader+32 );
    
    for (int bnd = 0; bnd < 8; ++bnd) {        // set bounds
        dValue = psSHP->adBounds[bnd];			
        ByteCopy( &dValue, abyHeader + 36 + (bnd*8), 8 );
        NativeToLittleEndian( 8, abyHeader + 36 + (bnd*8) );
    }

    
/* -------------------------------------------------------------------- */
/*      Write .shp file header.                                         */
/* -------------------------------------------------------------------- */
    fseek( psSHP->fpSHP, 0, 0 );
    fwrite( abyHeader, 100, 1, psSHP->fpSHP );
 
    /* -------------------------------------------------------------------- */
    /*      Prepare, and write .shx file header.                            */
    /* -------------------------------------------------------------------- */
    i32 = (psSHP->nRecords * 2 * sizeof(int32) + 100)/2;   /* file size */
    ByteCopy( &i32, abyHeader+24, 4 );
    NativeToBigEndian(4, abyHeader+24 );
 
    fseek( psSHP->fpSHX, 0, 0 );
    fwrite( abyHeader, 100, 1, psSHP->fpSHX );
 
    /* -------------------------------------------------------------------- */
    /*      Write out the .shx contents.                                    */
    /* -------------------------------------------------------------------- */
    panSHX = (int32 *) malloc(sizeof(int32) * 2 * psSHP->nRecords);
 
    for ( i = 0; i < psSHP->nRecords; i++ ) {
        panSHX[i*2  ] = psSHP->panRecOffset[i]/2;
        panSHX[i*2+1] = psSHP->panRecSize[i]/2;
 
        NativeToBigEndian( 4, panSHX+i*2 );
        NativeToBigEndian( 4, panSHX+i*2+1 );
    }
 
    fwrite( panSHX, sizeof(int32) * 2, psSHP->nRecords, psSHP->fpSHX );
 
    free( panSHX );
}






/************************************************************************/
/*                              SHPOpen()                               */
/*                                                                      */
/*      Open the .shp and .shx files based on the basename of the       */
/*      files or either file name.                                      */
/************************************************************************/

SHPHandle SHPOpen( const char * pszLayer, const char * pszAccess ) {
    char *pszFullname, *pszBasename;
    SHPHandle psSHP;

    unsigned char *pabyBuf;
    int i;
    double dValue;

/* -------------------------------------------------------------------- */
/*      Ensure the access string is one of the legal ones.              */
/* -------------------------------------------------------------------- */
    if (
        strcmp(pszAccess,"r") != 0
     && strcmp(pszAccess,"r+") != 0
     && strcmp(pszAccess,"rb") != 0 
     && strcmp(pszAccess,"rb+") != 0 
    ) {
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*	Initialize the info structure.					*/
/* -------------------------------------------------------------------- */
    psSHP = (SHPHandle) malloc(sizeof(SHPInfo));

    psSHP->bUpdated = FALSE;

/* -------------------------------------------------------------------- */
/*	Compute the base (layer) name.  If there is any extension	*/
/* on the passed in filename we will strip it off.                      */
/* -------------------------------------------------------------------- */
    pszBasename = (char *) malloc(strlen(pszLayer)+5);
    strcpy( pszBasename, pszLayer );
    for (
        i = strlen(pszBasename)-1; 
        i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/' && pszBasename[i] != '\\';
        i-- 
    ) {
        //no-op
    }

    if (pszBasename[i] == '.') {
        pszBasename[i] = '\0';
    }

/* -------------------------------------------------------------------- */
/* Open the .shp and .shx files.  Note that files pulled from	*/
/* a PC to Unix with upper case filenames won't work!		*/
/* -------------------------------------------------------------------- */
    pszFullname = (char *) malloc(strlen(pszBasename) + 5);
    sprintf( pszFullname, "%s.shp", pszBasename );
    psSHP->fpSHP = fopen(pszFullname, pszAccess );
    if (psSHP->fpSHP == NULL) {
        return NULL;
    }

    sprintf( pszFullname, "%s.shx", pszBasename );
    psSHP->fpSHX = fopen(pszFullname, pszAccess );
    if (psSHP->fpSHX == NULL) {
        return NULL;
    }

    free( pszFullname );
    free( pszBasename );

/* -------------------------------------------------------------------- */
/*  Read the file size from the SHP file.				*/
/* -------------------------------------------------------------------- */
    pabyBuf = (unsigned char *) malloc(100);
    fread( pabyBuf, 100, 1, psSHP->fpSHP );

    psSHP->nFileSize = 2 * (
        pabyBuf[24] * 0x1000000
      + pabyBuf[25] * 0x10000
      + pabyBuf[26] * 0x100
      + pabyBuf[27]
    );

/* -------------------------------------------------------------------- */
/*  Read SHX file Header info                                           */
/* -------------------------------------------------------------------- */
    fread( pabyBuf, 100, 1, psSHP->fpSHX );

    if (
        pabyBuf[0] != 0
     || pabyBuf[1] != 0
     || pabyBuf[2] != 0x27
     || (pabyBuf[3] != 0x0a && pabyBuf[3] != 0x0d)
    ) {
        fclose(psSHP->fpSHP);
        fclose(psSHP->fpSHX);
        free(psSHP);

        return NULL;
    }

    psSHP->nRecords = pabyBuf[27] + (
        pabyBuf[26] * 0x100
      + pabyBuf[25] * 0x10000 
      + pabyBuf[24] * 0x1000000
    );
    psSHP->nRecords = (psSHP->nRecords*2 - 100) / 8;

    psSHP->nShapeType = (SHPShapeType) pabyBuf[32];
    
    for (int bnd=0; bnd<8; ++bnd) {
        NativeToLittleEndian(8, pabyBuf + 36 + bnd*8);
        memcpy( &dValue, pabyBuf + 36 + bnd*8, 8);
        psSHP->adBounds[bnd] = dValue;
    }

    free(pabyBuf);

/* -------------------------------------------------------------------- */
/*	Read the .shx file to get the offsets to each record in 	*/
/*	the .shp file.							*/
/* -------------------------------------------------------------------- */
    psSHP->nMaxRecords = psSHP->nRecords;

    psSHP->panRecOffset = (int *) malloc(sizeof(int) * psSHP->nMaxRecords );
    psSHP->panRecSize = (int *) malloc(sizeof(int) * psSHP->nMaxRecords );

    pabyBuf = (unsigned char *) malloc(8 * psSHP->nRecords );
    fread( pabyBuf, 8, psSHP->nRecords, psSHP->fpSHX );

    for (i = 0; i < psSHP->nRecords; i++) {
        int32 nOffset, nLength;

        memcpy(&nOffset, pabyBuf + i * 8, 4);
        NativeToBigEndian(4, &nOffset);

        memcpy(&nLength, pabyBuf + i * 8 + 4, 4);
        NativeToBigEndian(4, &nLength);
     
        psSHP->panRecOffset[i] = nOffset*2;
        psSHP->panRecSize[i] = nLength*2;
    }
    
    free(pabyBuf);

    return psSHP;
}





/************************************************************************/
/*                              SHPClose()                              */
/*								       	*/
/*	Close the .shp and .shx files.					*/
/************************************************************************/

void SHPClose(SHPHandle psSHP ) {
    /* -------------------------------------------------------------------- */
    /*	Update the header if we have modified anything.			*/
    /* -------------------------------------------------------------------- */
    if (psSHP->bUpdated) {
        SHPWriteHeader(psSHP);
    }

    /* -------------------------------------------------------------------- */
    /*      Free all resources, and close files.                            */
    /* -------------------------------------------------------------------- */
    free(psSHP->panRecOffset);
    free(psSHP->panRecSize);

    fclose(psSHP->fpSHX);
    fclose(psSHP->fpSHP);

    free(psSHP);
}




/************************************************************************/
/*                             SHPGetInfo()                             */
/*                                                                      */
/*      Fetch general information about the shape file.                 */
/************************************************************************/

void SHPGetInfo(SHPHandle psSHP, int * pnEntities, SHPShapeType * pnShapeType ) {
    if (pnEntities != NULL) {
        *pnEntities = psSHP->nRecords;
    }

    if (pnShapeType != NULL) {
        *pnShapeType = psSHP->nShapeType;
    }
}





/************************************************************************/
/*                             SHPCreate()                              */
/*                                                                      */
/*      Create a new shape file and return a handle to the open         */
/*      shape file with read/write access.                              */
/************************************************************************/

SHPHandle SHPCreate(const char * pszLayer, SHPShapeType nShapeType) {
    char *pszBasename, *pszFullname;
    int i;
    FILE *fpSHP, *fpSHX;
    unsigned char abyHeader[100];
    int32 i32;
    double dValue;


/* -------------------------------------------------------------------- */
/*	Compute the base (layer) name.  If there is any extension	*/
/*	on the passed in filename we will strip it off.			*/
/* -------------------------------------------------------------------- */
    pszBasename = (char *) malloc(strlen(pszLayer)+5);
    strcpy( pszBasename, pszLayer );
    for (
        i = strlen(pszBasename)-1;
        i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/' && pszBasename[i] != '\\';
        i-- 
    ) {
        // no-op
    }

    if (pszBasename[i] == '.') {
        pszBasename[i] = '\0';
    }

/* -------------------------------------------------------------------- */
/*      Open the two files so we can write their headers.               */
/* -------------------------------------------------------------------- */
    pszFullname = (char *) malloc(strlen(pszBasename) + 5);
    sprintf(pszFullname, "%s.shp", pszBasename);
    fpSHP = fopen(pszFullname, "wb");
    if (fpSHP == NULL) {
        return NULL;
    }

    sprintf(pszFullname, "%s.shx", pszBasename);
    fpSHX = fopen(pszFullname, "wb");
    if (fpSHX == NULL) {
        return NULL;
    }

    free(pszFullname);
    free(pszBasename);

/* -------------------------------------------------------------------- */
/*      Prepare header block for .shp file.                             */
/* -------------------------------------------------------------------- */
    for (i = 0; i < 100; i++) {
        abyHeader[i] = 0;
    }

    abyHeader[2] = 0x27;				/* magic cookie */
    abyHeader[3] = 0x0a;

    i32 = 50;						/* file size */
    ByteCopy( &i32, abyHeader+24, 4 );
    NativeToBigEndian( 4, abyHeader+24 );

    i32 = 1000;						/* version */
    ByteCopy( &i32, abyHeader+28, 4 );
    NativeToLittleEndian( 4, abyHeader+28 );

    i32 = nShapeType;					/* shape type */
    ByteCopy( &i32, abyHeader+32, 4 );
    NativeToLittleEndian( 4, abyHeader+32 );
    
    dValue = 0.0;					/* set bounds */
    for (int bnd=0; bnd<8; ++bnd) {
        ByteCopy( &dValue, abyHeader + 36 + bnd*8, 8 );
        NativeToLittleEndian( 8, abyHeader + 36 + bnd*8 );
    }

/* -------------------------------------------------------------------- */
/*      Write .shp file header.                                         */
/* -------------------------------------------------------------------- */
    fwrite( abyHeader, 100, 1, fpSHP );

/* -------------------------------------------------------------------- */
/*      Prepare, and write .shx file header.                            */
/* -------------------------------------------------------------------- */
    i32 = 50;						/* file size */
    ByteCopy( &i32, abyHeader+24, 4 );
    NativeToBigEndian( 4, abyHeader+24 );
    fwrite( abyHeader, 100, 1, fpSHX );

/* -------------------------------------------------------------------- */
/*      Close the files, and then open them as regular existing files.  */
/* -------------------------------------------------------------------- */
    fclose( fpSHP );
    fclose( fpSHX );

    return SHPOpen( pszLayer, "rb+" );
}

/************************************************************************/
/*                           _SHPSetBounds()                            */
/*                                                                      */
/*      Compute a bounds rectangle for a shape, and set it into the     */
/*      indicated location in the record.                               */
/************************************************************************/

static void _SHPSetBounds( unsigned char * pabyRec, int nVCount, double * padVertices) {
    double adBounds[4] = {0,0,0,0};
    int i;

    if ( nVCount != 0 ) {
        adBounds[0] = adBounds[2] = padVertices[0];
        adBounds[1] = adBounds[3] = padVertices[1];
  
        for ( i = 1; i < nVCount; i++ ) {
            double &x = padVertices[i*2];
            double &y = padVertices[i*2+1];
            
            if (x < adBounds[0])
                adBounds[0] = x;
            if (x > adBounds[2])
                adBounds[2] = x;
            if (y < adBounds[1])
                adBounds[1] = y;
            if (y > adBounds[3])
                adBounds[3] = y;
        }
    }
    
    for (int bnd=0; bnd<4; ++bnd) {
        NativeToLittleEndian( 8, adBounds+bnd );
        ByteCopy( adBounds+bnd, pabyRec + bnd*8, 8 );
    }
}





/************************************************************************/
/*                          SHPWriteVertices()                          */
/*                                                                      */
/*      Write out the vertices of a new structure.  Note that it is     */
/*      only possible to write vertices at the end of the file.         */
/************************************************************************/

int SHPWriteVertices(
    SHPHandle psSHP, 
    int nVCount, int nPartCount,
    int * panParts, double * padVertices
) {
    return SHPWriteVerticesEx(psSHP, nVCount, nPartCount, panParts, padVertices, NULL, NULL);
}

int SHPWriteVerticesEx(
    SHPHandle psSHP,
    int nVCount, int nPartCount,
    int * panParts, double * padVertices,
    double *zValues, double *mValues
) {
    int nRecordOffset, nRecordSize;
    unsigned char *pabyRec = NULL;
    int32 i32;
    int i;

    psSHP->bUpdated = TRUE;

/* -------------------------------------------------------------------- */
/*      Add the new entity to the in memory index.                      */
/* -------------------------------------------------------------------- */
    psSHP->nRecords++;
    if ( psSHP->nRecords > psSHP->nMaxRecords ) {
        psSHP->nMaxRecords = (int)(psSHP->nMaxRecords * 1.3) + 100;

        psSHP->panRecOffset = (int *) realloc(psSHP->panRecOffset,sizeof(int) * psSHP->nMaxRecords);
        psSHP->panRecSize = (int *) realloc(psSHP->panRecSize,sizeof(int) * psSHP->nMaxRecords);
    }

/* -------------------------------------------------------------------- */
/*      Initialize record.                                              */
/* -------------------------------------------------------------------- */
    psSHP->panRecOffset[psSHP->nRecords-1] = nRecordOffset = psSHP->nFileSize;
    
    int recSize = nVCount * 16 + nPartCount * 4 + 128;
    if (psSHP->nShapeType > 10) {
        //measures
        recSize += (nVCount * 8 + 16);
        
        //zeds, too
        if (psSHP->nShapeType < 20) {
            recSize += (nVCount * 8 + 16);
        }
    }

    pabyRec = (unsigned char *) malloc(recSize);

/* -------------------------------------------------------------------- */
/*  Extract vertices for a Polygon or Arc.				*/
/* -------------------------------------------------------------------- */
    
    int basicShapeType = psSHP->nShapeType < 30 ? psSHP->nShapeType % 10 : 0;
    
    if ( basicShapeType == SHPT_POLYGON || basicShapeType == SHPT_ARC ) {
        int32 nPoints, nParts;

        nPoints = nVCount;
        nParts = nPartCount;

        _SHPSetBounds( pabyRec + 12, nVCount, padVertices );

        NativeToLittleEndian( 4, &nPoints );
        NativeToLittleEndian( 4, &nParts );

        ByteCopy( &nPoints, pabyRec + 40 + 8, 4 );
        ByteCopy( &nParts, pabyRec + 36 + 8, 4 );

        ByteCopy( panParts, pabyRec + 44 + 8, 4 * nPartCount );
        for ( i = 0; i < nPartCount; i++ ) {
            NativeToLittleEndian( 4, pabyRec + 44 + 8 + 4*i );
        }

        ByteCopy(padVertices, pabyRec + 44 + 4*nPartCount + 8, 16*nVCount);

        for ( i = 0; i < nVCount; i++ ) {
            NativeToLittleEndian( 8, pabyRec + 44+4*nPartCount+8+i*16 );
            NativeToLittleEndian( 8, pabyRec + 44+4*nPartCount+8+i*16+8 );
        }

        nRecordSize = 44 + 4*nPartCount + 16*nVCount;
        
        if (psSHP->nShapeType > 10 && psSHP->nShapeType < 20) {
            //add z stuff
            if (zValues) {
                double zLimits[2] = {zValues[0], zValues[0]};
                
                for (i = 1; i < nVCount ; ++i) {
                    if (zValues[i] < zLimits[0])
                        zLimits[0] = zValues[i];
                    if (zValues[i] > zLimits[1])
                        zLimits[1] = zValues[i];
                }
                
                ByteCopy( zLimits, pabyRec + 8 + nRecordSize, 16);
                ByteCopy( zValues, pabyRec + 8 + nRecordSize + 16, 8 * nVCount);
                
                
                
            } else {
                // no bits set makes a zero.  NaN doesn't seem to work for Z.
                memset(pabyRec + 8 + nRecordSize, 0, 16 + 8 * nVCount);
                //for (i = -2; i < nVCount; ++i) {
                //    *(double*)(pabyRec + 8 + nRecordSize + 16 + i*8) = 0;
                //}
            }
            
            for (i = -2; i < nVCount; ++i) {
                NativeToLittleEndian( 8, pabyRec + 8 + nRecordSize + 16 + i*8);
            }
            
            nRecordSize += (16 + 8 * nVCount);
        }
        
        if (psSHP->nShapeType > 10 && psSHP->nShapeType < 30) {
            //add m stuff
            if (mValues) {
                double mLimits[2] = {mValues[0], mValues[0]};
                
                for (i = 1; i < nVCount ; ++i) {
                    if (mValues[i] < mLimits[0])
                        mLimits[0] = mValues[i];
                    if (mValues[i] > mLimits[1])
                        mLimits[1] = mValues[i];
                }
                
                ByteCopy( mLimits, pabyRec + 8 + nRecordSize, 16);
                ByteCopy( mValues, pabyRec + 8 + nRecordSize + 16, 8 * nVCount);
                
            } else {
                //all-bits-set in an FP makes a quiet NaN.
                memset(pabyRec + 8 + nRecordSize, 0xFF, 16 + 8 * nVCount);
                //for (i = -2; i < nVCount; ++i) {
                //    *(double*)(pabyRec + 8 + nRecordSize + 16 + i*8) = 0;
                //}
            }
            
            for (i = -2; i < nVCount; ++i) {
                NativeToLittleEndian( 8, pabyRec + 8 + nRecordSize + 16 + i*8);
            }
            
            nRecordSize += (16 + 8 * nVCount);
        }
        
    } else if ( basicShapeType == SHPT_MULTIPOINT ) {
        /* -------------------------------------------------------------------- */
        /*  Extract vertices for a MultiPoint.					*/
        /* -------------------------------------------------------------------- */
        int32		nPoints;

        nPoints = nVCount;

        _SHPSetBounds( pabyRec + 12, nVCount, padVertices );

        NativeToLittleEndian( 4, &nPoints );

        ByteCopy( &nPoints, pabyRec + 44, 4 );
        ByteCopy( panParts, pabyRec + 48, 16 * nVCount );

        for ( i = 0; i < nVCount; i++ ) {
            NativeToLittleEndian( 8, pabyRec + 48 + i*16 );
            NativeToLittleEndian( 8, pabyRec + 48 + i*16 + 8 );
        }

        nRecordSize = 40 + 16 * nVCount;
        
        // TODO: M and Z, as with SHPT_ARC
        
    } else if (basicShapeType == SHPT_POINT) {
        /* -------------------------------------------------------------------- */
        /*      Extract vertices for a point.                                   */
        /* -------------------------------------------------------------------- */
        ByteCopy( padVertices, pabyRec + 12, 16 );

        NativeToLittleEndian( 8, pabyRec + 12 );
        NativeToLittleEndian( 8, pabyRec + 20 );
        
        nRecordSize = 20;

        // TODO: M and Z.


    } else {
        // TODO: double check that we can never get here
        nRecordSize = 0;
        return -1;
    }
        

/* -------------------------------------------------------------------- */
/*      Set the shape type, record number, and record size.             */
/* -------------------------------------------------------------------- */
    i32 = psSHP->nRecords-1+1;					/* record # */
    NativeToBigEndian( 4, &i32 );
    ByteCopy( &i32, pabyRec, 4 );

    i32 = nRecordSize/2;				/* record size */
    NativeToBigEndian( 4, &i32 );
    ByteCopy( &i32, pabyRec + 4, 4 );

    i32 = psSHP->nShapeType;				/* shape type */
    NativeToLittleEndian( 4, &i32 );
    ByteCopy( &i32, pabyRec + 8, 4 );

/* -------------------------------------------------------------------- */
/*      Write out record.                                               */
/* -------------------------------------------------------------------- */
    fseek( psSHP->fpSHP, nRecordOffset, 0 );
    fwrite( pabyRec, nRecordSize+8, 1, psSHP->fpSHP );
    free( pabyRec );

    psSHP->panRecSize[psSHP->nRecords-1] = nRecordSize;
    psSHP->nFileSize += nRecordSize + 8;

/* -------------------------------------------------------------------- */
/*	Expand file wide bounds based on this shape.			*/
/* -------------------------------------------------------------------- */

    if ( psSHP->nRecords == 1 ) {
        init_box(psSHP->adBounds, padVertices, nVCount);
    } else {
        expand_box(psSHP->adBounds, padVertices, nVCount);
    }
    
    // TODO: limits for Z and M

    return psSHP->nRecords - 1;
}

/************************************************************************/
/*                          SHPReadVertices()                           */
/*                                                                      */
/*      Read the vertices for one shape from the shape file.            */
/************************************************************************/


// Does not support reading M/Z, although it will read the X/Y coords for 
// a shapefile even if it's an M or Z shapefile.  If you want full "read"
// support for M/Z, add it, or use Frank's version which already has that.

double * SHPReadVertices(
    SHPHandle psSHP, int hEntity,
    int * pnVCount, int * pnPartCount, int ** ppanParts
) {
    static unsigned char *pabyRec = NULL;
    static double *padVertices = NULL;
    static int nVertMax = 0, nPartMax = 0, *panParts = NULL;
    static int nBufSize = 0;
    
/* -------------------------------------------------------------------- */
/*      Validate the record/entity number.                              */
/* -------------------------------------------------------------------- */
    if ( hEntity < 0 || hEntity >= psSHP->nRecords ) {
        return NULL;
    }
    

/* -------------------------------------------------------------------- */
/*      Ensure our record buffer is large enough.                       */
/* -------------------------------------------------------------------- */
    if ( psSHP->panRecSize[hEntity]+8 > nBufSize ) {
        nBufSize = psSHP->panRecSize[hEntity]+8;
        pabyRec = (unsigned char *) realloc(pabyRec,nBufSize);
    }

/* -------------------------------------------------------------------- */
/*      Read the record.                                                */
/* -------------------------------------------------------------------- */
    fseek( psSHP->fpSHP, psSHP->panRecOffset[hEntity], 0 );
    fread( pabyRec, psSHP->panRecSize[hEntity]+8, 1, psSHP->fpSHP );

    if ( pnPartCount != NULL ) {
        *pnPartCount = 0;
    }

    if ( ppanParts != NULL ) {
        *ppanParts = NULL;
    }

    *pnVCount = 0;
    
    int basicType = psSHP->nShapeType < 30 ? psSHP->nShapeType % 10 : 0;

/* -------------------------------------------------------------------- */
/*  Extract vertices for a Polygon or Arc.				*/
/* -------------------------------------------------------------------- */
    if ( basicType == SHPT_POLYGON || basicType == SHPT_ARC ) {
        int32 nPoints, nParts;
        int i;

        memcpy( &nPoints, pabyRec + 40 + 8, 4 );
        memcpy( &nParts, pabyRec + 36 + 8, 4 );

        NativeToLittleEndian( 4, &nPoints );
        NativeToLittleEndian( 4, &nParts );

        *pnVCount = nPoints;
        *pnPartCount = nParts;

/* -------------------------------------------------------------------- */
/*      Copy out the part array from the record.                        */
/* -------------------------------------------------------------------- */
        if ( nPartMax < nParts ) {
            nPartMax = nParts;
            panParts = (int *) realloc(panParts, nPartMax * sizeof(int) );
        }

        memcpy( panParts, pabyRec + 44 + 8, 4 * nParts );
        for ( i = 0; i < nParts; i++ ) {
            NativeToLittleEndian( 4, panParts+i );
        }

/* -------------------------------------------------------------------- */
/*      Copy out the vertices from the record.                          */
/* -------------------------------------------------------------------- */
        if ( nVertMax < nPoints ) {
            nVertMax = nPoints;
            padVertices = (double *) realloc(padVertices, nVertMax * 2 * sizeof(double) );
        }

        for ( i = 0; i < nPoints; i++ ) {
            memcpy(&(padVertices[i*2]), pabyRec + 44 + 4*nParts + 8 + i * 16, 8 );
            memcpy(&(padVertices[i*2+1]), pabyRec + 44 + 4*nParts + 8 + i * 16 + 8, 8 );

            NativeToLittleEndian( 8, padVertices+i*2 );
            NativeToLittleEndian( 8, padVertices+i*2+1 );
        }
    } else if ( basicType == SHPT_MULTIPOINT ) {
        /* -------------------------------------------------------------------- */
        /*  Extract vertices for a MultiPoint.					*/
        /* -------------------------------------------------------------------- */
        int32		nPoints;
        int    		i;

        memcpy( &nPoints, pabyRec + 44, 4 );
        NativeToLittleEndian( 4, &nPoints );

        *pnVCount = nPoints;
        if ( nVertMax < nPoints ) {
            nVertMax = nPoints;
            padVertices = (double *) realloc(padVertices, nVertMax * 2 * sizeof(double) );
        }

        for ( i = 0; i < nPoints; i++ ) {
            memcpy(padVertices+i*2,   pabyRec + 48 + 16 * i, 8 );
            memcpy(padVertices+i*2+1, pabyRec + 48 + 16 * i + 8, 8 );

            NativeToLittleEndian( 8, padVertices+i*2 );
            NativeToLittleEndian( 8, padVertices+i*2+1 );
        }
    } else if ( basicType == SHPT_POINT ) {
        /* -------------------------------------------------------------------- */
        /*      Extract vertices for a point.                                   */
        /* -------------------------------------------------------------------- */
        *pnVCount = 1;
        if ( nVertMax < 1 ) {
            nVertMax = 1;
            padVertices = (double *) realloc(padVertices,nVertMax * 2 * sizeof(double) );
        }

        memcpy( padVertices, pabyRec + 12, 8 );
        memcpy( padVertices+1, pabyRec + 20, 8 );

        NativeToLittleEndian( 8, padVertices );
        NativeToLittleEndian( 8, padVertices+1 );
    }

    *ppanParts = panParts;

    return padVertices;
}

/************************************************************************/
/*                           SHPReadBounds()                            */
/*                                                                      */
/*      Read the bounds for one shape, or for the whole shapefile.      */
/************************************************************************/

void SHPReadBounds( SHPHandle psSHP, int hEntity, double * padBounds) {

/* -------------------------------------------------------------------- */
/*      Validate the record/entity number.                              */
/* -------------------------------------------------------------------- */
    if ( hEntity < -1 || hEntity >= psSHP->nRecords ) {
        padBounds[0] = 0.0;
        padBounds[0] = 0.0;
        padBounds[0] = 0.0;
        padBounds[0] = 0.0;

        return;
    }
    

    int basicType = psSHP->nShapeType < 30 ? psSHP->nShapeType % 10 : 0;
    
/* -------------------------------------------------------------------- */
/*	If the entity is -1 we fetch the bounds for the whole file.	*/
/* -------------------------------------------------------------------- */
    if ( hEntity == -1 ) {
        padBounds[0] = psSHP->adBounds[0];
        padBounds[1] = psSHP->adBounds[1];
        padBounds[2] = psSHP->adBounds[2];
        padBounds[3] = psSHP->adBounds[3];
    } else if ( basicType != SHPT_POINT ) {
        /* -------------------------------------------------------------------- */
        /*      Extract bounds for any record but a point record.               */
        /* -------------------------------------------------------------------- */
        fseek( psSHP->fpSHP, psSHP->panRecOffset[hEntity]+12, 0 );
        fread( padBounds, sizeof(double)*4, 1, psSHP->fpSHP );

        NativeToLittleEndian( 8, padBounds );
        NativeToLittleEndian( 8, padBounds+1 );
        NativeToLittleEndian( 8, padBounds+2 );
        NativeToLittleEndian( 8, padBounds+3 );
    } else {
        /* -------------------------------------------------------------------- */
        /*      For points we fetch the point, and duplicate it as the          */
        /*      minimum and maximum bound.                                      */
        /* -------------------------------------------------------------------- */
        fseek( psSHP->fpSHP, psSHP->panRecOffset[hEntity]+12, 0 );
        fread( padBounds, sizeof(double)*2, 1, psSHP->fpSHP );

        NativeToLittleEndian( 8, padBounds );
        NativeToLittleEndian( 8, padBounds+1 );
      
        memcpy( padBounds+2, padBounds, 2*sizeof(double) );
    }
}

} // extern "C"
