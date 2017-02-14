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
 * Portions created by ESRI Canada are Copyright (C) 1997-2009
 * ESRI Canada Limited.  All Rights Reserved.
 *
 * Contributor(s):
 *  Bruce Dodson <bdodson@esricanada.com>
**/

// intxfile.cpp
//
// Fortran direct, random-access file.  Implementation.


#include "intxfile.h"
#include "byteswap.h"
#include <fcntl.h>

//#ifdef DEBUG
#include <stdio.h>
//#endif

//1024 (0x400) byteswapped is 0x40000
#define SWAP1024 0x00040000L

#ifdef NTXFILE_CACHE
long iNTXfile::badBuffer[256] = { NTX_NODATA };
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif



int iNTXfile::FileError(int errcode, char *message) {
#ifdef DEBUG
    fprintf(stderr,"LoadRecord failed: %s\n",message);
#endif
#ifdef NTXFILE_CACHE
    buffer = badBuffer;
#endif

    for ( long * bufPtr = buffer+255; bufPtr>=buffer; --bufPtr) {
        *bufPtr = NTX_NODATA;
    }

    return errcode;
}




iNTXfile::iNTXfile(const char * pszFilename)
  : rawFile( open(pszFilename, O_RDONLY|O_BINARY) ),
    curRecNo(-2),
    numRecs(0),
    recBracket(0)
{
    if (rawFile != -1) {
        // Since iNTXfile does its own buffering, in theory, FILE*'s buffer
        // is not needed.  This does seem to be true using Symantec's
        // RTL, or Microsoft's CRTDLL, but Cygwin does very poorly
        // without the buffer.  The same may be true of some other OS's.
        // therefore I'll put in a buffer.

        long signature[4];
        read(rawFile, &signature, 16);

        if ((signature[0] == 1024) && (signature[3] ==5)) {
            bracketSize = 4;
            recBracket  = 1024;
        } else if ((signature[0] == SWAP1024) && (signature[3] == 0x05000000)) {
            bracketSize = 4;
            recBracket  = SWAP1024;
        } else if (signature[2] == 5) {
            bracketSize = 0;
            recBracket  = 1024; // indicates no byteswap.
        } else if (signature[2] == 0x05000000) {
            bracketSize = 0;
            recBracket  = SWAP1024; // indicates need byteswap
        } else {
            close(rawFile);
            rawFile=-1;
            return;
        }

        numRecs = (lseek(rawFile,0,SEEK_END) + 1) / (1024+2*bracketSize);
        lseek(rawFile,0,SEEK_SET);
    }
}




int iNTXfile::LoadRecord(long rno) {
    curRecNo = rno;

    if (rawFile == -1) {
        return FileError(1, "File is not open");
    }

    if ((rno<numRecs) && (rno>=0)) {

#ifdef NTXFILE_CACHE
        static int load_buffer = -1;

        for (int i=0;i<buffer_count;++i) {
          if (buffers[i].curRecNo == curRecNo) {
            buffer = buffers[i].buffer;
            return 0;
          }
        }

        load_buffer = (load_buffer+1) % buffer_count;
        buffer = buffers[load_buffer].buffer;
        buffers[load_buffer].curRecNo = curRecNo;
#endif

        int err = lseek(rawFile, rno * (1024+bracketSize*2) + bracketSize, SEEK_SET);
        if (err == -1) {
          return FileError(2, "Record number in range but could not seek");
        }

        int nRead = read(rawFile, buffer,1024);
        if (nRead<1024)
          return FileError(3, "Could not read entire record");

        //Byteswap the entire record if the byte order is reversed.
        if (recBracket == SWAP1024) {
          for ( long * bufPtr = buffer+255; bufPtr>=buffer; --bufPtr) {
            (void)AllSwap( *bufPtr );                 //byteswap the whole thing
          }
        }

    } else {
        return FileError(4, "Record number out of range");
    }

    return 0;
}








const long *iNTXfile::array(long recno,int offset, long numelts, long *outbuf, int forceOutbuf) {
    //just reference the internal buffer if possible
    if (! (forceOutbuf || offset+numelts > 256) ) {
        if (curRecNo != recno) {
            LoadRecord(recno);
        }
        return buffer + offset;

    } else if (outbuf) {
        if (curRecNo != recno) {
            LoadRecord(recno);
        }

        for (int i = 0; i < numelts ; ++i) {
            outbuf[i] = buffer[offset];
            if (++offset == 256) {
                LoadRecord(++recno);
                offset=0;
            }
        }
    }

    return outbuf;  //this may be null, in which case the user provided
                    //no outbuf, but one was needed.
}
