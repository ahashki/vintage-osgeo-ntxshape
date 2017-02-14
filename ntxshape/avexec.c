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
 *  Bruce Dodson, ESRI Canada <bdodson@esricanada.com>
**/ 

/* avexec.c */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "avexec.h"


static HINSTANCE _hm_avexec = NULL;
typedef char *(*_pf_avexec_t)(char *);
static _pf_avexec_t _pf_avexec = NULL;
static int _load_count = 0;

int UnloadAVExec() {
    _load_count--;

    if (_load_count <= 0) {
        _pf_avexec = NULL;
        _load_count = 0;

        if (_hm_avexec) {
            FreeLibrary(_hm_avexec);
            _hm_avexec = NULL;

            return TRUE;
        }
    }

    return FALSE;
}

int LoadAVExec() {
    _load_count++;

    if (_pf_avexec) return TRUE;

    if (GetModuleHandle(NULL) == GetModuleHandle("ARCVIEW.EXE")) {
        _hm_avexec = LoadLibrary("avexec32.dll");
        if (_hm_avexec) {
            _pf_avexec = (_pf_avexec_t) GetProcAddress(_hm_avexec,"AVExec");

            if (_pf_avexec) return TRUE;
        }
    }

    /* failed; clean up */
    UnloadAVExec();
    return FALSE;
}

char *AVExec(char*source) {

    if (_pf_avexec) {
        return _pf_avexec(source);
    }

    if (LoadAVExec()) {
        char *result;
        result = _pf_avexec(source);
        UnloadAVExec();
        return result;
    }

    return NULL;
}
