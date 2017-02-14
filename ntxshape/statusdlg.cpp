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


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "statusdlg.h"
#include <stdio.h>

extern "C" {

    // The critical section isn't really required, but what it does is it prevents
    // 2 calls (from different threads) from running at the same time, both showing
    // progress.  (One thread will block until the other is done.)
    
    // usage: OpenProgressDialog(message, hwndParent)
    // The hwndParent argument causes it to fail, so best to leave that at the default (NULL).
    
    // UpdateProgressDialog(float percent)
    
    // CloseProgressDialog();
    // You _must_ call this; it is what releases the critical section, for one thing.
    
    // The dialog runs in its own thread because I didn't think it would update properly
    // otherwise.
    
    //extern HINSTANCE dllModuleHandle;
    extern CRITICAL_SECTION dllCriticalSection;
    
    HANDLE hThreadStatusDlg = 0;
    volatile HWND hWndStatusDlg = 0;
    
    enum {
      WM_STATUSDLG_DONE = WM_USER+1
    };
    
    BOOL CALLBACK StatusDlgProc(
        HWND hwndDlg,	// handle to dialog box
        UINT uMsg,	// message
        WPARAM wParam,	// first message parameter
        LPARAM lParam 	// second message parameter
    ) {
        if (uMsg==WM_INITDIALOG) {
            hWndStatusDlg = hwndDlg;
            SendMessage(
                GetDlgItem(hwndDlg, IDC_ProgressBar),
                PBM_SETRANGE, 0, MAKELPARAM(0,1000)
            );
          
            if (lParam) {
                SetEvent((HANDLE)lParam);
            }
            return 1;
            
        } else if (uMsg==WM_STATUSDLG_DONE) {
            EndDialog(hwndDlg, (int)wParam);
            return 1;
          
        } else {
            return 0;
        }
    }

    
    DWORD WINAPI StatusDlgThreadFunction(LPVOID pvData) {
        InitCommonControls();
    
        LPVOID *threadParams = (LPVOID *)(pvData);
        return DialogBoxParam(
            GetModuleHandle(NTX_DLL_NAME),
            MAKEINTRESOURCE(IDD_ProgressDialog),
            (HWND)(threadParams[0]),
            (DLGPROC)StatusDlgProc,
            (LPARAM)(threadParams[1])
        );
    }
    
    // create the status dialog in a separate thread, so that its message queue will not block?
    
    
    
    
    void __stdcall OpenProgressDialog(const char *message, HWND hwndParent) {
        EnterCriticalSection(&dllCriticalSection);
    
    
        // initialize the event
        HANDLE hEvtDlgReady = CreateEvent(NULL, TRUE, FALSE, NULL);
    
        LPVOID threadParams[2] = {
            (LPVOID)(IsWindow(hwndParent)?hwndParent:0),
            (LPVOID)hEvtDlgReady
        };
    
        DWORD dwThreadId;
        hThreadStatusDlg = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StatusDlgThreadFunction, (LPVOID)(threadParams), 0, &dwThreadId);
        if (hThreadStatusDlg) {
            DWORD waitResult = WaitForSingleObject(hEvtDlgReady, 20000);
            if (waitResult==WAIT_OBJECT_0) {
                if (message && *message) {
                    SetDlgItemText(hWndStatusDlg, IDC_StaticTaskName, message);
                }
                if (hwndParent==NULL) {
                    SetWindowPos(hWndStatusDlg, HWND_TOPMOST, 0,0 , 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_ASYNCWINDOWPOS);
                }
            } else {
                TerminateThread(hThreadStatusDlg, 1);
                CloseHandle(hThreadStatusDlg);
                hThreadStatusDlg = 0;
            }
        }
    
        CloseHandle(hEvtDlgReady); //hEvtDlgReady
    }
    
    
    void __stdcall UpdateProgressDialog(float progress) {
        //progress > 0 &&
        if (hThreadStatusDlg && hWndStatusDlg) {
            PostMessage(
                GetDlgItem(hWndStatusDlg, IDC_ProgressBar),
                PBM_SETPOS, (WPARAM)(progress*10), 0
            );
        }
    }
    
    void __stdcall CloseProgressDialog() {
        if (hWndStatusDlg) {
            PostMessage(hWndStatusDlg, WM_STATUSDLG_DONE, 0, 0);
            hWndStatusDlg = 0;
        }
    
        if (hThreadStatusDlg) {
            DWORD waitResult = WaitForSingleObject(hThreadStatusDlg, 20000);
            if (waitResult!=WAIT_OBJECT_0) {
                if (hThreadStatusDlg) {
                    TerminateThread(hThreadStatusDlg, 1);
                }
            }
            CloseHandle(hThreadStatusDlg);
            hThreadStatusDlg = 0;
        }
    
        LeaveCriticalSection(&dllCriticalSection);
    }

}
