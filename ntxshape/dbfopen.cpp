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

/*
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
 * attributed to Frank Warmerdam was dated 1996/02/12.
 *
 * Changes from the public domain version of 1996/02/12:
 *
 * Revision 1.8  2002/05/11 02:15:20  bdodson
 * switched to C++
 * made DBFRead*Attribute and DBFWrite*Attribute type-safe
 * dropped the distinction between double and int.  double was
 *  used internally anyway, so int offered no advantage.
 * standardized formatting, added Mozilla license notice
 *
 * Revision 1.7  2000-04-24 14:29:23-03  bdodson
 * Removed some unused variables detected by -Wall
 *
 * Revision 1.6  2000-03-27 19:18:32-04  bdodson
 * dropped rcsid[]
 *
 * Revision 1.5  2000-02-03 23:28:57-04  bdodson
 * change to dBase IV datestamp (i.e. this is year 100, not year 0)
 * so AV3.0 will not think the file is corrupt
 *
 * Revision 1.4  1999-12-13 19:40:22-04  bdodson
 * New DBFFieldInfo structure.  Cleaner and slightly faster.
 *
 * Revision 1.3  1999-03-31 18:59:01-04  bdodson
 * Fixed end-of-records marker
 *
 * Revision 1.2  1999-01-29 12:46:25-04  bdodson
 * whitespace and formatting changes
 *
 * Revision 1.1  1998/06/10 18:42:55  bdodson
 * Initial Revision
 */

#include "shpfile.h"

#include <math.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <time.h>


#ifndef FALSE
#  define FALSE		0
#  define TRUE		1
#endif

#define DBF_END_HEADER (0x0d)
#define DBF_END_RECORDS (0x1a)

