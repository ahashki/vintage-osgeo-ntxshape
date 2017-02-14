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
 *
 * The last modification to the public domain source code
 * attributed to Frank Warmerdam was dated 1995/08/23.
 *
 * Changes from the public domain version of 1995/08/23:
 *
 * Revision 1.3  2002/05/11 02:34:34  bdodson
 * api changed slightly to become more type-safe and closer to thread-safe
 *   a buffer can be supplied to DBFReadStringAttribute
 *   FTInteger and FTDouble are replaced with FTNumber
 * standardized indentation, added mozilla license notice
 *
 * Revision 1.2  1999-12-13 19:40:22-04  bdodson
 * New DBFFieldInfo structure.  Cleaner and slightly faster.
 *
 * Revision 1.1  1998-06-10 15:43:27-03  bdodson
 * Initial revision
**/



#ifndef __SHPFILE_H
#define __SHPFILE_H


#define SHP_API
#define SHP_CALL


#ifdef __cplusplus
extern "C" {
#endif



#include <stdio.h>

/************************************************************************/
/*  SHAPEFILE SUPPORT
*************************************************************************/

/*  DATATYPES
*************************************************************************/

typedef enum {
    SHPT_POINT = 1,
    SHPT_ARC = 3,
    SHPT_POLYGON = 5,
    SHPT_MULTIPOINT = 8,
    
    SHPT_POINT_Z = 11,
    SHPT_ARC_Z = 13,
    SHPT_POLYGON_Z = 15,
    SHPT_MULTIPOINT_Z = 18,
    
    SHPT_POINT_M = 21,
    SHPT_ARC_M = 23,
    SHPT_POLYGON_M = 25,
    SHPT_MULTIPOINT_M = 28,
    
    SHPT_MULTIPATCH = 31
} SHPShapeType;

typedef	struct {
    FILE *fpSHP;
    FILE *fpSHX;

    SHPShapeType  nShapeType;
    int	nFileSize;

    int nRecords;
    int	nMaxRecords;
    int	*panRecOffset;
    int	*panRecSize;

    double adBounds[8];

    int bUpdated;
} SHPInfo, *SHPHandle;




/*  FUNCTIONS
*************************************************************************/

/*
 * To access M/Z coordinates, use the ...Ex variant of the function,
 * if it exists.
 *
 * Currently I have only added ShapeWriteVerticesEx.  I'm not concerned 
 * about reading those values, since I use this code for shape output.
 *
 * The old functions are extended with limited support for 3D shapefiles.
 * With ShapeReadVertices and ShapeReadBounds, you can access the 2D part
 * of a shape with M/Z.  Also, if you call ShapeWriteVertices for a
 * measured or 3D shapefile, the M/Z coordinates will just be zero.
 */
 


SHPHandle SHPOpen(
    const char *pszShapeFile, const char *pszAccess
);

SHPHandle SHPCreate(
    const char *pszShapeFile, SHPShapeType nShapeType
);

void SHPGetInfo(
    SHPHandle hSHP,
    int *pnEntities,
    SHPShapeType *pnShapeType
);

double *SHPReadVertices(
    SHPHandle hSHP,
    int iRecno,
    int *pnVCount,
    int *pnPartCount,
    int **ppanParts
);

int SHPWriteVertices(
    SHPHandle hSHP,
    int nVCount,
    int nPartCount,
    int * panParts,
    double * pdVertices
);

int SHPWriteVerticesEx(
    SHPHandle hSHP,
    int nVCount,
    int nPartCount,
    int * panParts,
    double * pdVertices,
    double * pdZValues,
    double * pdMValues
);

void SHPReadBounds(
    SHPHandle hSHP,
    int iShape,
    double * padBounds
);

void SHPClose(SHPHandle hSHP);





/************************************************************************/
/*  DBF SUPPORT
*************************************************************************/

/*  DATATYPES
*************************************************************************/

typedef enum {
    FTString,
    FTNumber,
    FTInvalid
} DBFFieldType;


typedef struct {
  int offset;
  int size;
  int decimals;
  char type;
  char format[16];

} DBFFieldInfo;

typedef	struct {
    FILE *fp;

    int nRecords;

    int	nRecordLength;
    int	nHeaderLength;
    int	nFields;
    DBFFieldInfo*fields;

    char *pszHeader;

    int	nCurrentRecord;
    int	bCurrentRecordModified;
    char *pszCurrentRecord;

    int	bNoHeader;
    int	bUpdated;

} DBFInfo, *DBFHandle;




/**
 * FUNCTIONS
 *
 *  Note: some of these maintain local state.
 *  e.g. If you read one string attribute, a char* is returned.
 *  if you read another string attribute, the first one becomes
 *  garbage.  Easy fix: assign/cast the result to some string
 *  class, e.g. std::string<char>
 *
**/

DBFHandle DBFOpen(
    const char * pszDBFFile,
    const char * pszAccess
);


DBFHandle DBFCreate(
    const char * pszDBFFile
);


int DBFGetFieldCount(
    DBFHandle psDBF
);


int DBFGetRecordCount(
    DBFHandle psDBF
);


int DBFAddField(
    DBFHandle hDBF,
    const char * pszFieldName,
    DBFFieldType eType,
    int nWidth,
    int nDecimals
);


DBFFieldType DBFGetFieldInfo(
    DBFHandle psDBF,
    int iField,
    char * pszFieldName,
    int * pnWidth,
    int * pnDecimals
);


int DBFReadIntegerAttribute(
    DBFHandle hDBF,
    int iShape,
    int iField
);


double DBFReadDoubleAttribute(
    DBFHandle hDBF,
    int iShape,
    int iField
);


const char * DBFReadStringAttribute(
    DBFHandle hDBF,
    int iShape,
    int iField,
    char * buffer
);


int DBFWriteIntegerAttribute(
    DBFHandle hDBF,
    int iShape,
    int iField,
    int nFieldValue
);


int DBFWriteDoubleAttribute(
    DBFHandle hDBF,
    int iShape,
    int iField,
    double dFieldValue
);


int DBFWriteStringAttribute(
    DBFHandle hDBF,
    int iShape,
    int iField,
    const char * pszFieldValue
);


void DBFClose(
    DBFHandle hDBF
);

#ifdef __cplusplus
}
#endif


#endif
