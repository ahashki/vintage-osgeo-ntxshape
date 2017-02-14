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


/* ntxshp.h */

#ifndef __NTXSHP_H
#define __NTXSHP_H

#if defined(__GNUC__) && !defined(__stdcall)
#define __stdcall __attribute__((stdcall))
#endif

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if defined(_NTXSHAPE_DLL)
#define NTX_API __declspec(dllexport)
#else
#define NTX_API __declspec(dllimport)
#endif


#define NTX_CALL __stdcall

#else

#ifdef _NTXSHAPE_DLL
#define NTX_API
#else
#define NTX_API extern
#endif

#define NTX_CALL

#endif



#ifdef __cplusplus
extern "C" {
#endif

#define NTX_FILTER_NORMAL 0
#define NTX_FILTER_POLY   1

#define NTX_READ_FIRST 0
#define NTX_READ_NEXT  1


/*
NTXStatusProc: provides status indication
 It takes a float, which is the current percentage progress,
 and returns a float, which tells the engine at what % it wants
 to be notified again.
*/

struct NTXDescriptor;
struct NTXConverter;
typedef struct NTXConverter *HNTX;

typedef void  (__stdcall * NTXScanDescProc)  (int recno, NTXDescriptor const *, void *);
typedef int   (__stdcall * NTXCustomFilter)  (int recno, NTXDescriptor const *);

NTX_API void  NTX_CALL NTXSetMainWindow      (HWND hMainWindow);

NTX_API HNTX  NTX_CALL NTXOpen               (char const * fname);

NTX_API void  NTX_CALL NTXClose              (HNTX *hntx);

NTX_API long  NTX_CALL NTXIsValid            (HNTX const *hntx);
NTX_API float NTX_CALL NTXGetProgress        (HNTX const *hntx);

NTX_API int   NTX_CALL NTXReadDescriptor     (HNTX const *hntx, NTXDescriptor *desc, int zeroForFirstOneForNext);

NTX_API void  NTX_CALL NTXSetThemeFilter     (HNTX const *hntx, int theme);
NTX_API void  NTX_CALL NTXSetFCodeFilter     (HNTX const *hntx, char const * fcode);
NTX_API void  NTX_CALL NTXSetStandardFilter  (HNTX const *hntx, int filter_id);
NTX_API void  NTX_CALL NTXSetCustomFilter    (HNTX const *hntx, NTXCustomFilter);

NTX_API int   NTX_CALL NTXScanDescriptors    (HNTX const *hntx, NTXScanDescProc, void *, int showProgress);

NTX_API int   NTX_CALL NTXConvertDescriptors (HNTX const *hntx, const char * dbfOut, int append, int showProgress);
NTX_API int   NTX_CALL NTXConvertPoints      (HNTX const *hntx, const char * shpOut, int append, int showProgress);
NTX_API int   NTX_CALL NTXConvertLines       (HNTX const *hntx, const char * shpOut, int append, int showProgress);
NTX_API int   NTX_CALL NTXConvertNames       (HNTX const *hntx, const char * shpOut, int append, int showProgress);
NTX_API int   NTX_CALL NTXConvertPolygons    (HNTX const *hntx, const char * shpOut, int append, int showProgress);
NTX_API int   NTX_CALL NTXConvertLinesZ      (HNTX const *hntx, const char * shpOut, int append, int showProgress);


#ifdef __cplusplus
}
#endif

#ifndef _NTXSHAPE_DLL
#undef NTX_API
#undef NTX_CALL
#endif

#endif
