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

//ntxshp.cpp

#define _NTXSHAPE_DLL
#include "ntxshp.h"

#include "ntx.h"
#include "ntxshape.h"

#include "avexec.h"
#include "statusdlg.h"

extern "C" {
    
    extern int isArcView3x;
    
    HWND hwndMain = 0;
    
    void __stdcall AVStatusBar (float percent) {
        char script[32];
        sprintf(script,"av.setstatus(%8i)",(int)percent);
        AVExec(script);
    }
    
    
    static void BeginProgress(HNTX ntx, const char *message, const char *outFile=0) {
    
        if (ntx) {
    
            if (isArcView3x) {
                // ArcView extension is responsible for doing this.
                //char script[1024];
                //sprintf(script, "av.ShowMessage(\"%s\")", message);
                //AVExec(script);
    
                ntx->SetStatusCallback(AVStatusBar);
    
            } else {
                char fullMessage[200]="";
                if (outFile && *outFile) {
                    const char *baseName = strrchr(outFile,'\\');
                    if (!baseName) {
                        baseName = strchr(outFile,':');
                    }
                    baseName = baseName ? baseName+1 : outFile;
                    _snprintf(fullMessage, 200, "%s to %s...", message, baseName);
                } else {
                    _snprintf(fullMessage, 200, "%s...", message);
                }
                
                fullMessage[199] = 0;
       
                OpenProgressDialog(fullMessage, hwndMain);
       
                ntx->SetStatusCallback(UpdateProgressDialog);
            }
        }
    }
    
    static void EndProgress(HNTX ntx) {
        if (ntx) {
            ntx->SetStatusCallback(NULL);
      
            if (isArcView3x) {
                //ArcView extension is responsible for doing its own tear-down
                //AVExec("av.ClearStatus");
            } else {
                CloseProgressDialog();
            }
        }
    }
    
    
    NTX_API void  NTX_CALL NTXSetMainWindow (HWND hMainWindow) {
        hwndMain = hMainWindow;
    }
    
    
    HNTX NTX_CALL NTXOpen(char const * fname) {
        try {
            NTXConverter *ntx = new NTXConverter(fname);
            return ntx;
        } catch (...) {
            return 0;
        }
    }
    

    void NTX_CALL NTXClose(HNTX *hntx) {
       if (hntx && *hntx) {
           delete (*hntx);
           *hntx = 0;
       }
    }
    

    long NTX_CALL NTXIsValid(HNTX const *hntx) {
        return ( hntx && *hntx && (*hntx)->IsValid() );
    }
    

    float NTX_CALL NTXGetProgress(HNTX const *hntx) {
        return ( (hntx&&*hntx) ? (*hntx)->GetProgress() : 0);
    }
    

    int NTX_CALL NTXReadDescriptor(HNTX const *hntx, NTXDescriptor *desc, int zeroForFirstOneForNext) {
        if (!(hntx && *hntx && desc)) {
            return NTX_NODATA;
        }
        return (*hntx)->ReadDescriptor(*desc, zeroForFirstOneForNext==0 ? ntxReadFirst : ntxReadNext);
    }

    
    int NTX_CALL NTXScanDescriptors(HNTX const *hntx, NTXScanDescProc callback, void *cbdata, int showProgress) {
        if (!(hntx && *hntx && callback)) {
            return 1;
        }
        
        HNTX ntx = *hntx;
        
        if (showProgress) {
            BeginProgress(ntx, "Scanning descriptors", NULL);
        }
        
        int result = ntx->ScanDescriptors(callback, cbdata);
        
        if (showProgress) {
            EndProgress(ntx);
        }
      
        return result;
    }
    
    
    
    int NTX_CALL NTXConvertDescriptors(HNTX const *hntx,const char * dbfOut,int append, int showProgress) {
        if (!(hntx && *hntx)) {
            return 1;
        }
        
        HNTX ntx = *hntx;
        
        if (showProgress) {
            BeginProgress(ntx, "Converting descriptors", dbfOut);
        }
        
        int result = ntx->ConvertDescriptors(dbfOut, append);
        
        if (showProgress) {
            EndProgress(ntx);
        }
      
        return result;
    }
    
    
    int NTX_CALL NTXConvertPoints(HNTX const *hntx, const char * pntOut, int append, int showProgress) {
        if (!(hntx && *hntx)) {
            return 1;
        }
        HNTX ntx = *hntx;
        if (showProgress) {
            BeginProgress(ntx, "Converting points", pntOut);
        }
        
        int result = ntx->ConvertPoints(pntOut,append);
        
        if (showProgress) {
            EndProgress(ntx);
        }
      
        return result;
    }
    
    
    int NTX_CALL NTXConvertLines(HNTX const *hntx,const char * linOut, int append, int showProgress) {
        if (!(hntx && *hntx)) {
            return 1;
        }
        
        HNTX ntx = *hntx;
        
        if (showProgress) {
            BeginProgress(ntx, "Converting lines", linOut);
        }
        
        int result = ntx->ConvertLines(linOut, append);
        
        if (showProgress) {
            EndProgress(ntx);
        }
      
        return result;
    }

    
    int NTX_CALL NTXConvertLinesZ(HNTX const *hntx,const char * linOut, int append, int showProgress) {
        if (!(hntx && *hntx)) {
            return 1;
        }
        
        HNTX ntx = *hntx;
        
        if (showProgress) {
            BeginProgress(ntx, "Converting 3D lines", linOut);
        }
        
        int result = ntx->ConvertLinesZ(linOut, append);
        
        if (showProgress) {
            EndProgress(ntx);
        }
        
        return result;
    }
    
    
    int NTX_CALL NTXConvertNames(HNTX const *hntx, const char * namOut, int append, int showProgress) {
        if (!(hntx && *hntx)) {
            return 1;
        }
        
        HNTX ntx = *hntx;
        
        if (showProgress) {
            BeginProgress(ntx, "Converting names", namOut);
        }
        
        int result = ntx->ConvertNames(namOut, append);
        
        if (showProgress) {
            EndProgress(ntx);
        }
      
        return result;
    }
    
    // At first this one seemed to contain a leak that I could not find in
    // the code.  However, it's not a programmer-error leak.  The process
    // just doesn't return freed memory to the OS under normal circumstances.
    // This levels off after a couple of successive calls.  However where
    // the calling process might use a different heap, the memory allocated
    // by SC might never be re-used.  NTXConvertPolygons could allocate its
    // own heap, or a mem-mapped file, to manage all the memory it allocates.
    
    int NTX_CALL NTXConvertPolygons(HNTX const * hntx, const char *polOut, int append, int showProgress) {
        if (!(hntx && *hntx)) {
            return 1;
        }
        
        HNTX ntx = *hntx;
        
        if (showProgress) {
            BeginProgress(ntx,"Converting polygons", polOut);
        }
        
        int result = ntx->ConvertPolygons(polOut, append);
        
        if (showProgress) {
            EndProgress(ntx);
        }
      
        return result;
    }
    
    void NTX_CALL NTXSetThemeFilter(HNTX const *hntx, int theme) {
        if (hntx && *hntx) {
            (*hntx)->SetThemeNumber(theme);
        }
    }
    void NTX_CALL NTXSetFCodeFilter(HNTX const *hntx, char const *fcode) {
        if (hntx && *hntx) {
            (*hntx)->SetFcodePattern(fcode);
        }
    }
    
    void NTX_CALL NTXSetCustomFilter(HNTX const *hntx, NTXCustomFilter custom) {
        if (hntx && *hntx) {
            (*hntx)->SetCustomFilter(custom);
        }
    }
    
    void NTX_CALL NTXSetStandardFilter(HNTX const *hntx, int filter_id) {
        if (hntx && *hntx) {
            HNTX ntx = *hntx;
        
            switch (filter_id) {
              case 0:
                ntx->SetDataType(0);
                ntx->SetDescFlags(0);
                break;
              
              case 1:
                ntx->SetDataType(7);
                ntx->AddDataType(1);
                ntx->AddDataType(3);
                ntx->AddDataType(4);
                ntx->SetDescFlags( (1<<10) | (1<<11) );
                break;
            }
        }
    }
    
}
