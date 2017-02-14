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


//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wtypes.h>
#include <basetyps.h>
#include <stdio.h>
#include "avexec.h"
#include "resource.h"
#include "appmutex.h"

//#include <olectl.h> // - can't include due to a w32api bug
#ifndef SELFREG_E_TYPELIB
#define SELFREG_E_TYPELIB MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x200)
#endif

static AppMutex appMutex("NTXSHAPE_APP_MUTEX");

extern "C" {
  

    int isArcView3x = 0;
    CRITICAL_SECTION dllCriticalSection;
    
    static HRESULT RegUnregServer(int zeroMeansRegister);


  
    __declspec(dllexport) HRESULT __stdcall DllRegisterServer() {
        return RegUnregServer(0);
    }
    
    __declspec(dllexport) HRESULT __stdcall DllUnregisterServer() {
        return RegUnregServer(1);
    }
    
    
    
    BOOL __stdcall DllMain( HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID lpReserved ) {
        switch( ul_reason_for_call ) {
          case DLL_PROCESS_ATTACH:
            InitializeCriticalSection(&dllCriticalSection);
            isArcView3x = LoadAVExec();
            break;
          
          case DLL_PROCESS_DETACH:
            UnloadAVExec();
            DeleteCriticalSection(&dllCriticalSection);
            break;
        }
        return TRUE;
    }
    
    
    
    
    
    
    static HRESULT RegUnregServer(int zeroMeansRegister) {
        char mod_filename[MAX_PATH+4] = "";
    
        // reg_entries contains key/value pairs that will be used during
        // HKEY_CLASSES_ROOT.  These are listed in order of build-up,
        // or reverse order of tear-down.  Note the NULL values in the
        // typelib: these are skipped during registration but are needed
        // for unreg.  Also note the NULL key at the end, which signals
        // end of registry entries.  That's in case this ends up in a
        // string table or somewhere else where sizeof() is not known.
    
        char * tlb_entries[][2] = {
            {"TypeLib\\{990A7B19-B9FB-4641-BCF1-B15D4CE016F0}",NULL},
            {"TypeLib\\{990A7B19-B9FB-4641-BCF1-B15D4CE016F0}\\" NTX_VERSION_BASE ,"NTX Conversion Library"},
            {"TypeLib\\{990A7B19-B9FB-4641-BCF1-B15D4CE016F0}\\" NTX_VERSION_BASE "\\0",NULL},
            {"TypeLib\\{990A7B19-B9FB-4641-BCF1-B15D4CE016F0}\\" NTX_VERSION_BASE "\\0\\win32",mod_filename},
            {"TypeLib\\{990A7B19-B9FB-4641-BCF1-B15D4CE016F0}\\" NTX_VERSION_BASE "\\FLAGS","0"},
            {NULL, NULL}
        };
    
    
        if (0==zeroMeansRegister) {
            char ** entry;
    
            // for some reason that I don't understand, the module handle is wrong.
    
            if (0 == GetModuleFileName(GetModuleHandleA(NTX_DLL_NAME), mod_filename, MAX_PATH)) {
                return E_UNEXPECTED;
            }
    
            strcat(mod_filename,"\\2");
    
            for (int i = 0; *(entry = tlb_entries[i]); ++i) {
                // skip keys with null values; those are for unreg.
                if ( entry[1] && (ERROR_SUCCESS != RegSetValueA(HKEY_CLASSES_ROOT, entry[0], REG_SZ, entry[1], strlen(entry[1]))) ) {
                    return SELFREG_E_TYPELIB;
                }
            }
            
            return S_OK;
            
        } else {
            // NT's RegDeleteKey doesn't implement recursive delete,
            // so tear down the keys in reverse order.
    
            SCODE status = S_OK;
    
            // scan forward for the last entry.  Then walk backward & delete keys.
            int i = 0;
            for (; tlb_entries[i][0]; ++i) {
              // no-op
            }
            
            while (i--) {
                if (ERROR_SUCCESS != RegDeleteKeyA( HKEY_CLASSES_ROOT, tlb_entries[i][0] )) {
                    status = SELFREG_E_TYPELIB;
                }
            }
    
            // If we got an error, assume we're NT and that there
            // are other subkeys that we didn't add.
            return status;
        }
    }
    


}
