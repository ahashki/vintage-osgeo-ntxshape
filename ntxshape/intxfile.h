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


//====================================================================
// intxfile.h
//
//====================================================================

#ifndef __INTXFILE_H
#define __INTXFILE_H

#ifdef _WIN32
#include <io.h>
#endif

#ifdef unix
#include <unistd.h>
#include <fcntl.h>
#endif

#ifndef NTX_NODATA
#define NTX_NODATA ((long)(0x80000000))
#endif

//Fortran direct access files are interesting; the record blocks are prefixed
//and suffixed by the record length, making normal C direct access messy.
//However, I can take advantage of the fact that, in NTX, the record length is
//always 256 * sizeof(long) = 1024.  Using that fact, I can look at the first
//long of the file to determine whether the bytes need to be swapped.

//Accessing elements is through 2d array notation, where the first index
//is the record number, and the second is the offset.  If the recno is bad,
//the second subscript will return NTX_NODATA.  It's up to the user to make
//sure the offset portion does not exceed 255.

//Also,there is an at() which returns the long at an absolute offset within
//the file (ignoring those 1024's).  I provided this because the NTX file is
//conceptually a sequential file, despite those fixed-length records.
//Addressing it in terms of rows and columns may be more true to the low-level
//format, but it requires too much of a shift in thought process.
//There is also an at() for 2-d, which is slightly safer than the array notation
//because it's kinder about offsets greater than 255 (it spills these into the
//next record).  Both are implemented using [r][c]

//The 2d array notation is convenient because it matches the file format.
//That's why I keep it, even though I probably won't use it externally.

//There is one other problem that is best handled at this level: features may cross
//file record boundaries.  You don't want to think about those file boundaries
//at all above this level, so there is an array() method to take care of that.
//array() makes a copy of the data if necessary, to ensure that the range is
//contiguous.  This method should be used to request anything larger than one
//32-bit aligned word.
//As with at(), there is a record-offset version which probably won't be used
//externally.
//
//
//Room for improvement:
//Looking back after finishing the application, I realize it would have been
//worth the effort to keep 2 records in memory instead of just one.  That
//would complicate iNTXfile slightly, but would have given me more freedom
//to think about the file as an array rather than a stream.  I had a tendency
//to avoid looking back at the DAD's header once I was into the data part,
//due to the risk that this might cause unnecessary disk I/O.  A pair of
//buffers would have freed me to ignore that possibility.



class iNTXfile {
  private:
    iNTXfile & operator=(const iNTXfile &);
    iNTXfile(const iNTXfile &);
  protected:
    int rawFile;

#ifdef NTXFILE_CACHE
    enum { buffer_count = 4 };

    struct buffer_t {
        long curRecNo;
        long buffer[256];
        buffer_t():curRecNo(-1) { for (int i=0; i<256;++i) buffer[i] = NTX_NODATA; }
    } buffers[buffer_count];

    static long badBuffer[256];

    long *buffer;
#else
    long buffer[256];
#endif

    long curRecNo;

    //Load a new record into the buffer.Return pBuffer if successful, else
    //null pointer.  (don't modify curRecNo or buffer if out of range)
    int LoadRecord(long i);

    int FileError(int errcode, char *message);

    long numRecs;

  private:
    long recBracket;  // record is preceded and followed by this value
                      // (used in integrity checks and byte-swapping)
                      // (used only to indicate byte-swp if no no bracket)
    long bracketSize; // 4 if the record is bracketed; else 0


  public:

    //Construct from filename.
    iNTXfile( const char * pszFilename);

    ~iNTXfile() {
        if (rawFile != -1) {
            close(rawFile);
        }
    }

    const long* operator[](long recNo) {
        if (recNo!=curRecNo) {
            LoadRecord(recNo);
        }
        return buffer;
    }

    long at(long absOffset) { return (*this)[absOffset/256][absOffset%256]; }
    long at(long recno,int offset) { return (*this)[recno+offset/256][offset%256]; }


    const long* array(long recno,int offset, long numelts, long *outbuf = 0, int forceOutbuf = 0);
    const long* array(long absoffset, long numelts, long * outbuf = 0, int forceOutbuf = 0) {
        return array(absoffset/256,absoffset%256,numelts,outbuf,forceOutbuf);
    }


    long count() const { return numRecs; }

    int isValid() const { return ( (numRecs>0) ); }  // && rawFile
};

#endif