extern "C" {

/************************************************************************/
/*                           DBFWriteHeader()                           */
/*                                                                      */
/*      This is called to write out the file header, and field          */
/*      descriptions before writing any actual data records.  This      */
/*      also computes all the DBFDataSet field offset/size/decimals     */
/*      and so forth values.                                            */
/************************************************************************/

static void DBFWriteHeader(DBFHandle psDBF) {
    unsigned char abyHeader[32];
    int i;

    if (!psDBF->bNoHeader) {
        return;
    }

    psDBF->bNoHeader = FALSE;

/* -------------------------------------------------------------------- */
/*	Initialize the file header information.				*/
/* -------------------------------------------------------------------- */
    for (i = 0; i < 32; i++) {
        abyHeader[i] = 0;
    }

    abyHeader[0] = 0x03;		/* memo field? - just copying 	*/

    /* HEADER HAS A TERMINATING CHAR INCLUDED IN ITS LENGTH */
    psDBF->nHeaderLength = psDBF->nHeaderLength + 1;

    /* date updated on close, record count preset at zero */

    abyHeader[8] = psDBF->nHeaderLength % 256;
    abyHeader[9] = psDBF->nHeaderLength / 256;

    abyHeader[10] = psDBF->nRecordLength % 256;
    abyHeader[11] = psDBF->nRecordLength / 256;

/* -------------------------------------------------------------------- */
/*      Write the initial 32 byte file header, and all the field        */
/*      descriptions.                                                   */
/* -------------------------------------------------------------------- */
    fseek(psDBF->fp, 0, 0);
    fwrite(abyHeader, 32, 1, psDBF->fp);
    fwrite(psDBF->pszHeader, 32, psDBF->nFields, psDBF->fp);

    /* WRITE THE HEADER'S TERMINATING CHAR */
    fputc(DBF_END_HEADER, psDBF->fp);


}

/************************************************************************/
/*                           DBFFlushRecord()                           */
/*                                                                      */
/*      Write out the current record if there is one.                   */
/************************************************************************/

static void DBFFlushRecord(DBFHandle psDBF) {
    int nRecordOffset;

    if (psDBF->bCurrentRecordModified && psDBF->nCurrentRecord > -1) {
        psDBF->bCurrentRecordModified = FALSE;

        nRecordOffset = (
            psDBF->nRecordLength * psDBF->nCurrentRecord
          + psDBF->nHeaderLength
        );

        fseek(psDBF->fp, nRecordOffset, 0);
        fwrite(psDBF->pszCurrentRecord, psDBF->nRecordLength, 1, psDBF->fp);
    }
}

/************************************************************************/
/*                              DBFOpen()                               */
/*                                                                      */
/*      Open a .dbf file.                                               */
/************************************************************************/

SHP_API DBFHandle SHP_CALL DBFOpen(const char * pszFilename, const char * pszAccess) {
    DBFHandle psDBF;
    unsigned char *pabyBuf;
    DBFFieldInfo *pField;
    int nFields, nRecords, nHeadLen, nRecLen, iField;

    char szBinaryAccess[4] = "rb+";

/* -------------------------------------------------------------------- */
/*      We only allow the access strings "rb" and "rb+".                */
/* -------------------------------------------------------------------- */

    if (pszAccess[0] != 'r') {
        return NULL;
    }

    if (!strchr(pszAccess,'+')) {
        szBinaryAccess[2] = '\0'; /* drop the + */
    }

    psDBF = (DBFHandle) malloc(sizeof(DBFInfo));
    psDBF->fp = fopen(pszFilename, szBinaryAccess);
    if (psDBF->fp == NULL) {
        free(psDBF);
        return NULL;
    }

    psDBF->bNoHeader = FALSE;
    psDBF->nCurrentRecord = -1;
    psDBF->bCurrentRecordModified = FALSE;

/* -------------------------------------------------------------------- */
/*  Read Table Header info                                              */
/* -------------------------------------------------------------------- */
    pabyBuf = (unsigned char *) malloc(500);
    fread(pabyBuf, 32, 1, psDBF->fp);

    psDBF->nRecords = nRecords = (
        pabyBuf[4]
      + pabyBuf[5] * 0x100
      + pabyBuf[6] * 0x10000
      + pabyBuf[7] * 0x1000000
    );

    psDBF->nHeaderLength = nHeadLen = pabyBuf[8] + pabyBuf[9]*0x100;

    psDBF->nRecordLength = nRecLen = pabyBuf[10] + pabyBuf[11]*0x100;

    psDBF->nFields = nFields = (nHeadLen - 32) / 32;

    psDBF->pszCurrentRecord = (char *) malloc(nRecLen);

/* -------------------------------------------------------------------- */
/*  Read in Field Definitions                                           */
/* -------------------------------------------------------------------- */

    pabyBuf = (unsigned char *) realloc(pabyBuf,nHeadLen);
    psDBF->pszHeader = (char *) pabyBuf;

    fseek(psDBF->fp, 32, 0);
    fread(pabyBuf, nHeadLen, 1, psDBF->fp);

    psDBF->fields = (DBFFieldInfo*) malloc(sizeof(DBFFieldInfo) * nFields);


    for (iField = 0, pField = psDBF->fields; iField < nFields; iField++, ++pField) {
        unsigned char *pabyFInfo;


        pabyFInfo = pabyBuf+iField*32;

        if (pabyFInfo[11] == 'N') {
            pField->size = pabyFInfo[16];
            pField->decimals = pabyFInfo[17];
        } else {
            pField->size = pabyFInfo[16] + pabyFInfo[17]*256;
            pField->decimals = 0;
        }

        pField->type = (char) pabyFInfo[11];

        if( iField == 0 ) {
            pField->offset = 1;
        } else {
            pField->offset = (pField-1)->offset + (pField-1)->size;
        }

        if ((pField->type == 'D') || (pField->type=='N')) {
            sprintf(pField->format, "%%%d.%df", pField->size, pField->decimals);
        } else {
            sprintf(pField->format, "%%%ds", pField->size);
        }
    }

    return psDBF;
}

/************************************************************************/
/*                              DBFClose()                              */
/************************************************************************/

SHP_API void SHP_CALL DBFClose(DBFHandle psDBF) {


/* -------------------------------------------------------------------- */
/*      Write out header if not already written.                        */
/* -------------------------------------------------------------------- */
    if (psDBF->bNoHeader) {
        DBFWriteHeader(psDBF);
    }

    DBFFlushRecord(psDBF);

/* -------------------------------------------------------------------- */
/*      Update last access date, and number of records if we have	*/
/*	write access.                					*/
/* -------------------------------------------------------------------- */
    if (psDBF->bUpdated) {
        struct tm *ptmNow;
        time_t tNow;
        unsigned char abyFileHeader[32];
        int offsetToEnd;

        //WRITE THE DBF EOF INDICATOR IF IT IS NOT PRESENT
        offsetToEnd = psDBF->nRecordLength * psDBF->nRecords + psDBF->nHeaderLength;
        fseek( psDBF->fp, offsetToEnd, 0);

        fputc( DBF_END_RECORDS, psDBF->fp ); //might be overwriting already-written DBF_END_RECORDS

        //UPDATE THE HEADER.
        fseek( psDBF->fp, 0, 0 );
        fread( abyFileHeader, 32, 1, psDBF->fp );

        time(&tNow);
        ptmNow = localtime(&tNow);
        abyFileHeader[1] = ptmNow->tm_year;  // was tm_year % 100; changed so 2001 -> 101 so AV3.0 can read it.
        abyFileHeader[2] = ptmNow->tm_mon + 1;
        abyFileHeader[3] = ptmNow->tm_mday;

        abyFileHeader[4] = psDBF->nRecords & 0xFF;
        abyFileHeader[5] = (psDBF->nRecords/0x100) & 0xFF;
        abyFileHeader[6] = (psDBF->nRecords/(0x10000)) & 0xFF;
        abyFileHeader[7] = (psDBF->nRecords/(0x1000000)) & 0xFF;

        fseek(psDBF->fp, 0, 0);
        fwrite(abyFileHeader, 32, 1, psDBF->fp);
    }

/* -------------------------------------------------------------------- */
/*      Close, and free resources.                                      */
/* -------------------------------------------------------------------- */
    fclose(psDBF->fp);

    if (psDBF->fields != NULL) {
        free(psDBF->fields);
    }


    free(psDBF->pszHeader);
    free(psDBF->pszCurrentRecord);

    free(psDBF);
}

/************************************************************************/
/*                             DBFCreate()                              */
/*                                                                      */
/*      Create a new .dbf file.                                         */
/************************************************************************/

SHP_API DBFHandle SHP_CALL DBFCreate( const char * pszFilename ) {
    DBFHandle psDBF;
    FILE *fp;

/* -------------------------------------------------------------------- */
/*      Create the file.                                                */
/* -------------------------------------------------------------------- */
    fp = fopen( pszFilename, "wb" );
    if (fp == NULL) {
        return NULL;
    }

    fputc(0, fp);
    fclose(fp);

    fp = fopen( pszFilename, "rb+" );
    if (fp == NULL) {
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*	Create the info structure.					*/
/* -------------------------------------------------------------------- */
    psDBF = (DBFHandle) malloc(sizeof(DBFInfo));

    psDBF->fp = fp;
    psDBF->nRecords = 0;
    psDBF->nFields = 0;
    psDBF->nRecordLength = 1;
    psDBF->nHeaderLength = 32;

    psDBF->fields = NULL;

    psDBF->pszHeader = NULL;

    psDBF->nCurrentRecord = -1;
    psDBF->bCurrentRecordModified = FALSE;
    psDBF->pszCurrentRecord = NULL;

    psDBF->bNoHeader = TRUE;

    return psDBF;
}

/************************************************************************/
/*                            DBFAddField()                             */
/*                                                                      */
/*      Add a field to a newly created .dbf file before any records     */
/*      are written.                                                    */
/************************************************************************/

SHP_API int	SHP_CALL DBFAddField(
    DBFHandle psDBF,
    const char * pszFieldName,
    DBFFieldType eType,
    int nWidth,
    int nDecimals
) {
    char *pszFInfo;
    DBFFieldInfo *pField;
    int i;

/* -------------------------------------------------------------------- */
/*      Do some checking to ensure we can add records to this file.     */
/* -------------------------------------------------------------------- */
    if ((psDBF->nRecords > 0) || (!psDBF->bNoHeader)) {
        return FALSE;
    }

    if ((eType != FTNumber) && (nDecimals != 0)) {
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      realloc all the arrays larger to hold the additional field      */
/*      information.                                                    */
/* -------------------------------------------------------------------- */
    psDBF->nFields++;

    psDBF->fields = (DBFFieldInfo*) realloc(psDBF->fields, sizeof(DBFFieldInfo)*psDBF->nFields);

/* -------------------------------------------------------------------- */
/*      Assign the new field information fields.                        */
/* -------------------------------------------------------------------- */

    pField = psDBF->fields + (psDBF->nFields-1);
    pField->offset = psDBF->nRecordLength;

    pField->size = nWidth;
    psDBF->nRecordLength += nWidth;

    pField->decimals = nDecimals;

    if (eType == FTString) {
        pField->type = 'C';
        sprintf( pField->format, "%%%ds", pField->size );
    } else {
        pField->type = 'N';
        sprintf( pField->format, "%%%d.%df", pField->size, pField->decimals );
    }



/* -------------------------------------------------------------------- */
/*      Extend the required header information.                         */
/* -------------------------------------------------------------------- */
    psDBF->nHeaderLength += 32;

    psDBF->pszHeader = (char *) realloc(psDBF->pszHeader,psDBF->nFields*32);

    pszFInfo = psDBF->pszHeader + 32 * (psDBF->nFields-1);

    for (i = 0; i < 32; i++) {
        pszFInfo[i] = '\0';
    }

    if (strlen(pszFieldName) < 10 ) {
        for (i = 0; i < (int)strlen(pszFieldName); i++) {
            pszFInfo[i] = toupper(pszFieldName[i]);
        }
    } else {
        for (i = 0; i < 10; i++) {
            pszFInfo[i] = toupper(pszFieldName[i]);
        }
    }
    

    pszFInfo[11] = pField->type;

    if (eType == FTString) {
        pszFInfo[16] = nWidth % 0x100;
        pszFInfo[17] = nWidth / 0x100;
    } else {
        pszFInfo[16] = nWidth;
        pszFInfo[17] = nDecimals;
    }

/* -------------------------------------------------------------------- */
/*      Make the current record buffer appropriately larger.            */
/* -------------------------------------------------------------------- */
    psDBF->pszCurrentRecord = (char *) (
        realloc(psDBF->pszCurrentRecord, psDBF->nRecordLength)
    );

    return( TRUE );
}




static int DBFGotoRecord(DBFHandle psDBF, int hEntity, int forWrite = 0) {
    int nRecordOffset;

    if (psDBF->bNoHeader && forWrite && hEntity==0) {
        DBFWriteHeader(psDBF);
        psDBF->bUpdated = TRUE;
    }

    if (hEntity == psDBF->nRecords && forWrite) {
        DBFFlushRecord(psDBF);

        psDBF->nRecords++;
        for (int i = 0; i < psDBF->nRecordLength; i++) {
            psDBF->pszCurrentRecord[i] = ' ';
        }

        psDBF->nCurrentRecord = hEntity;
        
        psDBF->bUpdated = TRUE;
        psDBF->bCurrentRecordModified = TRUE;

    } else if (hEntity < 0 || hEntity >= psDBF->nRecords) {
        return FALSE;
    }
    
    
    if (psDBF->nCurrentRecord != hEntity) {
        DBFFlushRecord(psDBF);

        nRecordOffset = psDBF->nRecordLength * hEntity + psDBF->nHeaderLength;

        fseek(psDBF->fp, nRecordOffset, 0);
        fread(psDBF->pszCurrentRecord, psDBF->nRecordLength, 1, psDBF->fp);

        psDBF->nCurrentRecord = hEntity;
    }
    
    return TRUE;
}






/************************************************************************/
/*                        DBFReadDoubleAttribute()                      */
/*                                                                      */
/*      Read a double attribute.                                        */
/************************************************************************/
  
  
SHP_API double SHP_CALL DBFReadDoubleAttribute(DBFHandle psDBF, int hEntity, int iField) {
  
    if (DBFGotoRecord(psDBF, hEntity)) {
        unsigned char * pabyRec = (unsigned char *) psDBF->pszCurrentRecord;
        DBFFieldInfo * pField = psDBF->fields + iField;
        if( pField->type == 'N' || pField->type == 'D' ) {
            double dDoubleField;
            char pszStringField[pField->size + 1];
            strncpy(pszStringField, (char const *)(pabyRec+pField->offset), pField->size);
            pszStringField[pField->size] = '\0';
          
            sscanf( pszStringField, "%lf", &dDoubleField );
            return dDoubleField;
        }
    }
    
    return 0;
}

/************************************************************************/
/*                        DBFReadIntAttribute()                         */
/*                                                                      */
/*      Read an integer attribute.                                      */
/************************************************************************/

SHP_API int SHP_CALL DBFReadIntegerAttribute(DBFHandle psDBF, int iRecord, int iField) {
    return (int) DBFReadDoubleAttribute(psDBF, iRecord, iField);
}



/************************************************************************/
/*                        DBFReadStringAttribute()                      */
/*                                                                      */
/*      Read a string attribute.                                        */
/*      This will work with any field type. 				*/
/*      If you pass in a buffer, make sure it's big enough.             */
/*      If you don't pass in a buffer, make sure you free the result.   */
/************************************************************************/


SHP_API const char * SHP_CALL DBFReadStringAttribute(DBFHandle psDBF, int hEntity, int iField, char *buffer) {

    if (DBFGotoRecord(psDBF, hEntity)) {
        unsigned char *pabyRec = (unsigned char *) (psDBF->pszCurrentRecord);
        DBFFieldInfo *pField = psDBF->fields + iField;
      
        if (!buffer) {
            buffer = (char *) malloc(pField->size + 1);
        }
        
        if (buffer) {
            strncpy(buffer, (char const *)(pabyRec+pField->offset), pField->size);
            buffer[pField->size] = '\0';
            return buffer;
        }
    }
    
    return NULL;
}


/************************************************************************/
/*                          DBFGetFieldCount()                          */
/*                                                                      */
/*      Return the number of fields in this table.                      */
/************************************************************************/

SHP_API int	SHP_CALL DBFGetFieldCount(DBFHandle psDBF) {
    return( psDBF->nFields );
}

/************************************************************************/
/*                         DBFGetRecordCount()                          */
/*                                                                      */
/*      Return the number of records in this table.                     */
/************************************************************************/

SHP_API int	SHP_CALL DBFGetRecordCount(DBFHandle psDBF) {
    return psDBF->nRecords;
}

/************************************************************************/
/*                          DBFGetFieldInfo()                           */
/*                                                                      */
/*      Return any requested information about the field.               */
/************************************************************************/

SHP_API DBFFieldType SHP_CALL DBFGetFieldInfo(
    DBFHandle psDBF, int iField,
    char * pszFieldName, int * pnWidth, int * pnDecimals
) {
    DBFFieldInfo *pField = psDBF->fields + iField;

    if (iField < 0 || iField >= psDBF->nFields) {
        return FTInvalid;
    }

    if (pnWidth != NULL) {
        *pnWidth = pField->size;
    }

    if (pnDecimals != NULL) {
        *pnDecimals = pField->decimals;
    }

    if (pszFieldName != NULL) {
        int i;
      
        char *pHeaderField = (char *)(psDBF->pszHeader+iField*32);
        for (i = 0; i < 10 && pHeaderField[i] != ' '; ++i) {
            pszFieldName[i] = toupper(pHeaderField[i]);
        }
        
        pszFieldName[i] = '\0';
    }

    if( pField->type == 'N' || pField->type == 'D' ) {
        return( FTNumber );
    } else {
        return( FTString );
    }
}


/************************************************************************/
/*                      DBFWriteDoubleAttribute()                       */
/*                                                                      */
/*      Write a double attribute.                                       */
/************************************************************************/

SHP_API int SHP_CALL DBFWriteDoubleAttribute(DBFHandle psDBF, int iRecord, int iField, double dValue) {
    if (DBFGotoRecord(psDBF,iRecord, TRUE)) {
        unsigned char *pabyRec = (unsigned char *) psDBF->pszCurrentRecord;
        DBFFieldInfo *pField = psDBF->fields + iField;
        if( pField->type == 'N' || pField->type == 'D' ) {
          
            char szSField[80];
            sprintf(szSField, pField->format, dValue);
            if (strlen(szSField) > (size_t)pField->size) {
                szSField[pField->size] = '\0';
            }
    
            strncpy((char *)(pabyRec+pField->offset), szSField, strlen(szSField));

            psDBF->bCurrentRecordModified = TRUE;
            psDBF->bUpdated = TRUE;
            
            return TRUE;
        }
      }
  
    return FALSE;
}

/************************************************************************/
/*                      DBFWriteIntegerAttribute()                      */
/*                                                                      */
/*      Write an integer attribute.                                     */
/************************************************************************/

SHP_API int SHP_CALL DBFWriteIntegerAttribute(DBFHandle psDBF, int iRecord, int iField, int nValue) {
    return DBFWriteDoubleAttribute(psDBF, iRecord, iField, (double) nValue);
}

/************************************************************************/
/*                      DBFWriteStringAttribute()                       */
/*                                                                      */
/*      Write a string attribute.                                       */
/************************************************************************/

SHP_API int SHP_CALL DBFWriteStringAttribute(DBFHandle psDBF, int iRecord, int iField, const char *pszValue) {
    if (DBFGotoRecord(psDBF,iRecord, TRUE)) {
        unsigned char *pabyRec = (unsigned char *) psDBF->pszCurrentRecord;
        DBFFieldInfo *pField = psDBF->fields + iField;
      
        int j = strlen(pszValue);
        if (j > pField->size) {
            j = pField->size;
        }

        strncpy((char *) (pabyRec+pField->offset), pszValue, j);
        
        psDBF->bCurrentRecordModified = TRUE;
        psDBF->bUpdated = TRUE;
        
        return TRUE;
    }
  
    return FALSE;
}


} // extern "C"
